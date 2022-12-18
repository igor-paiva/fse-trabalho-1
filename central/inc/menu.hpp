#ifndef MENU_HPP
#define MENU_HPP

#ifdef WIN_32
    #define CLEAR "cls"
#else
    #define CLEAR " clear "
#endif

#include <iostream>
#include <cstring>

#include "types.hpp"
#include "room.hpp"

using namespace std;

namespace Menu {
    void clear_screen();

    void main_menu_loop();

    void room_info_menu_loop();
}

#endif
