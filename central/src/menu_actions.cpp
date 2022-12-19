#include "menu_actions.hpp"

using namespace std;

extern unordered_map<string, Room *> connected_rooms;
extern mutex connected_rooms_mutex;

state MenuActions::send_set_all_output_device_message(string room_name, bool value, string & error_msg) {
    state send_state;
    string response_msg;
    cJSON * json, * response_json;

    connected_rooms_mutex.lock();

    Room * room = connected_rooms[room_name];

    connected_rooms_mutex.unlock();

    json = cJSON_CreateObject();

    if (json == NULL) return JSON_ITEM_CREATE_FAIL;

    cJSON_AddItemToObject(
        json,
        "action",
        cJSON_CreateString(value ? "activate_all" : "deactivate_all")
    );

    send_state = Messager::send_json_message_wait_response(
        room->get_room_service_address(),
        room->get_room_service_port(),
        json,
        &response_json,
        true,
        true
    );

    if (is_error(send_state)) return send_state;

    if (Messager::get_response_success_value(response_json)) {
        cJSON * array = cJSON_GetObjectItem(response_json, "successful_devices");
        int success_devices_size = cJSON_GetArraySize(array);

        for (int i = 0; i < success_devices_size; i++) {
            cJSON * array_item = cJSON_GetArrayItem(array, i);
            string device_tag = array_item->valuestring;

            room->set_device_value(device_tag, value);
        }

        return SUCCESS;
    }

    error_msg = Messager::get_response_error_msg(response_json);

    cJSON_Delete(response_json);

    return ERROR;
}

state MenuActions::send_set_output_device_message(string room_name, string device_tag, string & error_msg) {
    state send_state;
    bool current_value;
    string response_msg;
    cJSON * json, * response_json;

    connected_rooms_mutex.lock();

    Room * room = connected_rooms[room_name];

    connected_rooms_mutex.unlock();

    room->get_device_value(device_tag, &current_value);

    json = cJSON_CreateObject();

    if (json == NULL) return JSON_ITEM_CREATE_FAIL;

    cJSON_AddItemToObject(
        json,
        "action",
        cJSON_CreateString(current_value ? "deactivate" : "activate")
    );

    cJSON_AddItemToObject(json, "tag", cJSON_CreateString(device_tag.c_str()));

    send_state = Messager::send_json_message_wait_response(
        room->get_room_service_address(),
        room->get_room_service_port(),
        json,
        &response_json,
        true,
        true
    );

    if (is_error(send_state)) return send_state;

    if (Messager::get_response_success_value(response_json)) {
        room->set_device_value(device_tag, !current_value);
        return SUCCESS;
    }

    error_msg = Messager::get_response_error_msg(response_json);

    cJSON_Delete(response_json);

    return ERROR;
}
