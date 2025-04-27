#pragma once

#include <string>

enum message_type { registration = 1, login, get_messages, send_message, users, messages };

extern std::string login_exists;
extern std::string db_fault;
extern std::string login_dont_exists;
extern std::string incorrect_password;