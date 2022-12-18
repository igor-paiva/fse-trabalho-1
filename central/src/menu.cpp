#include "menu.hpp"

using namespace std;

extern unordered_map<string, Room *> connected_rooms;
extern mutex connected_rooms_mutex;

void print_rooms_info_summary() {
    lock_guard<mutex> lock(connected_rooms_mutex);

    cout << "Salas conectadas: " << connected_rooms.size() << endl;

    for (auto& [key, _]: connected_rooms) {
        cout << "\t" << key << endl;
    }
}

void print_rooms_info() {
    lock_guard<mutex> lock(connected_rooms_mutex);

    for (auto& [key, _]: connected_rooms) {
        cout << endl << connected_rooms[key]->to_string() << endl << endl;
    }
}

void print_room_info(Room * room) {
    if (room == NULL) return;

    cout << endl << room->to_string() << endl << endl;
}

void print_main_menu_options() {
    cout << "\n\n1) Listar detalhes todas as salas" << endl;
    cout << "2) Listar detalhes de uma sala" << endl;
    cout << "3) Sair" << endl;
}

int read_menu_option() {
    string option;

    cout << "\nDigite uma opção: ";

    cin >> option;

    return atoi(option.c_str());
}

string read_menu_string_option(string message = "Digite uma opção") {
    string option;

    cout << "\n" << message << ": ";

    getline(cin, option);

    return option;
}

bool trigger_main_menu_action(int option) {
    switch(option) {
        case 1:
            print_rooms_info();
            break;
        case 2:
            Menu::clear_screen();
            Menu::room_info_menu_loop();
            break;
        case 3:
            // TODO: do a gracefully shutdown
            cout << "\nSaindo..." << endl;
            exit(0);

            break;
        default:
            return false;
    }

    return true;
}

void print_room_info_menu_options() {
    lock_guard<mutex> lock(connected_rooms_mutex);
    int i = 1;

    for (auto& [key, _]: connected_rooms) {
        cout << i << ") " << key << endl;

        i++;
    }
}

void get_room_name_loop(void (*callback)(Room *)) {
    string option = "";
    Room * room = NULL;

    while (!room) {
        if (option != "" && !room)
            cout << "Opção inválida! Digite uma das opções listadas\n" << endl;

        print_room_info_menu_options();

        option = read_menu_string_option("Digite o nome da sala");

        connected_rooms_mutex.lock();

        if (option.length() > 0 && connected_rooms.count(option) == 1) {
            room = connected_rooms[option];
        }

        connected_rooms_mutex.unlock();

        if (room) {
            callback(room);
            break;
        } else {
            Menu::clear_screen();
        }
    }
}

void Menu::clear_screen() {
    system(CLEAR);
}

void Menu::room_info_menu_loop() {
    get_room_name_loop(print_room_info);
}

void Menu::main_menu_loop() {
    while (true) {
        int option = -1;
        bool action_res = false;

        while (!action_res) {
            if (option != -1 && !action_res)
                cout << "Opção inválida! Digite uma das opções listadas\n" << endl;

            print_rooms_info_summary();

            print_main_menu_options();

            option = read_menu_option();

            action_res = trigger_main_menu_action(option);

            if (!action_res) clear_screen();
        }
    }
}
