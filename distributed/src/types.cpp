#include "types.hpp"

using namespace std;

bool is_error(state value) {
    return value < 0;
}

bool is_success(state value) {
    return value > 0;
}

string char_pointer_to_string(char * str) {
    ostringstream os;

    os << str;

    return os.str();
}
