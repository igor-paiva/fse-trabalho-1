#ifndef LOGGER_HPP
#define LOGGER_HPP

#include <iostream>
#include <fstream>

#include "types.hpp"

#define MAX_FILE_NAME_SIZE 20
#define MAX_DATETIME_STRING_SIZE 27
#define CSV_FILE_DELIMITER ";"
#define LOG_FOLDER "app_data/logs"
#define LOG_FILE_COLUMNS "acao;descricao;sala;dispositivo;data_hora"

using namespace std;

namespace Logger {
    state create_log_file(string & filepath);

    bool log_file_exists(const char * file_path);

    state add_action_to_log_file(const char * file_path, string action, string description, string room_name, string device_tag);
}

#endif
