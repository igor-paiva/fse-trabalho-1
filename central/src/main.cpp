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
#include <unordered_map>

#include "cJSON.h"
#include "room.hpp"
#include "types.hpp"
#include "messager.hpp"
#include "server.hpp"
#include "menu.hpp"

#define QLEN 10

using namespace std;

unordered_map<string, Room *> connected_rooms;
mutex connected_rooms_mutex;

void print_msg_and_exit(const string message) {
    cout << message << endl;
    exit(1);
}

void handle_critical_failure_int(int rt, const string message) {
    if (rt < 0) {
        print_msg_and_exit(message);
    }
}

int main(int argc, char * argv[]) {
    struct sockaddr_in server_addr;
    int server_sd, bind_ret, listen_ret;
    state set_socket_addr_state;

    if (argc < 3) {
        cout << "You must provide 2 arguments: IP/HOSTNAME and PORT to run the server" << endl;
        exit(0);
    }

    memset((char *) &server_addr, 0, sizeof(server_addr));

    set_socket_addr_state = Messager::set_socket_addr(&server_addr, argv[1], (uint16_t) atoi(argv[2]));

    handle_critical_failure_int(set_socket_addr_state, "Fail to get address info");

    server_sd = socket(AF_INET, SOCK_STREAM, 0);

    handle_critical_failure_int(server_sd, "Fail to create the socket");

    bind_ret = bind(server_sd, (struct sockaddr *) &server_addr, sizeof(server_addr));

    handle_critical_failure_int(bind_ret, "Bind fail");

    listen_ret = listen(server_sd, QLEN);

    handle_critical_failure_int(listen_ret, "Fail to listen on socket");

    thread (Server::start_server, server_sd).detach();

    Menu::main_menu_loop();

    return 0;
}
