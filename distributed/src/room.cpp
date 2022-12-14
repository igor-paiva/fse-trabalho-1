#include "room.hpp"

using namespace std;

Room::Room(cJSON * json) {
    this->initialize_data(json);
}

InitializationData Room::get_initialization_data() {
    return this->initialization_data;
}

void Room::initialize_data(cJSON * json) {
    char * json_str = cJSON_Print(json);

    cout << json_str << endl;

    free(json_str);
}
