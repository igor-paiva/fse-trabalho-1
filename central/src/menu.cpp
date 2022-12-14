#include "menu.hpp"

using namespace std;

extern unordered_map<string, Room *> connected_rooms;
extern mutex connected_rooms_mutex;
extern bool alarm_system;

void print_rooms_info_summary() {
    cout << "\nAlarme: " << (alarm_system ? "ligado" : "desligado") << endl;

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
    cout << "4) " << (alarm_system ? "Desligar" : "Ligar") << " sistema de alarme" << endl;
    cout << "5) Sair" << endl;
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
    cout << "2) Acionar todos os equipamentos de saída por sala" << endl;
    cout << "3) Desligar todos os equipamentos de saída por sala" << endl;
    cout << "4) Ligar todos os equipamentos de saída no prédio" << endl;
    cout << "5) Desligar todos os equipamentos de saída no prédio" << endl;
    cout << "6) Voltar" << endl;}

void turn_on_off_alarm_system() {
    if (!alarm_system) {
        MenuActions::turn_on_alarm();
    } else {
        string message = "";

        MenuActions::turn_off_alarm(message);

        if (message.length() > 0)
            cout << "\n\n" << message << "\n" << endl;
    }
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
            turn_on_off_alarm_system();
            break;
        case 5:
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
        bool current_value;

        connected_rooms[room_name]->get_device_value(data.tag, &current_value);

        if (data.type != "alarme") {
            cout << i << ") " << data.tag << " (" << (current_value ? "ligado" : "desligado") << ")" << endl;

            i++;
        }
    }

    cout << i << ") Voltar" << endl;

    return i;
}

state get_room_tag_loop(string room_name, state (*callback)(string, string, string&), string & error_msg) {
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
            return callback(room_name, tag, error_msg);
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
    string error_msg;

    state send_state = get_room_tag_loop(
        room->get_name(),
        MenuActions::send_set_output_device_message,
        error_msg
    );

    if (is_success(send_state)) {
        cout << "\nDispositivo acionado com sucesso" << endl;
    } else {
        cout << "Falha ao acionar o dispositivo: " << send_state << ". " << error_msg << endl;
    }
}

void call_action_to_room(Room * room, bool value) {
    string error_msg;

    state send_state = MenuActions::send_set_all_output_device_message(
        room->get_name(),
        value,
        error_msg
    );

    if (is_success(send_state)) {
        cout << "\nDispositivos " << (value ? "acionados" : "desligados") << " com sucesso" << endl;
    } else {
        cout << "Falha ao acionar os dispositivos: " << error_msg << endl;
    }
}

void set_all_devices_all_rooms(bool value) {
    string error_msg;

    state send_state = MenuActions::send_set_all_output_device_message_all_rooms(value, error_msg);

    if (is_success(send_state)) {
        cout << "\nDispositivos " << (value ? "acionados" : "desligados") << " com sucesso" << endl;
    } else {
        cout << "Falha ao acionar o(s) dispositivo(s).\n\n" << error_msg << endl;
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
            get_room_name_loop([] (Room * room) { call_action_to_room(room, true); });
            action_res = true;
        } else if (option == 3) {
            get_room_name_loop([] (Room * room) { call_action_to_room(room, false); });
            action_res = true;
        } else if (option == 4) {
            set_all_devices_all_rooms(true);
        } else if (option == 5) {
            set_all_devices_all_rooms(false);
        } else if (option == 6) {
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
