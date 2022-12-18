#include "server.hpp"

using namespace std;

extern unordered_map<string, Room *> connected_rooms;
extern mutex connected_rooms_mutex;

void handle_requested_action(int server_sd, cJSON * request_data) {
    if (!cJSON_HasObjectItem(request_data, "action")) {
        Messager::send_error_message(server_sd, "Ação desconhecida");
        return;
    }

    if (!cJSON_HasObjectItem(request_data, "room_data")) {
        Messager::send_error_message(server_sd, "Chave 'room_data' é obrigatória");
        return;
    }

    char * action = cJSON_GetObjectItem(request_data, "action")->valuestring;
    cJSON * room_data = cJSON_GetObjectItem(request_data, "room_data");

    if (action == NULL) {
        Messager::send_error_message(server_sd, "Ação desconhecida");
        return;
    }

    if (strcmp(action, "update_room_data") == 0) {
        string room_name = cJSON_GetObjectItem(room_data, "name")->valuestring;

        if (connected_rooms.count(room_name) == 0) {
            connected_rooms[room_name] = new Room(room_data);
        } else {
            Room * stored_room = connected_rooms[room_name];

            delete stored_room;

            connected_rooms[room_name] = new Room(room_data);
        }

        // TODO: remove print
        char * request_data_str = cJSON_Print(request_data);

        cout << "Ação de atualizar os dado da sala\n\t" << request_data_str << endl;

        free(request_data_str);
    } else {
        Messager::send_error_message(server_sd, "Ação desconhecida");
    }
}

void handle_request(int server_sd, string request_message) {
    cJSON * request_data;

    /* logging received requests in the console */
    // log_receive_request(request_message);

    request_data = cJSON_Parse(request_message.c_str());

    if (request_data == NULL) {
        Messager::send_error_message(server_sd, "Falha ao alocar memória");

        return;
    }

    handle_requested_action(server_sd, request_data);

    cJSON_Delete(request_data);
}

void answer_distributed_server(int server_sd, struct sockaddr_in client_addr) {
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
