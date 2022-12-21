#include "server.hpp"

using namespace std;

extern unordered_map<string, Room *> connected_rooms;
extern mutex connected_rooms_mutex;

extern bool alarm_system;

void turn_on_off_buzzer_in_all_rooms(const char * action) {
    cJSON * json = cJSON_CreateObject();

    cJSON_AddItemToObject(json, "action", cJSON_CreateString(action));

    connected_rooms_mutex.lock();

    for (auto& [key, room] : connected_rooms) {
        for (int i = 0; i < 3; i++) {
            state send_state = Messager::send_async_json_message(
                room->get_room_service_address(),
                room->get_room_service_port(),
                json,
                false
            );

            if (is_success(send_state)) break;

            this_thread::sleep_for(50ms);
        }
    }

    connected_rooms_mutex.unlock();

    cJSON_Delete(json);
}

void check_all_rooms_for_smoke() {
    connected_rooms_mutex.lock();

    bool has_smoke = false;

    for (auto& [key, room] : connected_rooms) {
        for (auto data : room->get_input_devices()) {
            if (data.type != "fumaca") continue;

            bool value;

            room->get_device_value(data.tag, &value);

            if (value) {
                has_smoke = true;
                break;
            }
        }
    }

    connected_rooms_mutex.unlock();

    if (!has_smoke) turn_on_off_buzzer_in_all_rooms("turn_off_buzzer");
}

void handle_update_room_data(int server_sd, cJSON * request_data, char * client_addr) {
    if (!cJSON_HasObjectItem(request_data, "room_data")) {
        Messager::send_error_message(server_sd, "Chave 'room_data' é obrigatória");
        return;
    }

    cJSON * room_data = cJSON_GetObjectItem(request_data, "room_data");

    string room_name = cJSON_GetObjectItem(room_data, "name")->valuestring;

    connected_rooms_mutex.lock();

    if (connected_rooms.count(room_name) == 0) {
        connected_rooms[room_name] = new Room(room_data, client_addr);
    } else {
        Room * stored_room = connected_rooms[room_name];

        delete stored_room;

        connected_rooms[room_name] = new Room(room_data, client_addr);
    }

    connected_rooms_mutex.unlock();
}

void handle_update_device_value(int server_sd, cJSON * request_data) {
    bool has_tag = cJSON_HasObjectItem(request_data, "tag");
    bool has_room_name = cJSON_HasObjectItem(request_data, "room_name");
    bool has_value = cJSON_HasObjectItem(request_data, "value");
    bool has_type = cJSON_HasObjectItem(request_data, "type");

    if (!has_tag || !has_value || !has_room_name || !has_type) return;

    bool value = cJSON_IsTrue(cJSON_GetObjectItem(request_data, "value"));
    string device_tag = cJSON_GetObjectItem(request_data, "tag")->valuestring;
    string device_type = cJSON_GetObjectItem(request_data, "type")->valuestring;
    string room_name = cJSON_GetObjectItem(request_data, "room_name")->valuestring;

    connected_rooms_mutex.lock();

    if (connected_rooms.count(room_name) == 1) {
        connected_rooms[room_name]->set_device_value(device_tag, value);
    }

    connected_rooms_mutex.unlock();

    if (device_type == "fumaca" && value) {
        thread (turn_on_off_buzzer_in_all_rooms, "turn_on_buzzer").detach();
    } else if (device_type == "fumaca" && !value) {
        thread (check_all_rooms_for_smoke).detach();
    }

    if (device_type == "presenca" || device_type == "janela" || device_type == "porta") {
        if (alarm_system && value) {
            thread (turn_on_off_buzzer_in_all_rooms, "turn_on_buzzer").detach();
        }
    }

    // TODO: remove print
    char * request_data_str = cJSON_Print(request_data);

    cout << "Ação de atualizar dispositivo\n\t" << request_data_str << endl;

    free(request_data_str);
}

void handle_update_temperature_data(int server_sd, cJSON * request_data) {
    bool has_temperature = cJSON_HasObjectItem(request_data, "temperature");
    bool has_humidity = cJSON_HasObjectItem(request_data, "humidity");
    bool has_room_name = cJSON_HasObjectItem(request_data, "room_name");

    if (!has_temperature || !has_humidity || !has_room_name) return;

    string room_name = cJSON_GetObjectItem(request_data, "room_name")->valuestring;
    float temperature = (float) cJSON_GetObjectItem(request_data, "tag")->valuedouble;
    float humidity = (float) cJSON_GetObjectItem(request_data, "type")->valuedouble;

    connected_rooms_mutex.lock();

    if (connected_rooms.count(room_name) == 1) {
        connected_rooms[room_name]->set_temperature_data(temperature, humidity);
    }

    connected_rooms_mutex.unlock();
}

void handle_requested_action(int server_sd, cJSON * request_data, char * client_addr) {
    if (!cJSON_HasObjectItem(request_data, "action")) {
        Messager::send_error_message(server_sd, "Ação desconhecida");
        return;
    }

    char * action = cJSON_GetObjectItem(request_data, "action")->valuestring;

    if (action == NULL) {
        Messager::send_error_message(server_sd, "Ação desconhecida");
        return;
    }

    if (strcmp(action, "update_room_data") == 0) {
        handle_update_room_data(server_sd, request_data, client_addr);
    } else if (strcmp(action, "update_device_value") == 0) {
        handle_update_device_value(server_sd, request_data);
    } else if (strcmp(action, "update_temperature_data") == 0) {
        handle_update_temperature_data(server_sd, request_data);
    } else {
        Messager::send_error_message(server_sd, "Ação desconhecida");
    }
}

void handle_request(int server_sd, string request_message, char * client_addr) {
    cJSON * request_data;

    /* logging received requests in the console */
    // log_receive_request(request_message);

    request_data = cJSON_Parse(request_message.c_str());

    if (request_data == NULL) {
        Messager::send_error_message(server_sd, "Falha ao alocar memória");

        return;
    }

    handle_requested_action(server_sd, request_data, client_addr);

    cJSON_Delete(request_data);
}

void answer_distributed_server(int server_sd, struct sockaddr_in client_addr) {
    string message;

    state recv_state = Messager::receive_message_from_socket(server_sd, message);

    if (is_success(recv_state)) {
        handle_request(server_sd, message, inet_ntoa(client_addr.sin_addr));
    } else {
        Messager::send_error_message(
            server_sd,
            recv_state == ALLOC_FAIL ? "Falha ao alocar memória" : "Ocorreu um erro ao processar a requisição"
        );
    }

    close(server_sd);
}

void Server::start_server(int server_sd) {
    while (true) {
        int accept_sd;
        socklen_t addr_len;
        struct sockaddr_in client_addr;

        addr_len = sizeof(client_addr);
        accept_sd = accept(server_sd, (struct sockaddr *) &client_addr, &addr_len);

        if (accept_sd < 0) {
            cout << "<< Server thread >>" << endl;
            cout << inet_ntoa(client_addr.sin_addr) << ntohs(client_addr.sin_port) << "=> Accept fail: " << strerror(errno) << endl;
            continue;
        }

        thread (answer_distributed_server, accept_sd, client_addr).detach();
    }
}
