#ifndef ROOM_HPP
#define ROOM_HPP

#include <string>
#include <vector>
#include <iostream>
#include <mutex>
#include <unordered_map>

#include "cJSON.h"

using namespace std;

typedef enum {
    OUTPUT = 1,
    INPUT,
} pin_mode_t;

typedef struct {
    string type;
    string tag;
    int gpio;
    bool value;
    pin_mode_t pin_mode;
} DeviceData;

class Room {

public:
    Room(cJSON * json);

    cJSON * to_json();
    string to_json_str();

    string get_name();
    string get_central_server_ip();
    uint16_t get_central_server_port();
    string get_room_service_address();
    uint16_t get_room_service_port();
    uint16_t get_room_service_client_port();

    vector<DeviceData> get_output_devices();
    vector<DeviceData> get_input_devices();
    DeviceData get_temperature_device();

    unordered_map<string, DeviceData> get_devices_map();

private:
    mutex room_mutex;

    string name;
    string central_server_ip;
    uint16_t central_server_port;
    string room_service_address;
    uint16_t room_service_port;
    uint16_t room_service_client_port;
    vector<DeviceData> output_devices;
    vector<DeviceData> input_devices;
    DeviceData temperature_device;
    unordered_map<string, DeviceData> devices_map;

    void initialize_data(cJSON * json);
    vector<DeviceData> get_devices_data(cJSON * json, string item);
    void add_devices_to_json_array(cJSON * array, vector<DeviceData> devices);
};

#endif
