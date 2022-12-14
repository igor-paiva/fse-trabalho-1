#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <stdio.h>
#include <unistd.h>
#include <time.h>
#include <string>
#include <iostream>
#include <thread>
#include <cstring>
#include <cstdlib>
#include <sstream>
#include <mutex>
#include <chrono>
#include <csignal>

#include "cJSON.h"
#include "DHT22.h"
#include "room.hpp"
#include "types.hpp"
#include "gpio.hpp"
#include "messager.hpp"

using namespace std;

#define QLEN 10
#define MAX_INIT_JSON_SIZE 8192
#define SENSOR_UPDATE_MAX_RETRIES 5

Room * room;

void handle_exit_signal(int signum) {
    cout << "\nExiting..." << endl;

    if (signum == SIGINT || signum == SIGTERM || signum == SIGQUIT) {
        if (room && room->get_central_server_ip().length() > 0) {
            cJSON * json_msg = cJSON_CreateObject();

            cJSON_AddItemToObject(json_msg, "action", cJSON_CreateString("remove_room"));
            cJSON_AddItemToObject(json_msg, "room_name", cJSON_CreateString(room->get_name().c_str()));

            Messager::send_json_message(
                room->get_central_server_ip(),
                room->get_central_server_port(),
                json_msg,
                true,
                false
            );

            cJSON_Delete(json_msg);
        }
    }

    exit(0);
}

void print_msg_and_exit(const string message) {
    cout << message << endl;
    exit(1);
}

void handle_critical_failure_int(int rt, const string message) {
    if (rt < 0) {
        print_msg_and_exit(message);
    }
}

void handle_critical_failure_ptr(void * ptr, const string message) {
    if (ptr == NULL) {
        print_msg_and_exit(message);
    }
}

void log_receive_request(string request_data) {
    time_t t = time(NULL);
    struct tm tm = *localtime(&t);

    // TODO: Write to file (this is not mandatory)

    printf(
        "%02d/%02d/%d  %02d:%02d:%02d %ld GMT\n\t%s\n",
        tm.tm_mon + 1,
        tm.tm_mday,
        tm.tm_year + 1900,
        tm.tm_hour,
        tm.tm_min,
        tm.tm_sec,
        tm.tm_gmtoff / 3600,
        request_data.c_str()
    );
}

void change_pin_value_action(int server_sd, bool value, DeviceData * data) {
    if (data->pin_mode != DEVICE_OUTPUT) {
        Messager::send_error_message(server_sd, "N??o ?? poss??vel escrever em pino de entrada");
        return;
    }

    bool current_value;
    state get_state = room->get_device_value(data->tag, &current_value);

    if (is_error(get_state)) {
        Messager::send_error_message(server_sd, "Falha ao buscar o estado atual do dispositivo");
        return;
    }

    if (current_value != value) {
        state ret = GpioInterface::write_pin(data->gpio, value);

        if (is_success(ret))
            Messager::send_success_message(server_sd, NULL);
        else
            Messager::send_error_message(server_sd, value ? "Falha ao ativar" : "Falha ao desativar");
    } else {
        Messager::send_error_message(server_sd, value ? "J?? est?? ativado" : "J?? est?? desativado");
    }
}

void change_all_output_to_value(int server_sd, bool value) {
    cJSON * success_res_json;
    vector<string> error_devices;

    success_res_json = cJSON_CreateObject();
    cJSON * array = cJSON_AddArrayToObject(success_res_json, "successful_devices");

    for (auto device_data : room->get_output_devices()) {
        if (device_data.type != "alarme") {
            state ret = GpioInterface::write_pin(device_data.gpio, value);

            if (is_error(ret)) {
                error_devices.push_back(device_data.tag);
            } else {
                room->set_device_value(device_data.tag, value);
                cJSON_AddItemToArray(array, cJSON_CreateString(device_data.tag.c_str()));
            }
        }
    }

    if (error_devices.size() == 0) {
        Messager::send_success_message(server_sd, success_res_json);
    } else {
        int i = 1;
        string error_msg = "Falha ao ativar o(s) dispositivo(s): ";

        for (auto tag : error_devices) {
            if (i == 1 || i == (int) error_devices.size()) {
                error_msg += tag;
            } else {
                error_msg = error_msg + ", " + tag;
            }

            i++;
        }

        Messager::send_error_message(server_sd, error_msg);
    }
}

state get_device_data_by_request_tag(int server_sd, cJSON * request_data, DeviceData * device_data) {
    char * tag;
    unordered_map<string, DeviceData> devices_map;

    if (!cJSON_HasObjectItem(request_data, "tag")) {
        Messager::send_error_message(server_sd, "Tag n??o informada");
        return ERROR;
    }

    tag = cJSON_GetObjectItem(request_data, "tag")->valuestring;

    if (tag == NULL) {
        Messager::send_error_message(server_sd, "Tag n??o informada");
        return ERROR;
    }

    devices_map = room->get_devices_map();

    if (devices_map.count(tag) == 0) {
        Messager::send_error_message(server_sd, "Tag desconhecida");
        return ERROR;
    }

    *device_data = devices_map[tag];

    return SUCCESS;
}

state set_buzzer_to_value(bool value) {
    DeviceData device_data;

    for (auto data : room->get_output_devices()) {
        if (data.type == "alarme") {
            device_data = data;
            break;
        }
    }

    return GpioInterface::write_pin(device_data.gpio, value);
}

void handle_requested_action(int server_sd, cJSON * request_data) {
    if (!cJSON_HasObjectItem(request_data, "action")) {
        Messager::send_error_message(server_sd, "A????o desconhecida");
        return;
    }

    unordered_map<string, DeviceData> devices_map;
    char * action = cJSON_GetObjectItem(request_data, "action")->valuestring;

    if (action == NULL) {
        Messager::send_error_message(server_sd, "A????o desconhecida");
        return;
    }

    devices_map = room->get_devices_map();

    action = cJSON_GetObjectItem(request_data, "action")->valuestring;

    if (strcmp(action, "activate") == 0) {
        DeviceData device_data;

        get_device_data_by_request_tag(server_sd, request_data, &device_data);

        change_pin_value_action(
            server_sd,
            true,
            &device_data
        );
    } else if (strcmp(action, "deactivate") == 0) {
        DeviceData device_data;

        get_device_data_by_request_tag(server_sd, request_data, &device_data);

        change_pin_value_action(
            server_sd,
            false,
            &device_data
        );
    } else if (strcmp(action, "activate_all") == 0) {
        change_all_output_to_value(server_sd, true);
    } else if (strcmp(action, "deactivate_all") == 0) {
        change_all_output_to_value(server_sd, false);
    } else if (strcmp(action, "turn_on_buzzer") == 0) {
        set_buzzer_to_value(true);
    } else if (strcmp(action, "turn_off_buzzer") == 0) {
        set_buzzer_to_value(false);
    } else {
        Messager::send_error_message(server_sd, "A????o desconhecida");
    }
}

void handle_request(int server_sd, string request_message) {
    cJSON * request_data;

    /* logging received requests in the console */
    log_receive_request(request_message);

    request_data = cJSON_Parse(request_message.c_str());

    if (request_data == NULL) {
        Messager::send_error_message(server_sd, "Falha ao alocar mem??ria");

        return;
    }

    handle_requested_action(server_sd, request_data);

    cJSON_Delete(request_data);
}

void answer_central_server(int server_sd, struct sockaddr_in client_addr) {
    string message;

    state recv_state = Messager::receive_message_from_socket(server_sd, message);

    if (is_success(recv_state)) {
        handle_request(server_sd, message);
    } else {
        Messager::send_error_message(
            server_sd,
            recv_state == ALLOC_FAIL ? "Falha ao alocar mem??ria" : "Ocorreu um erro ao processar a requisi????o"
        );
    }

    close(server_sd);
}

void send_existence_to_server(string server_hostname, uint16_t server_port) {
    while (true) {
        cJSON * json = cJSON_CreateObject();

        cout << "Trying to warn central server of this room existence...\n" << endl;

        cJSON_AddItemToObject(json, "action", cJSON_CreateString("update_room_data"));
        cJSON_AddItemReferenceToObject(json, "room_data", room->to_json());

        state send_state = Messager::send_json_message(
            server_hostname,
            server_port,
            json,
            true,
            true
        );

        if (is_error(send_state)) {
            this_thread::sleep_for(1s);
        } else {
            break;
        }
    }
}

cJSON * read_initialization_json(char * json_path) {
    FILE * file;
    char * file_content;

    file_content = (char *) malloc (MAX_INIT_JSON_SIZE * sizeof(char));

    handle_critical_failure_ptr(file_content, "Fail allocate memory to init JSON file content");

    memset(file_content, 0, MAX_INIT_JSON_SIZE * sizeof(char));

    file = fopen(json_path, "rb");

    if (file == NULL) {
        free(file_content);
        print_msg_and_exit("Fail to open inicialization JSON");
    }

    size_t read_bytes = fread(file_content, sizeof(char), MAX_INIT_JSON_SIZE, file);

    fclose(file);

    if (read_bytes <= 0) {
        free(file_content);
        print_msg_and_exit("Fail to read inicialization JSON");
    }

    file_content = (char *) realloc (file_content, read_bytes);

    cJSON * json = cJSON_Parse(file_content);

    if (json == NULL) {
        free(file_content);
        print_msg_and_exit("Fail to parse JSON");
    }

    return json;
}

void send_sensor_update_message(DeviceData device_data, bool value) {
    cJSON * json_msg = cJSON_CreateObject();

    cJSON_AddItemToObject(json_msg, "action", cJSON_CreateString("update_device_value"));
    cJSON_AddItemToObject(json_msg, "room_name", cJSON_CreateString(room->get_name().c_str()));
    cJSON_AddItemToObject(json_msg, "tag", cJSON_CreateString(device_data.tag.c_str()));
    cJSON_AddItemToObject(json_msg, "type", cJSON_CreateString(device_data.type.c_str()));
    cJSON_AddItemToObject(json_msg, "value", cJSON_CreateBool(value));

    for (int i = 0; i < SENSOR_UPDATE_MAX_RETRIES; i++) {
        state send_state = Messager::send_json_message(
            room->get_central_server_ip(),
            room->get_central_server_port(),
            json_msg,
            true,
            false
        );

        if (is_success(send_state)) break;

        this_thread::sleep_for(60ms);
    }

    cJSON_Delete(json_msg);
}

void monitor_sensor(DeviceData device_data) {
    while (true) {
        bool read_value;
        bool current_value;

        state read_state = GpioInterface::read_pin(device_data.gpio, &read_value);

        state get_state = room->get_device_value(device_data.tag, &current_value);

        if (is_success(read_state) && is_success(get_state) && current_value != read_value) {
            room->set_device_value(device_data.tag, read_value);

            thread (send_sensor_update_message, device_data, read_value).detach();
        }

        this_thread::sleep_for(50ms);
    }
}

void start_sensors_threads() {
    for (DeviceData data : room->get_input_devices()) {
        if (data.type == "presenca" || data.type == "fumaca" || data.type == "janela" || data.type == "porta") {
            thread (monitor_sensor, data).detach();
        }
    }
}

void send_temperature_update_message(float temperature, float humidity) {
    cJSON * json_msg = cJSON_CreateObject();

    cJSON_AddItemToObject(json_msg, "action", cJSON_CreateString("update_temperature_data"));
    cJSON_AddItemToObject(json_msg, "room_name", cJSON_CreateString(room->get_name().c_str()));
    cJSON_AddItemToObject(json_msg, "temperature", cJSON_CreateNumber((double) temperature));
    cJSON_AddItemToObject(json_msg, "humidity", cJSON_CreateNumber((double) humidity));

    for (int i = 0; i < SENSOR_UPDATE_MAX_RETRIES; i++) {
        state send_state = Messager::send_json_message(
            room->get_central_server_ip(),
            room->get_central_server_port(),
            json_msg,
            true,
            false
        );

        if (is_success(send_state)) break;

        this_thread::sleep_for(60ms);
    }

    cJSON_Delete(json_msg);
}

void monitor_temperature(DeviceData device_data) {
    TDHT22 * data = new TDHT22(GpioInterface::get_wiringpi_pin_value(device_data.gpio));

    data->Init();

    while (true) {
        data->Fetch();

        if (data->Valid) {
            room->set_temperature_data(data->Temp, data->Hum);

            thread (send_temperature_update_message, data->Temp, data->Hum).detach();

            this_thread::sleep_for(2s);
        } else {
            this_thread::sleep_for(110ms);
        }
    }

    delete data;
}

int main(int argc, char * argv[]) {
    int sd, bind_res, listen_res;
    struct sockaddr_in server_addr;
    state set_socket_addr_state;

    if (argc < 2) {
        cout << "You must provide 1 argument: path to inicialization JSON file" << endl;
        exit(0);
    }

    signal(SIGINT, handle_exit_signal);
    signal(SIGTERM, handle_exit_signal);
    signal(SIGQUIT, handle_exit_signal);

    cJSON * json = read_initialization_json(argv[1]);

    room = new Room(json);

    send_existence_to_server(room->get_central_server_ip(), room->get_central_server_port());

    cout << "Central server was warned of this room existence...\n" << endl;

    start_sensors_threads();

    thread (monitor_temperature, room->get_temperature_device()).detach();

    memset((char *) &server_addr, 0, sizeof(server_addr));

    set_socket_addr_state = Messager::set_socket_addr(
        &server_addr,
        room->get_room_service_address().c_str(),
        room->get_room_service_port()
    );

    handle_critical_failure_int(set_socket_addr_state, "Fail to get address info");

    sd = socket(AF_INET, SOCK_STREAM, 0);

    handle_critical_failure_int(sd, "Fail to create the socket");

    bind_res = bind(sd, (struct sockaddr *) &server_addr, sizeof(server_addr));

    handle_critical_failure_int(bind_res, "Bind fail");

    listen_res = listen(sd, QLEN);

    handle_critical_failure_int(listen_res, "Fail to listen on socket");

    while (true) {
        int accept_sd;
        socklen_t addr_len;
        struct sockaddr_in client_addr;

        addr_len = sizeof(client_addr);
        accept_sd = accept(sd, (struct sockaddr *) &client_addr, &addr_len);

        if (accept_sd < 0) {
            cout << inet_ntoa(client_addr.sin_addr) << ":" << ntohs(client_addr.sin_port) << "=> Accept fail" << endl;
            continue;
        }

        thread (answer_central_server, accept_sd, client_addr).detach();
    }

    delete room;

    return 0;
}
