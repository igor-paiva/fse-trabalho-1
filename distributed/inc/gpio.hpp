#ifndef GPIO_HPP
#define GPIO_HPP

#include <string>
#include <iostream>
#include <unordered_map>
#include <wiringPi.h>

#include "types.hpp"

using namespace std;

namespace GpioInterface {
    unordered_map<int, int> get_wiringpi_pins_map();

    state write_pin(int pin, bool value);

    state read_pin(int pin, bool * value);
}

#endif
