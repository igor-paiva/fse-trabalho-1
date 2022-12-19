#ifndef TYPES_HPP
#define TYPES_HPP

#include <string>
#include <vector>
#include <iostream>
#include <bits/stdc++.h>

#include "cJSON.h"

using namespace std;

/* general status, could be either a success_state or an error_state */
typedef int state;

/* those will always be positive */
typedef enum {
    SUCCESS = 1
} success_state;

/* those will always be negative */
typedef enum {
    ERROR = INT_MIN,
    ALLOC_FAIL,
    UNABLE_OPEN_FILE,
    UNABLE_READ_FILE,
    UNABLE_WRITE_FILE,
    UNABLE_CREATE_FILE,
    JSON_ITEM_ADD_FAIL,
    JSON_ITEM_PRINT_FAIL,
    JSON_ITEM_CREATE_FAIL,
    JSON_PARSE_FAIL,
    UNEXPECTED_JSON_KEY,
    FAIL_TO_READ_FROM_SOCKET,
    FAIL_TO_SEND_MESSAGE_SOCKET,
    FAIL_TO_GET_ADDRESS_INFO,
    FAIL_TO_CREATE_SOCKET,
    FAIL_TO_CONNECT_ADDRESS_SOCKET,
    FAIL_TO_CLOSE_SOCKET,
    FAIL_TO_INIT_WIRING_PI,
    MAP_KEY_DONT_EXISTS
} error_state;

bool is_error(state value);

bool is_success(state value);

string to_lower_case(string str);

string to_upper_case(string str);

#endif
