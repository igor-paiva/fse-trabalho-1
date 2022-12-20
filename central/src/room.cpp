#include "room.hpp"

Room::Room(cJSON * json, char * client_addr) {
    this->initialize_data(json, client_addr);
}

void Room::initialize_data(cJSON * json, char * client_addr) {
    lock_guard<mutex> lock(this->room_mutex);

    this->name = cJSON_GetObjectItem(json, "name")->valuestring;
    // TODO: the IP sent is the one on the init file, so we must use de public IP (use sockaddr_in)
    this->room_service_address = client_addr;
    this->room_service_port = (uint16_t) cJSON_GetObjectItem(json, "port")->valueint;
    // this->room_service_address = cJSON_GetObjectItem(json, "ip_address")->valuestring;

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

string Room::to_string() {
    lock_guard<mutex> lock(this->room_mutex);

    ostringstream os;

    os << this->name << endl;

    os << "\tSaídas:" << endl;

    for (auto device_data : this->output_devices) {
        os << "\n\t\t" << device_data.tag << endl;
        os << "\t\t\t" << "Tipo: " << device_data.type << endl;
        os << "\t\t\t" << "Estado: " << (this->devices_values[device_data.tag] ? "Ligado" : "Desligado") << endl;
    }

    os << "\tEntradas:" << endl;

    for (auto device_data : this->input_devices) {
        if (device_data.type == "contagem") continue;

        os << "\n\t\t" << device_data.tag << endl;
        os << "\t\t\t" << "Tipo: " << device_data.type << endl;
        os << "\t\t\t" << "Estado: " << (this->devices_values[device_data.tag] ? "Ligado" : "Desligado") << endl;
    }

    return os.str();
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

string Room::get_room_service_address() {
    lock_guard<mutex> lock(this->room_mutex);

    return this->room_service_address;
};

uint16_t Room::get_room_service_port() {
    lock_guard<mutex> lock(this->room_mutex);

    return this->room_service_port;
};

vector<DeviceData> Room::get_output_devices() {
    lock_guard<mutex> lock(this->room_mutex);

    return this->output_devices;
};

vector<DeviceData> Room::get_input_devices() {
    lock_guard<mutex> lock(this->room_mutex);

    return this->input_devices;
};
