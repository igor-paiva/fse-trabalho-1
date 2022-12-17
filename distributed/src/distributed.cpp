#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <stdio.h>
#include <unistd.h>
#include <string>
#include <iostream>
#include <thread>
#include <cstring>
#include <cstdlib>
#include <sstream>
#include <mutex>
#include <chrono>

#include "cJSON.h"
#include "room.hpp"
#include "types.hpp"
#include "gpio.hpp"
#include "messager.hpp"

using namespace std;

#define QLEN 1
#define MAX_INIT_JSON_SIZE 8192

Room * room;

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
        Messager::send_error_message(server_sd, "Não é possível escrever em pino de entrada");
        return;
    }

    bool current_value;
    state get_state = room->get_device_value(data->tag, &current_value);

    if (is_success(get_state) && current_value != value) {
        state ret = GpioInterface::write_pin(data->gpio, value);

        if (is_success(ret))
            Messager::send_success_message(server_sd, NULL);
        else
            Messager::send_error_message(server_sd, value ? "Falha ao ativar" : "Falha ao desativar");
    } else {
        Messager::send_error_message(server_sd, value ? "Já está ativado" : "Já está desativado");
    }
}

void handle_requested_action(int server_sd, cJSON * request_data) {
    if (!cJSON_HasObjectItem(request_data, "action")) {
        Messager::send_error_message(server_sd, "Ação desconhecida");
        return;
    }

    char * action = cJSON_GetObjectItem(request_data, "action")->valuestring;
    char * tag = cJSON_GetObjectItem(request_data, "tag")->valuestring;

    if (action == NULL || tag == NULL) {
        Messager::send_error_message(server_sd, "Ação desconhecida");
        return;
    }

    unordered_map<string, DeviceData> devices_map = room->get_devices_map();

    if (devices_map.count(tag) == 0)
        Messager::send_error_message(server_sd, "Tag desconhecida");

    DeviceData data = devices_map[tag];

    if (strcmp(action, "activate") == 0) {
        change_pin_value_action(server_sd, true, &data);
    } else if (strcmp(action, "deactivate") == 0) {
        change_pin_value_action(server_sd, false, &data);
    } else {
        Messager::send_error_message(server_sd, "Ação desconhecida");
    }
}

void handle_request(int server_sd, string request_message) {
    cJSON * request_data;

    /* logging received requests in the console */
    log_receive_request(request_message);

    request_data = cJSON_Parse(request_message.c_str());

    if (request_data == NULL) {
        Messager::send_error_message(server_sd, "Falha ao alocar memória");

        return;
    }

    handle_requested_action(server_sd, request_data);

    cJSON_Delete(request_data);
}

void answer_central_server(int server_sd, struct sockaddr_in client_addr) {
    while (true) {
        string message;

        state recv_state = Messager::receive_message_from_socket(server_sd, message);

        if (is_success(recv_state)) {
            handle_request(server_sd, message);
        } else {
            Messager::send_error_message(
                server_sd,
                recv_state == ALLOC_FAIL ? "Falha ao alocar memória" : "Ocorreu um erro ao processar a requisição"
            );
        }
    }

    close(server_sd);
}

void send_existence_to_server(string server_hostname, uint16_t server_port) {
    while (true) {
        cout << "Trying to warn central server of this room existence...\n" << endl;

        state send_state = Messager::send_string_message(
            server_hostname,
            server_port,
            room->to_json_str()
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

void send_sensor_update_message(string tag, bool value) {
    cJSON * json_msg = cJSON_CreateObject();

    cJSON_AddItemToObject(json_msg, "action", cJSON_CreateString("update_pin"));
    cJSON_AddItemToObject(json_msg, "tag", cJSON_CreateString(tag.c_str()));
    cJSON_AddItemToObject(json_msg, "value", cJSON_CreateBool(value ? cJSON_True : cJSON_False));

    Messager::send_json_message(
        room->get_central_server_ip(),
        room->get_central_server_port(),
        json_msg,
        true,
        true
    );
}

void monitor_sensor(DeviceData device_data) {
    while (true) {
        bool read_value;
        bool current_value;

        state read_state = GpioInterface::read_pin(device_data.gpio, &read_value);

        state get_state = room->get_device_value(device_data.tag, &current_value);

        cout << "Monitorando " << device_data.tag << ". Valor lido: " << read_value << ". Valor atual: " << current_value << endl;

        if (is_success(read_state) && is_success(get_state) && current_value != read_value) {
            room->set_device_value(device_data.tag, read_value);

            thread (send_sensor_update_message, device_data.tag, read_value).detach();
        }

        cout << endl << endl;

        this_thread::sleep_for(50ms);
    }
}

void start_sensors_threads() {
    vector<DeviceData> input_devices = room->get_input_devices();

    for (DeviceData data : input_devices) {
        if (data.type == "presenca" || data.type == "fumaca" || data.type == "janela" || data.type == "porta") {
            cout << data.tag << endl;

            thread (monitor_sensor, data).detach();
        }
    }
}

int main(int argc, char * argv[]) {
    int sd, bind_res, listen_res;
    struct sockaddr_in server_addr;
    state set_socket_addr_state;

    if (argc < 2) {
        cout << "You must provide 1 argument: path to inicialization JSON file" << endl;
        exit(0);
    }

    cJSON * json = read_initialization_json(argv[1]);

    room = new Room(json);

    send_existence_to_server(room->get_central_server_ip(), room->get_central_server_port());

    cout << "Central server was warned of this room existence...\n" << endl;

    start_sensors_threads();

    memset((char *) &server_addr, 0, sizeof(server_addr));

    set_socket_addr_state = Messager::set_socket_addr(
        &server_addr,
        room->get_room_service_address().c_str(),
        room->get_room_service_port()
    );

    handle_critical_failure_int(set_socket_addr_state, "Fail to get address info\n");

    sd = socket(AF_INET, SOCK_STREAM, 0);

    handle_critical_failure_int(sd, "Fail to create the socket\n");

    bind_res = bind(sd, (struct sockaddr *) &server_addr, sizeof(server_addr));

    handle_critical_failure_int(bind_res, "Bind fail\n");

    listen_res = listen(sd, QLEN);

    handle_critical_failure_int(listen_res, "Fail to listen on socket\n");

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

        answer_central_server(accept_sd, client_addr);
    }

    delete room;

    return 0;
}
