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
    ALLOC_FAIL = INT_MIN,
    UNABLE_OPEN_FILE,
    UNABLE_READ_FILE,
    UNABLE_WRITE_FILE,
    UNABLE_CREATE_FILE,
    JSON_ITEM_ADD_FAIL,
    JSON_ITEM_PRINT_FAIL,
    JSON_ITEM_CREATE_FAIL,
    UNEXPECTED_JSON_KEY
} error_state;

bool is_error(state value);

bool is_success(state value);

string char_pointer_to_string(char * str);

#endif
