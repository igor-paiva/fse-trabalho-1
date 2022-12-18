#ifndef ROOM_HPP
#define ROOM_HPP

#include <string>
#include <vector>
#include <iostream>
#include <unordered_map>

#include "cJSON.h"

using namespace std;

typedef enum {
    DEVICE_OUTPUT = 1,
    DEVICE_INPUT,
} pin_mode_t;

typedef struct {
    string type;
    string tag;
    int gpio;
    pin_mode_t pin_mode;
} DeviceData;

class Room {

public:
    Room(cJSON * json);

private:
    string name;
    string central_server_ip;
    uint16_t central_server_port;
    string room_service_address;
    uint16_t room_service_port;
    uint16_t room_service_client_port;
    vector<DeviceData> output_devices;
    vector<DeviceData> input_devices;
    DeviceData temperature_device;

    unordered_map<string, bool> devices_values;

    void initialize_data(cJSON * json);
    vector<DeviceData> get_devices_data(cJSON * json, string item);

};

#endif
