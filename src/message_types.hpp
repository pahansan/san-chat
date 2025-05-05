#pragma once

#include <string>

enum message_type { registration = 1, logining, get_messages, send_message, users, messages, get_users, get_file, file };

extern std::string login_exists;
extern std::string db_fault;
extern std::string login_dont_exists;
extern std::string incorrect_password;
extern std::string file_not_found;
extern std::string user_already_connected;