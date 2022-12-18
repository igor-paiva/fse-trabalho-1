#include "menu.hpp"

using namespace std;

void Menu::print_menu_options() {
    cout << "\n1) Fazer alguma coisa" << endl;
    cout << "2) Sair" << endl;
}

int Menu::read_menu_option() {
    int option;

    cout << "\nDigite uma opção: ";

    cin >> option;

    return option;
}

bool Menu::trigger_main_menu_action(int option) {
    switch(option) {
        case 1:
            cout << "Fazendo alguma coisa" << endl;
            break;
        case 2:
            // TODO: do a gracefully shutdown
            cout << "\nSaindo..." << endl;
            exit(0);
            break;
        default:
            cout << "Opção inválida" << endl;

            return false;
    }

    return true;
}


void Menu::main_menu_loop() {
    while (true) {
        int option;
        bool action_res;

        print_menu_options();

        option = read_menu_option();

        action_res = trigger_main_menu_action(option);

        if (action_res) {
            cout << "Do something" << endl;
        }
    }
}
