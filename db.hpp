#pragma once

#include <sqlite3.h>
#include <iostream>
#include <mutex>
#include <string>

std::mutex db_mutex;

int init_database();
bool user_exists(const std::string &login);
int add_user(const std::string &login, const std::string &password);
int get_user_id(const std::string &login);
int add_message(const std::string &sender_login, const std::string &reciever_login, const std::string &message);
