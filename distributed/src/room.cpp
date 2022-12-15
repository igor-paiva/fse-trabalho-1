#include "room.hpp"

using namespace std;

Room::Room(cJSON * json) {
    this->initialize_data(json);
}

void Room::initialize_data(cJSON * json) {
    this->name = cJSON_GetObjectItem(json, "nome")->valuestring;
    this->central_server_ip = cJSON_GetObjectItem(json, "ip_servidor_central")->valuestring;
    this->central_server_port = cJSON_GetObjectItem(json, "porta_servidor_central")->valueint;
    this->room_service_address = cJSON_GetObjectItem(json, "ip_servidor_distribuido")->valuestring;
    this->room_service_port = cJSON_GetObjectItem(json, "porta_servidor_distribuido")->valueint;

    this->output_devices = get_devices_data(json, "outputs");
    this->input_devices = get_devices_data(json, "inputs");
}

vector<DeviceData> Room::get_devices_data(cJSON * json, string item) {
    vector<DeviceData> devices_data;

    cJSON * devices_array = cJSON_GetObjectItem(json, item.c_str());

    if (cJSON_IsArray(devices_array)) {
        int array_size = cJSON_GetArraySize(devices_array);

        for (int i = 0; i < array_size; i++) {
            cJSON * item = cJSON_GetArrayItem(devices_array, i);

            if (cJSON_IsObject(item)) {
                DeviceData device_data;

                device_data.gpio = cJSON_GetObjectItem(item, "gpio")->valueint;
                device_data.type = cJSON_GetObjectItem(item, "type")->valuestring;;
                device_data.tag = cJSON_GetObjectItem(item, "tag")->valuestring;

                devices_data.push_back(device_data);
            }
        }
    }

    return devices_data;
}

char * Room::to_json_str() {
    return cJSON_PrintUnformatted(this->to_json());
}

cJSON * Room::to_json() {
    cJSON * json = cJSON_CreateObject();

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
        cJSON_AddItemToObject(item, "value", cJSON_CreateBool(devices[i].value ? cJSON_True : cJSON_False));

        cJSON_AddItemToArray(array, item);
    }

}

string Room::get_name() {
    return this->name;
};

string Room::get_central_server_ip() {
    return this->central_server_ip;
};

int Room::get_central_server_port() {
    return this->central_server_port;
};

string Room::get_room_service_address() {
    return this->room_service_address;
};

int Room::get_room_service_port() {
    return this->room_service_port;
};

vector<DeviceData> Room::get_output_devices() {
    return this->output_devices;
};

vector<DeviceData> Room::get_input_devices() {
    return this->input_devices;
};

DeviceData Room::get_temperature_device() {
    return this->temperature_device;
};
