#ifndef ROOM_HPP
#define ROOM_HPP

#include <string>
#include <vector>
#include <iostream>
#include <mutex>
#include <unordered_map>

#include "cJSON.h"
#include "DHT22.h"
#include "types.hpp"
#include "gpio.hpp"

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
    state get_device_value(string tag, bool * value);
    unordered_map<string, bool> get_devices_values();
    void set_device_value(string tag, bool new_value);

    unordered_map<string, DeviceData> get_devices_map();

    float get_temperature();
    float get_humidity();
    void set_temperature_data(float temperature, float humidity);

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

    unordered_map<string, bool> devices_values;

    float temperature;
    float humidity;

    void initialize_data(cJSON * json);
    vector<DeviceData> get_devices_data(cJSON * json, string item);
    DeviceData get_temperature_device_data(cJSON * json, string item_key);
    void add_devices_to_json_array(cJSON * array, vector<DeviceData> devices);
};

#endif
