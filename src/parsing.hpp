#pragma once

#include "db.hpp"

#include <string>
#include <vector>

enum prevent { space, no_space };

extern std::string current_user;

std::vector<std::string> fit_text(const std::string& text, const int& width);
users_map parse_users(const std::string& str);
std::vector<std::string> parse_messages(const std::string& messages, const int& width);
std::vector<std::string> parse_files(const std::string& messages, const int& width);
std::string skip_spaces(const std::string& str);
std::string get_file_name(const std::string& path);
bool is_current_user(const std::string& str);
