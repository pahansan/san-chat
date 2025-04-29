#pragma once

#include <string>

enum message_type { registration = 1, logining, get_messages, send_message, users, messages, get_users };

extern std::string login_exists;
extern std::string db_fault;
extern std::string login_dont_exists;
extern std::string incorrect_password;