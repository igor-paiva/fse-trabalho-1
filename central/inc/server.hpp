#ifndef SERVER_HPP
#define SERVER_HPP

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
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

using namespace std;

namespace Server {
    void start_server(int server_sd);
}

#endif
