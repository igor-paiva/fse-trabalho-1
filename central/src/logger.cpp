#include "logger.hpp"

state Logger::create_log_file(string & filepath) {
    char * file_path;
    ofstream log_file;

    time_t t = time(NULL);
    struct tm tm = *localtime(&t);

    file_path = (char *) malloc ((MAX_FILE_NAME_SIZE + strlen(LOG_FOLDER)) * sizeof(char));

    if (file_path == NULL) return ALLOC_FAIL;

    sprintf(
        file_path,
        "%s/%02d_%02d_%d-%02d_%02d.csv",
        LOG_FOLDER,
        tm.tm_mon + 1,
        tm.tm_mday,
        tm.tm_year + 1900,
        tm.tm_hour,
        tm.tm_min
    );

    filepath = file_path;

    if (log_file_exists(file_path)) return FILE_ALREADY_EXISTS;

    log_file.open(file_path, ios::trunc);

    log_file << LOG_FILE_COLUMNS << "\n";

    log_file.close();

    return SUCCESS;
}

bool Logger::log_file_exists(const char * file_path) {
    ofstream log_file;

    log_file.open(file_path, ios::in);

    if (log_file) return true;

    log_file.close();

    return false;
}

state Logger::add_action_to_log_file(const char * file_path, string action, string description, string room_name, string device_tag) {
    if (!log_file_exists(file_path))
        return FILE_DOES_NOT_EXISTS;

    ofstream log_file;
    time_t t = time(NULL);
    struct tm tm = *localtime(&t);

    char * datetime = (char *) malloc (MAX_DATETIME_STRING_SIZE * sizeof(char));

    if (datetime == NULL) return ALLOC_FAIL;

    sprintf(
        datetime,
        "%02d/%02d/%d  %02d:%02d:%02d %ld GMT",
        tm.tm_mon + 1,
        tm.tm_mday,
        tm.tm_year + 1900,
        tm.tm_hour,
        tm.tm_min,
        tm.tm_sec,
        tm.tm_gmtoff / 3600
    );

    log_file.open(file_path, ios::app);

    if (!log_file) return UNABLE_OPEN_FILE;

    log_file << action << CSV_FILE_DELIMITER << description << CSV_FILE_DELIMITER <<
        room_name << CSV_FILE_DELIMITER << device_tag << CSV_FILE_DELIMITER << datetime << "\n";

    log_file.close();

    return SUCCESS;
}
