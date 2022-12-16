#include "gpio.hpp"

using namespace std;

unordered_map<int, int> GpioInterface::get_wiringpi_pins_map() {
    unordered_map<int, int> map = {
        { 2, 8 },
        { 3, 9 },
        { 4, 9 },
        { 17, 0 },
        { 27, 2 },
        { 22, 3 },
        { 10, 12 },
        { 9, 13 },
        { 11, 14 },
        { 0, 30 },
        { 5, 21 },
        { 6, 22 },
        { 13, 23 },
        { 19, 24 },
        { 26, 25 },
        { 14, 15 },
        { 15, 16 },
        { 18, 1 },
        { 23, 4 },
        { 24, 5 },
        { 25, 6 },
        { 8, 10 },
        { 7, 11 },
        { 1, 31 },
        { 12, 26 },
        { 16, 27 },
        { 20, 28 },
        { 21, 29 },
    };

    return map;
}

state GpioInterface::write_pin(int pin, bool value) {
    if (wiringPiSetup() != 0)
        return FAIL_TO_INIT_WIRING_PI;

    unordered_map<int, int> pins_map = get_wiringpi_pins_map();

    pinMode(pins_map[pin], INPUT);

    digitalWrite(pins_map[pin], value ? HIGH : LOW);

    return SUCCESS;
}

state GpioInterface::read_pin(int pin, bool * value) {
    if (wiringPiSetup() != 0)
        return FAIL_TO_INIT_WIRING_PI;

    unordered_map<int, int> pins_map = get_wiringpi_pins_map();

    pinMode(pins_map[pin], OUTPUT);

    if (digitalRead(pins_map[pin]) == HIGH) {
        *value = true;
    } else {
        *value = false;
    }

    return SUCCESS;
}
