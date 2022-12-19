#include "menu.hpp"

using namespace std;

extern unordered_map<string, Room *> connected_rooms;
extern mutex connected_rooms_mutex;

void print_rooms_info_summary() {
    lock_guard<mutex> lock(connected_rooms_mutex);

    cout << "\nSalas conectadas: " << connected_rooms.size() << endl;

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
    cout << "3) Ações nas salas" << endl;
    cout << "4) Sair" << endl;
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

void print_actions_menu_options() {
    cout << "\n\n1) Acionar equipamentos de saída" << endl;
    cout << "2) Voltar" << endl;
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
            Menu::actions_menu_loop();
            break;
        case 4:
            // TODO: do a gracefully shutdown
            cout << "\nSaindo..." << endl;
            exit(0);

            break;
        default:
            return false;
    }

    return true;
}

int print_room_info_menu_options() {
    lock_guard<mutex> lock(connected_rooms_mutex);
    int i = 1;

    cout << "\nSalas conectadas:\n" << endl;

    for (auto& [key, _]: connected_rooms) {
        cout << i << ") " << key << endl;

        i++;
    }

    cout << i << ") Voltar" << endl;

    return i;
}

int print_room_tags_menu_options(string room_name) {
    lock_guard<mutex> lock(connected_rooms_mutex);
    int i = 1;

    cout << "\nDispositivos conectados:\n" << endl;

    for (auto data: connected_rooms[room_name]->get_output_devices()) {
        cout << i << ") " << data.tag << endl;

        i++;
    }

    cout << i << ") Voltar" << endl;

    return i;
}

state get_room_tag_loop(string room_name, state (*callback)(string, string)) {
    int option = -1;
    string tag = "";

    while (tag.length() == 0) {
        if (option != -1 && tag.length() == 0)
            cout << "Opção inválida! Digite uma das opções listadas\n" << endl;

        int back_option = print_room_tags_menu_options(room_name);

        option = read_menu_option();

        if (option == back_option) {
            Menu::clear_screen();
            break;
        }

        connected_rooms_mutex.lock();

        vector<DeviceData> output_devices = connected_rooms[room_name]->get_output_devices();

        if (connected_rooms.count(room_name) == 1 && option > 0 && option <= (int) output_devices.size()) {
            tag = output_devices[option - 1].tag;
        }

        connected_rooms_mutex.unlock();

        if (tag.length() > 0) {
            return callback(room_name, tag);
        } else {
            Menu::clear_screen();
        }
    }

    return SUCCESS;
}

void get_room_name_loop(void (*callback)(Room *)) {
    string option = "";
    Room * room = NULL;

    while (!room) {
        if (option != "" && !room)
            cout << "Opção inválida! Digite uma das opções listadas\n" << endl;

        int back_option = print_room_info_menu_options();

        option = read_menu_string_option("Digite o nome da sala");

        if (to_upper_case(option) == "VOLTAR" || option == to_string(back_option)) {
            Menu::clear_screen();
            break;
        }

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

void call_action_to_tag(Room * room) {
    state send_state = get_room_tag_loop(room->get_name(), MenuActions::send_set_output_device_message);

    if (is_success(send_state)) {
        cout << "\nDispositivo acionado com sucesso" << endl;
    } else {
        cout << "Falha ao acionar o dispositivo " << ". Tente novamente" << endl;
    }
}

void Menu::clear_screen() {
    system(CLEAR);
}

void Menu::actions_menu_loop() {
    int option = -1;
    bool action_res = false;

    while (true) {
        if (option != -1 && !action_res)
            cout << "Opção inválida! Digite uma das opções listadas\n" << endl;

        print_rooms_info_summary();

        print_actions_menu_options();

        option = read_menu_option();

        if (option == 1) {
            get_room_name_loop(call_action_to_tag);
            action_res = true;
        } else if (option == 2) {
            clear_screen();
            break;
        }

        if (!action_res) clear_screen();
    }
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
