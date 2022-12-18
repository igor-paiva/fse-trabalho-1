#include "types.hpp"

using namespace std;

bool is_error(state value) {
    return value < 0;
}

bool is_success(state value) {
    return value > 0;
}

string to_lower_case(string str) {
    string lower_str = str;

    transform(lower_str.begin(), lower_str.end(), lower_str.begin(), ::tolower);

    return lower_str;
}

string to_upper_case(string str) {
    string upper_str = str;

    transform(upper_str.begin(), upper_str.end(), upper_str.begin(), ::toupper);

    return upper_str;
}
