#ifndef MENU_ACTIONS_HPP
#define MENU_ACTIONS_HPP

#include <iostream>

#include "cJSON.h"
#include "room.hpp"
#include "types.hpp"
#include "messager.hpp"
#include "logger.hpp"

using namespace std;

namespace MenuActions {
    state send_set_output_device_message(string room_name, string device_tag, string & error_msg);

    state send_set_all_output_device_message_all_rooms(bool value, string & error_msg);

    state send_set_all_output_device_message(string room_name, bool value, string & error_msg);

    void turn_on_alarm();

    state turn_off_alarm(string & error_msg);
}

#endif
