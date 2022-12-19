#ifndef MENU_ACTIONS_HPP
#define MENU_ACTIONS_HPP

#include <iostream>

#include "cJSON.h"
#include "room.hpp"
#include "types.hpp"
#include "messager.hpp"

using namespace std;

namespace MenuActions {
    state send_set_output_device_message(string room_name, string device_tag, string & error_msg);
}

#endif
