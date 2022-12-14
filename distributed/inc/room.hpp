#ifndef ROOM_HPP
#define ROOM_HPP

#include <string>
#include <vector>
#include <iostream>

#include "cJSON.h"

using namespace std;

typedef struct {
    string type;
    string tag;
    int gpio;
} DeviceData;

typedef struct {
    string name;
    string central_server_ip;
    int central_server_port;
    int room_service_port;
    vector<DeviceData> output_devices;
    vector<DeviceData> input_devices;
    DeviceData temperature_device;
} InitializationData;

class Room {

public:
    Room(cJSON * json);

    InitializationData get_initialization_data();

private:
    InitializationData initialization_data;

    void initialize_data(cJSON * json);
};

#endif
