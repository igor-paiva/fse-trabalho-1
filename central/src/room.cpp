#include "room.hpp"

Room::Room(cJSON * json) {
    this->initialize_data(json);
}

void Room::initialize_data(cJSON * json) {
    this->name = cJSON_GetObjectItem(json, "name")->valuestring;
    // TODO: the IP sent is the one on the init file, so we must use de public IP (use sockaddr_in)
    this->room_service_address = cJSON_GetObjectItem(json, "ip_address")->valuestring;
    this->room_service_port = (uint16_t) cJSON_GetObjectItem(json, "port")->valueint;

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

                device_data.type = cJSON_GetObjectItem(json_item, "type")->valuestring;;
                device_data.tag = cJSON_GetObjectItem(json_item, "tag")->valuestring;
                device_data.pin_mode = item == "outputs" ? DEVICE_OUTPUT : DEVICE_INPUT;

                this->devices_values[device_data.tag] = false;

                devices_data.push_back(device_data);
            }
        }
    }

    return devices_data;
}
