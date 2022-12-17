#include "messager.hpp"

using namespace std;

state Messager::connect_to_socket(string hostname, uint16_t port, int * socket_descriptor) {
    int descriptor;
    int connect_ret;
    struct sockaddr_in address;

    memset((char *) &address, 0, sizeof(address));

    Messager::set_socket_addr(&address, hostname.c_str(), port);

    descriptor = socket(AF_INET, SOCK_STREAM, 0);

    if (descriptor < 0) return FAIL_TO_CREATE_SOCKET;

    connect_ret = connect(descriptor, (struct sockaddr *) &address, sizeof(address));

    if (connect_ret < 0) return FAIL_TO_CONNECT_ADDRESS_SOCKET;

    *socket_descriptor = descriptor;

    return SUCCESS;
}

state Messager::set_socket_addr(struct sockaddr_in * socket_addr, const char * addr, uint16_t port) {
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
        return FAIL_TO_GET_ADDRESS_INFO;
    }

    socket_addr->sin_family = AF_INET;
    temp_addr = (struct sockaddr_in *) result->ai_addr;
    socket_addr->sin_addr = temp_addr->sin_addr;
    socket_addr->sin_port = htons(port);

    return SUCCESS;
}

size_t Messager::send_message_socket(int descriptor, const string message) {
    return send(descriptor, message.c_str(), message.size(), 0);
}

size_t Messager::send_error_message(int descriptor, const string error_msg) {
    ostringstream os;

    os << "{\"success\":false,\"error_msg\":\"" << error_msg << "\"}";

    string response_str = os.str();

    return send_message_socket(descriptor, response_str);
}

state Messager::send_string_message(string hostname, uint16_t port, string message, bool close_socket) {
    size_t send_ret;
    int socket_descriptor;
    state connection_state;

    connection_state = connect_to_socket(hostname, port, &socket_descriptor);

    if (is_error(connection_state)) return connection_state;

    send_ret = send_message_socket(socket_descriptor, message);

    if (send_ret < message.size()) return FAIL_TO_SEND_MESSAGE_SOCKET;

    if (close_socket && close(socket_descriptor) < 0)
        return FAIL_TO_CLOSE_SOCKET;

    return SUCCESS;
}

void Messager::send_async_string_message(string hostname, uint16_t port, string message) {
    thread (send_string_message, hostname, port, message, false).detach();
}

state Messager::send_json_message(
    string hostname,
    uint16_t port,
    cJSON * json,
    bool close_socket,
    bool delete_json
) {
    string message;
    size_t send_ret;
    int socket_descriptor;
    state connection_state;

    connection_state = connect_to_socket(hostname, port, &socket_descriptor);

    if (is_error(connection_state)) return connection_state;

    message = cJSON_PrintUnformatted(json);

    send_ret = send_message_socket(socket_descriptor, message);

    if (send_ret < message.size()) return FAIL_TO_SEND_MESSAGE_SOCKET;

    if (delete_json) cJSON_Delete(json);

    if (close_socket && close(socket_descriptor) < 0)
        return FAIL_TO_CLOSE_SOCKET;

    return SUCCESS;
}

void Messager::send_async_json_message(string hostname, uint16_t port, cJSON * json, bool delete_json) {
    string message;

    message = cJSON_PrintUnformatted(json);

    if (delete_json) cJSON_Delete(json);

    thread (send_string_message, hostname, port, message, false).detach();
}

state Messager::receive_message_from_socket(int descriptor, string & string_message) {
    size_t recv_ret;
    char * buffer = NULL;

    buffer = (char *) malloc (MAX_REQUEST_DATA * sizeof(char));

    if (buffer == NULL) return ALLOC_FAIL;

    memset(buffer, 0x0, MAX_REQUEST_DATA * sizeof(char));

    recv_ret = recv(descriptor, buffer, MAX_REQUEST_DATA, 0);

    if (recv_ret <= 0) {
        free(buffer);
        return FAIL_TO_READ_FROM_SOCKET;
    }

    string_message = buffer;

    return SUCCESS;
}
