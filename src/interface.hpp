#pragma once

#include <mutex>
#include <string>

enum user_state { users_list, dialogue, files_list };

extern std::string current_user;

extern user_state current_state;
extern std::mutex chat_mutex;

extern std::mutex input_mutex;
extern std::string input_buffer;
extern size_t user_input_height;

extern std::string global_buffer;
extern std::string receiving_file;

void update_user_input();
void print_users(const std::string& users_string);
void print_messages(const std::string& messages);
void print_files(const std::string& messages);
void set_input_buffer(const std::string& str);
void set_input_buffer(const char& c);
std::string get_input_buffer();
std::string intrance(const int& fd, const std::string& app_name, const std::string& type, const std::string& login, const std::string& password);
user_state get_current_state();
void set_current_state(const user_state& state);
std::string get_global_buffer_substr(const size_t& start);
void set_global_buffer(const std::string& str);
std::string get_receiving_file();
void set_receiving_file(const std::string& str);
std::string get_current_user();
void set_current_user(const std::string& str);
void print_notification(const std::string& str);
