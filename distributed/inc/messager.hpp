#ifndef MESSAGER_HPP
#define MESSAGER_HPP

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
#include <chrono>

#include "cJSON.h"
#include "types.hpp"

using namespace std;

namespace Messager {
    state set_socket_addr(struct sockaddr_in * socket_addr, const char * addr, uint16_t port);

    state connect_to_socket(string hostname, uint16_t port, int * socket_descriptor);

    size_t send_message_socket(int descriptor, const string message);

    size_t send_error_response(int descriptor, const string error_msg);

    state send_string_message(string hostname, uint16_t port, string message, bool close_socket);

    state send_json_message(
        string hostname,
        uint16_t port,
        cJSON * json,
        bool close_socket,
        bool delete_json
    );
}

#endif
