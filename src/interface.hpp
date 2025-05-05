#pragma once

#include <mutex>
#include <string>

extern std::mutex input_mutex;
extern std::string input_buffer;
extern size_t user_input_height;

void update_user_input();
void print_users(const std::string& users_string);
void print_messages(const std::string& messages);
void print_files(const std::string& messages);
void set_input_buffer(const std::string& str);
void set_input_buffer(const char& c);
std::string get_input_buffer();
