#pragma once

#include <iostream>
#include <mutex>
#include <sqlite3.h>
#include <string>
#include <unordered_map>
#include <vector>

#define OFFLINE 0
#define ONLINE 1

typedef struct message {
    std::string time;
    std::string sender;
    std::string text;
    int is_file;
} message;

using messages_list = std::vector<message>;
using users_map = std::unordered_map<std::string, int>;

extern std::mutex db_mutex;

int init_database();
bool user_exists(const std::string& login);
int add_user(const std::string& login, const std::string& password);
int get_user_id(const std::string& login);
int add_message(const std::string& sender_login, const std::string& receiver_login, const std::string& message);
int add_file(const std::string& sender_login, const std::string& receiver_login, const std::string& path);
bool verify_user(const std::string& login, const std::string& password);
int change_user_status(const std::string& login, int status);
int get_user_status(const std::string& login);
users_map get_users_map();
messages_list get_messages_list(const std::string& sender_login, const std::string& receiver_login);