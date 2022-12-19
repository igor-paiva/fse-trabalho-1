#include "menu_actions.hpp"

using namespace std;

extern unordered_map<string, Room *> connected_rooms;
extern mutex connected_rooms_mutex;

state MenuActions::send_set_output_device_message(string room_name, string device_tag) {
    cJSON * json;
    bool current_value;

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

    return Messager::send_json_message(
        room->get_room_service_address(),
        room->get_room_service_port(),
        json
    );
}
