#include "room.hpp"

using namespace std;

Room::Room(cJSON * json) {
    this->initialize_data(json);
}

void Room::initialize_data(cJSON * json) {
    lock_guard<mutex> lock(this->room_mutex);

    this->name = cJSON_GetObjectItem(json, "nome")->valuestring;
    this->central_server_ip = cJSON_GetObjectItem(json, "ip_servidor_central")->valuestring;
    this->central_server_port = (uint16_t) cJSON_GetObjectItem(json, "porta_servidor_central")->valueint;
    this->room_service_address = cJSON_GetObjectItem(json, "ip_servidor_distribuido")->valuestring;
    this->room_service_port = (uint16_t) cJSON_GetObjectItem(json, "porta_servidor_distribuido")->valueint;
    this->room_service_client_port = (uint16_t) cJSON_GetObjectItem(json, "porta_cliente_servidor_distribuido")->valueint;

    this->output_devices = get_devices_data(json, "outputs");
    this->input_devices = get_devices_data(json, "inputs");
}

vector<DeviceData> Room::get_devices_data(cJSON * json, string item) {
    vector<DeviceData> devices_data;

    cJSON * devices_array = cJSON_GetObjectItem(json, item.c_str());

    if (cJSON_IsArray(devices_array)) {
        int array_size = cJSON_GetArraySize(devices_array);

        for (int i = 0; i < array_size; i++) {
            cJSON * json_item = cJSON_GetArrayItem(devices_array, i);

            if (cJSON_IsObject(json_item)) {
                DeviceData device_data;

                device_data.gpio = cJSON_GetObjectItem(json_item, "gpio")->valueint;
                device_data.type = cJSON_GetObjectItem(json_item, "type")->valuestring;;
                device_data.tag = cJSON_GetObjectItem(json_item, "tag")->valuestring;
                device_data.pin_mode = item == "outputs" ? DEVICE_OUTPUT : DEVICE_INPUT;
                this->devices_values[device_data.tag] = false;

                devices_data.push_back(device_data);

                this->devices_map[device_data.tag] = device_data;
            }
        }
    }

    return devices_data;
}

string Room::to_json_str() {
    cJSON * json = this->to_json();

    string json_str = cJSON_PrintUnformatted(json);

    cJSON_Delete(json);

    return json_str;
}

cJSON * Room::to_json() {
    cJSON * json = cJSON_CreateObject();

    lock_guard<mutex> lock(this->room_mutex);

    cJSON_AddStringToObject(json, "name", this->name.c_str());
    cJSON_AddStringToObject(json, "ip_address", this->room_service_address.c_str());
    cJSON_AddNumberToObject(json, "port", (double) this->room_service_port);

    cJSON * outputs = cJSON_AddArrayToObject(json, "outputs");
    cJSON * inputs = cJSON_AddArrayToObject(json, "inputs");

    add_devices_to_json_array(outputs, this->output_devices);
    add_devices_to_json_array(inputs, this->input_devices);

    return json;
}

void Room::add_devices_to_json_array(cJSON * array, vector<DeviceData> devices) {
    for (long unsigned int i = 0; i < devices.size(); i++) {
        cJSON * item = cJSON_CreateObject();

        cJSON_AddItemToObject(item, "tag", cJSON_CreateString(devices[i].tag.c_str()));
        cJSON_AddItemToObject(item, "type", cJSON_CreateString(devices[i].type.c_str()));
        cJSON_AddItemToObject(item, "value", cJSON_CreateBool(this->devices_values[devices[i].tag] ? cJSON_True : cJSON_False));

        cJSON_AddItemToArray(array, item);
    }
}

unordered_map<string, bool> Room::get_devices_values() {
    lock_guard<mutex> lock(this->room_mutex);

    return this->devices_values;
}

state Room::get_device_value(string tag, bool * value) {
    lock_guard<mutex> lock(this->room_mutex);

    if (this->devices_values.count(tag) == 0)
        return MAP_KEY_DONT_EXISTS;


    *value = this->devices_values[tag];

    return SUCCESS;
}

void Room::set_device_value(string tag, bool new_value) {
    lock_guard<mutex> lock(this->room_mutex);

    this->devices_values[tag] = new_value;
}

string Room::get_name() {
    lock_guard<mutex> lock(this->room_mutex);

    return this->name;
};

string Room::get_central_server_ip() {
    lock_guard<mutex> lock(this->room_mutex);

    return this->central_server_ip;
};

uint16_t Room::get_central_server_port() {
    lock_guard<mutex> lock(this->room_mutex);

    return this->central_server_port;
};

string Room::get_room_service_address() {
    lock_guard<mutex> lock(this->room_mutex);

    return this->room_service_address;
};

uint16_t Room::get_room_service_port() {
    lock_guard<mutex> lock(this->room_mutex);

    return this->room_service_port;
};

uint16_t Room::get_room_service_client_port() {
    lock_guard<mutex> lock(this->room_mutex);

    return this->room_service_client_port;
};

vector<DeviceData> Room::get_output_devices() {
    lock_guard<mutex> lock(this->room_mutex);

    return this->output_devices;
};

vector<DeviceData> Room::get_input_devices() {
    lock_guard<mutex> lock(this->room_mutex);

    return this->input_devices;
};

unordered_map<string, DeviceData> Room::get_devices_map() {
    lock_guard<mutex> lock(this->room_mutex);

    return this->devices_map;
}

DeviceData Room::get_temperature_device() {
    lock_guard<mutex> lock(this->room_mutex);

    return this->temperature_device;
};
