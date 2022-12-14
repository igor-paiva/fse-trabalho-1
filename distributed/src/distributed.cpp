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
// #include <chrono>

#include "cJSON.h"
#include "room.hpp"

using namespace std;

#define QLEN 1
#define MAX_REQUEST_DATA 16384
#define MAX_INIT_JSON_SIZE 8192

typedef struct ThreadParam {
    int accept_sd;
    struct sockaddr_in client_addr;
} ThreadParam;

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

void set_server_addr(struct sockaddr_in * serv_addr, char * addr, char * port) {
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

    serv_addr->sin_family = AF_INET;
    temp_addr = (struct sockaddr_in *) result->ai_addr;
    serv_addr->sin_addr = temp_addr->sin_addr;
    serv_addr->sin_port = htons(atoi(port));
}

void log_receive_request(char * request_data) {
    time_t t = time(NULL);
    struct tm tm = *localtime(&t);

    // TODO: Write to file

    printf(
        "%02d/%02d/%d  %02d:%02d:%02d %ld GMT\n\t%s\n",
        tm.tm_mon + 1,
        tm.tm_mday,
        tm.tm_year + 1900,
        tm.tm_hour,
        tm.tm_min,
        tm.tm_sec,
        tm.tm_gmtoff / 3600,
        request_data
    );
}

void send_response(int descriptor, const string response_msg) {
    send(descriptor, response_msg.c_str(), response_msg.size(), 0);
}

void send_error_response(int descriptor, const string error_msg) {
    ostringstream os;

    os << "{\"error_msg\":\"" << error_msg << "\"}";

    string response_str = os.str();

    send(descriptor, response_str.c_str(), response_str.size(), 0);
}

string char_pointer_to_string(char * str) {
    ostringstream os;

    os << str;

    return os.str();
}

void handle_requested_action(int descriptor, cJSON * request_data) {
    if (!cJSON_HasObjectItem(request_data, "action")) {
        send_error_response(descriptor, "Ação desconhecida");
        return;
    }

    char * action = cJSON_GetObjectItem(request_data, "action")->valuestring;

    if (action == NULL) {
        send_error_response(descriptor, "Ação desconhecida");
        return;
    }

    if (strcmp(action, "test") == 0) {
        char * json_str = cJSON_Print(request_data);

        cout << "request JSON:\n" << json_str << endl;

        string json_string = char_pointer_to_string(json_str);

        free(json_str);

        send_response(descriptor, json_string);
    } else {
        send_error_response(descriptor, "Ação desconhecida");
    }
}

void handle_request(char * request_string, int descriptor) {
    cJSON * request_data;

    /* logging received requests in the console */
    log_receive_request(request_string);

    request_data = cJSON_Parse(request_string);

    if (request_data == NULL) {
        send_error_response(descriptor, "Falha ao alocar memória");
        return;
    }

    handle_requested_action(descriptor, request_data);

    cJSON_Delete(request_data);
}

void answer_central_server(int accept_sd, struct sockaddr_in client_addr) {
    char * bufin = NULL;

    bufin = (char *) malloc(MAX_REQUEST_DATA * sizeof(char));

    if (bufin) {
        while (true) {
            size_t rec_bytes;
            memset(bufin, 0x0, MAX_REQUEST_DATA * sizeof(char));

            rec_bytes = recv(accept_sd, bufin, MAX_REQUEST_DATA, 0);

            if (rec_bytes > 0) {
                /* TODO: remove later */
                printf("\nReceived request:\n%s\n\n", bufin);

                handle_request(bufin, accept_sd);
            }
        }
    } else {
        send_error_response(accept_sd, "Falha ao alocar memória");
    }

    // free(param);
    free(bufin);

    close(accept_sd);
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

int main(int argc, char * argv[]) {
    struct sockaddr_in server_addr;
    int sd, bind_res, listen_res;

    if (argc < 2) {
        cout << "You must provide 1 argument: path to inicialization JSON file" << endl;
        exit(0);
    }

    cJSON * json = read_initialization_json(argv[1]);

    room = new Room(json);

    exit(0);

    // TODO: initialize sensors data thread

    memset((char *) &server_addr, 0, sizeof(server_addr));

    set_server_addr(&server_addr, argv[1], argv[2]);

    sd = socket(AF_INET, SOCK_STREAM, 0);

    handle_critical_failure_int(sd, "Fail to create the socket\n");

    bind_res = bind(sd, (struct sockaddr *) &server_addr, sizeof(server_addr));

    handle_critical_failure_int(bind_res, "Bind fail\n");

    listen_res = listen(sd, QLEN);

    handle_critical_failure_int(listen_res, "Fail to listen on socket\n");

    cout << "Listening in " << argv[1] << ":" << argv[2] << "...\n" << endl;

    while (true) {
        int accept_sd;
        socklen_t addr_len;
        ThreadParam * thread_param;
        struct sockaddr_in client_addr;

        addr_len = sizeof(client_addr);
        accept_sd = accept(sd, (struct sockaddr *) &client_addr, &addr_len);

        if (accept_sd < 0) {
            cout << inet_ntoa(client_addr.sin_addr) << ntohs(client_addr.sin_port) << "=> Accept fail" << endl;
            continue;
        }

        thread_param = (ThreadParam *) malloc (sizeof(ThreadParam));

        if (thread_param == NULL) {
            send_error_response(accept_sd, "Falha ao alocar memória");
            continue;
        }

        thread_param->accept_sd = accept_sd;
        thread_param->client_addr = client_addr;

        answer_central_server(accept_sd, client_addr);
    }

    return 0;
}
