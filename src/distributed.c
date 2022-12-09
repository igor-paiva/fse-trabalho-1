#include "distributed.h"

#define QLEN 10
#define MAX_REQUEST_DATA 16384
#define MAX_ERROR_MESSAGE_LENGTH 256

typedef struct ThreadParam {
    int accept_sd;
    struct sockaddr_in client_addr;
} ThreadParam;

void handle_critical_failure(int rt, char * message) {
    if (rt < 0) {
        printf("%s\n", message);
        exit(1);
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

    // TODO: Write to CSV

    printf(
        "%s => %02d/%02d/%d  %02d:%02d:%02d %ld GMT\n",
        request_data,
        tm.tm_mon + 1,
        tm.tm_mday,
        tm.tm_year + 1900,
        tm.tm_hour,
        tm.tm_min,
        tm.tm_sec,
        tm.tm_gmtoff / 3600
    );
}

void send_response(int descriptor, char * response_msg) {
    send(descriptor, response_msg, MAX_REQUEST_DATA, 0);
}

void send_error_response(int descriptor, char * error_msg) {
    char response_str[16 + MAX_ERROR_MESSAGE_LENGTH];

    sprintf(response_str, "{\"error_msg\":\"%s\"}", error_msg);

    send(descriptor, response_str, sizeof(response_str), 0);
}

void handle_requested_action(int descriptor, char * bufout, cJSON * request_data) {
    char * json_str = cJSON_Print(request_data);

    printf("request JSON:\n%s\n", json_str);

    free(json_str);
}

void handle_request(char * request_string, int descriptor) {
    char * bufout;
    cJSON * request_data;

    /* logging received requests in the console */
    log_receive_request(request_string);

    bufout = (char *) malloc(MAX_REQUEST_DATA * sizeof(char));

    if (bufout == NULL) {
        send_error_response(descriptor, "Falha ao alocar memória");

        return;
    }

    request_data = cJSON_Parse(request_string);

    if (request_data == NULL) {
        send_error_response(descriptor, "Falha ao alocar memória");
        free(bufout);

        return;
    }

    memset(bufout, 0x0, MAX_REQUEST_DATA * sizeof(char));

    handle_requested_action(descriptor, bufout, request_data);

    free(bufout);
    cJSON_Delete(request_data);
}

void * answer_central_server(void * param) {
    char * bufin = NULL;
    ThreadParam * param_struct = (ThreadParam *) param;

    bufin = (char *) malloc(MAX_REQUEST_DATA * sizeof(char));

    if (bufin) {
        while (TRUE) {
            int rec_bytes;
            memset(bufin, 0x0, MAX_REQUEST_DATA * sizeof(char));

            rec_bytes = recv(param_struct->accept_sd, bufin, MAX_REQUEST_DATA, 0);

            if (rec_bytes >= 0) {
                /* to remove later */
                printf("\nReceived request:\n%s\n\n", bufin);

                handle_request(bufin, param_struct->accept_sd);
            }
        }
    } else {
        send_error_response(param_struct->accept_sd, "Falha ao alocar memória");
    }

    // free(param);
    free(bufin);

    close(param_struct->accept_sd);

    pthread_exit(NULL);
}

int main(int argc, char * argv[]) {
    struct sockaddr_in server_addr;
    int sd, bind_res, listen_res;

    if (argc < 3) {
        printf("You must provide 2 arguments: the server address and port \n");
        exit(0);
    }

    memset((char *) &server_addr, 0, sizeof(server_addr));

    set_server_addr(&server_addr, argv[1], argv[2]);

    sd = socket(AF_INET, SOCK_STREAM, 0);

    handle_critical_failure(sd, "Fail to create the socket\n");

    bind_res = bind(sd, (struct sockaddr *) &server_addr, sizeof(server_addr));

    handle_critical_failure(bind_res, "Bind fail\n");

    listen_res = listen(sd, QLEN);

    handle_critical_failure(listen_res, "Fail to listen on socket\n");

    printf("Listening in %s:%s...\n\n", argv[1], argv[2]);

    while (TRUE) {
        pthread_t tid;
        int accept_sd;
        socklen_t addr_len;
        ThreadParam * param;
        struct sockaddr_in client_addr;

        addr_len = sizeof(client_addr);
        accept_sd = accept(sd, (struct sockaddr *) &client_addr, &addr_len);

        if (accept_sd < 0) {
            printf(
                "\n%s:%d: Accept fail\n",
                inet_ntoa(client_addr.sin_addr),
                ntohs(client_addr.sin_port)
            );
            continue;
        }

        param = (ThreadParam *) malloc (sizeof(ThreadParam));

        if (param == NULL) {
            send_error_response(accept_sd, "Falha ao alocar memória");
            continue;
        }

        param->accept_sd = accept_sd;
        param->client_addr = client_addr;

        pthread_create(&tid, 0, (void * (*)(void *)) answer_central_server, (void *) param);
    }

    return 0;
}
