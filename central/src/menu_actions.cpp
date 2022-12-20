#include "menu_actions.hpp"

using namespace std;

extern unordered_map<string, Room *> connected_rooms;
extern mutex connected_rooms_mutex;

extern string log_file_path;
extern bool alarm_system;

void MenuActions::turn_on_alarm() {
    Logger::add_action_to_log_file(
        log_file_path.c_str(),
        "ligar alarme",
        "liga o sistema de alarme",
        "-",
        "-"
    );

    alarm_system = true;
}

state MenuActions::turn_off_alarm(string & error_msg) {
    Logger::add_action_to_log_file(
        log_file_path.c_str(),
        "desligar alarme",
        "desliga o sistema de alarme",
        "-",
        "-"
    );

    bool has_alarm_trigger_activated = false;
    vector<string> activated_devices;

    connected_rooms_mutex.lock();

    for (auto& [key, room] : connected_rooms) {
        for (auto data : room->get_input_devices()) {
            bool device_value;

            if (data.type == "presenca" || data.type == "janela" || data.type == "porta") {
                room->get_device_value(data.tag, &device_value);

                if (device_value) {
                    activated_devices.push_back(data.tag + " (" + key + ") ");

                    has_alarm_trigger_activated = true;
                    break;
                }
            }
        }
    }

    connected_rooms_mutex.unlock();

    if (has_alarm_trigger_activated) {
        int i = 1;
        error_msg = "Não possível desativar o alarme pois os seguintes dispositivos ainda estão ativos: ";

        for (auto activated_device : activated_devices) {
            if (i == 1 || i == (int) activated_devices.size()) {
                error_msg += activated_device;
            } else {
                error_msg = error_msg + ", " + activated_device;
            }

            i++;
        }
    } else {
        cJSON * json = cJSON_CreateObject();

        cJSON_AddItemToObject(json, "action", cJSON_CreateString("turn_off_buzzer"));

        connected_rooms_mutex.lock();

        for (auto& [key, room] : connected_rooms) {
            for (int i = 0; i < 3; i++) {
                state send_state = Messager::send_async_json_message(
                    room->get_room_service_address(),
                    room->get_room_service_port(),
                    json,
                    false
                );

                if (is_success(send_state)) break;

                this_thread::sleep_for(50ms);
            }
        }

        connected_rooms_mutex.unlock();

        cJSON_Delete(json);

        alarm_system = false;
    }

    return SUCCESS;
}

state MenuActions::send_set_all_output_device_message(string room_name, bool value, string & error_msg) {
    state send_state;
    string response_msg;
    cJSON * json, * response_json;

    Logger::add_action_to_log_file(
        log_file_path.c_str(),
        (value ? "acionar todos" : "desligar todos"),
        "mudar valor de todos os dispositivo de saída",
        room_name,
        "-"
    );

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

    Logger::add_action_to_log_file(
        log_file_path.c_str(),
        (current_value ? "desligar" : "acionar"),
        "mudar valor de dispositivo de saída",
        room_name,
        device_tag
    );

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
