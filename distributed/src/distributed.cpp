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

using namespace std;

#define QLEN 1
#define MAX_REQUEST_DATA 16384
#define MAX_INIT_JSON_SIZE 8192

Room * room;

int central_server_socket = -1;
mutex central_server_socket_mutex;

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

void set_socket_addr(struct sockaddr_in * socket_addr, const char * addr, uint16_t port) {
    int errcode;
    struct sockaddr_in * temp_addr;
    struct addrinfo hints, * result;

    memset(&hints, 0, sizeof (hints));

    hints.ai_family = PF_UNSPEC;
    hints.ai_flags |= AI_CANONNAME;
    hints.ai_socktype = SOCK_STREAM;

    errcode = getaddrinfo(addr, NULL, &hints, &result);

    if (errcode != 0) {
        perror("getaddrinfo");
        return;
    }

    socket_addr->sin_family = AF_INET;
    temp_addr = (struct sockaddr_in *) result->ai_addr;
    socket_addr->sin_addr = temp_addr->sin_addr;
    socket_addr->sin_port = htons(port);
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

size_t send_message_socket(int descriptor, const string message) {
    return send(descriptor, message.c_str(), message.size(), 0);
}

size_t send_error_response(int descriptor, const string error_msg) {
    ostringstream os;

    os << "{\"error_msg\":\"" << error_msg << "\"}";

    string response_str = os.str();

    return send_message_socket(descriptor, response_str);
}

size_t receive_message_socket(int descriptor, void * buffer, size_t to_read_bytes) {
    return recv(descriptor, buffer, to_read_bytes, 0);
}

size_t send_message_to_central_server(const string message) {
    size_t send_ret;

    lock_guard<mutex> lock(central_server_socket_mutex);

    send_ret = send_message_socket(central_server_socket, message);

    return send_ret;
}

size_t send_error_message_to_central_server(const string error_msg) {
    size_t send_ret;

    lock_guard<mutex> lock(central_server_socket_mutex);

    send_ret = send_error_response(central_server_socket, error_msg);

    return send_ret;
}

state receive_message_from_central_server(string & string_message) {
    char * buffer;
    size_t recv_ret;

    buffer = (char *) malloc (MAX_REQUEST_DATA * sizeof(char));

    if (buffer == NULL) return ALLOC_FAIL;

    memset(buffer, 0x0, MAX_REQUEST_DATA * sizeof(char));

    central_server_socket_mutex.lock();

    recv_ret = receive_message_socket(central_server_socket, buffer, MAX_REQUEST_DATA);

    central_server_socket_mutex.unlock();

    if (recv_ret < 1) return FAIL_TO_READ_FROM_SOCKET;

    string_message = buffer;

    return SUCCESS;
}

void set_central_server_socket(int socket_fd) {
    lock_guard<mutex> lock(central_server_socket_mutex);

    central_server_socket = socket_fd;
}

void change_pin_value_action(bool value, DeviceData * data) {
    if (data->pin_mode != DEVICE_OUTPUT) {
        send_error_message_to_central_server("Não é possível escrever em pino de entrada");
        return;
    }

    bool current_value;
    state get_state = room->get_device_value(data->tag, &current_value);

    if (is_success(get_state) && current_value != value) {
        state ret = GpioInterface::write_pin(data->gpio, value);

        if (is_success(ret))
            send_message_to_central_server("{\"success\":true}");
        else
            send_error_message_to_central_server(value ? "Falha ao ativar" : "Falha ao desativar");
    } else {
        send_error_message_to_central_server(value ? "Já está ativado" : "Já está desativado");
    }
}

void handle_requested_action(cJSON * request_data) {
    if (!cJSON_HasObjectItem(request_data, "action")) {
        send_error_message_to_central_server("Ação desconhecida");
        return;
    }

    char * action = cJSON_GetObjectItem(request_data, "action")->valuestring;
    char * tag = cJSON_GetObjectItem(request_data, "tag")->valuestring;

    if (action == NULL || tag == NULL) {
        send_error_message_to_central_server("Ação desconhecida");
        return;
    }

    unordered_map<string, DeviceData> devices_map = room->get_devices_map();

    if (devices_map.count(tag) == 0)
        send_error_message_to_central_server("Tag desconhecida");

    DeviceData data = devices_map[tag];

    if (strcmp(action, "activate") == 0) {
        change_pin_value_action(true, &data);
    } else if (strcmp(action, "deactivate") == 0) {
        change_pin_value_action(false, &data);
    } else {
        send_error_message_to_central_server("Ação desconhecida");
    }
}

void handle_request(string request_message) {
    cJSON * request_data;

    /* logging received requests in the console */
    log_receive_request(request_message);

    request_data = cJSON_Parse(request_message.c_str());

    if (request_data == NULL) {
        send_error_message_to_central_server("Falha ao alocar memória");
        return;
    }

    handle_requested_action(request_data);

    cJSON_Delete(request_data);
}

void answer_central_server(int accept_sd, struct sockaddr_in client_addr) {
    while (true) {
        string message;

        state recv_state = receive_message_from_central_server(message);

        if (is_success(recv_state)) {
            handle_request(message);
        } else {
            send_error_message_to_central_server(
                recv_state == ALLOC_FAIL ? "Falha ao alocar memória" : "Ocorreu um erro ao processar a requisição"
            );
        }
    }

    close(accept_sd);
}

void send_existence_to_server(
    const char * server_hostname,
    uint16_t server_port
    // const char * local_hostname,
    // uint16_t local_port
) {
    int sd;
    bool error = false;
    // struct sockaddr_in local_addr;
    struct sockaddr_in server_addr;

    memset((char *) &server_addr, 0, sizeof(server_addr));
    set_socket_addr(&server_addr, server_hostname, server_port);

    // memset((char *) &local_addr, 0, sizeof(local_addr));
    // set_socket_addr(&local_addr, local_hostname, local_port);

    while (true) {
        cout << "Trying to warn central server of this room existence...\n" << endl;

        sd = socket(AF_INET, SOCK_STREAM, 0);

        if (sd < 0) error = true;

        // Trying not to use another student port (this doesnt work and I dont know why)
        // if (bind(sd, (struct sockaddr *) &local_addr, sizeof(local_addr)) < 0) {
        //     // cout << "Bind fail" << endl;
        //     cout << "Failed to bind socket! " << strerror(errno) << endl;
        //     error = true;
        // }

        if (connect(sd, (struct sockaddr *) &server_addr, sizeof(server_addr)) < 0)
            error = true;

        if (!error) {
            string message = room->to_json_str();

            if (send_message_socket(sd, message) < message.size())
                error = true;
        }

        if (error) {
            error = false;
            this_thread::sleep_for(1s);
        } else {
            close(sd);
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
    string message;
    cJSON * json_msg = cJSON_CreateObject();

    cJSON_AddItemToObject(json_msg, "action", cJSON_CreateString("update_pin"));
    cJSON_AddItemToObject(json_msg, "tag", cJSON_CreateString(tag.c_str()));
    cJSON_AddItemToObject(json_msg, "value", cJSON_CreateBool(value ? cJSON_True : cJSON_False));

    message = cJSON_PrintUnformatted(json_msg);

    send_message_to_central_server(message);
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

        this_thread::sleep_for(500ms);
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

    if (argc < 2) {
        cout << "You must provide 1 argument: path to inicialization JSON file" << endl;
        exit(0);
    }

    cJSON * json = read_initialization_json(argv[1]);

    room = new Room(json);

    send_existence_to_server(
        room->get_central_server_ip().c_str(),
        room->get_central_server_port()
        // room->get_room_service_address().c_str(),
        // room->get_room_service_client_port()
    );

    cout << "Central server was warned of this room existence...\n" << endl;

    start_sensors_threads();

    memset((char *) &server_addr, 0, sizeof(server_addr));

    set_socket_addr(&server_addr, room->get_room_service_address().c_str(), room->get_room_service_port());

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

        set_central_server_socket(accept_sd);

        answer_central_server(accept_sd, client_addr);
    }

    delete room;

    return 0;
}
