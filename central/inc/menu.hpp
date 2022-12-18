#ifndef MENU_HPP
#define MENU_HPP

#include <iostream>

#include "types.hpp"

using namespace std;

namespace Menu {
    void print_menu_options();

    int read_menu_option();

    bool trigger_main_menu_action(int option);

    void main_menu_loop();
}

#endif
