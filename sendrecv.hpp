#pragma once

#include <string>

ssize_t my_send(const int& fd, const std::string& data);
ssize_t my_recv(const int& fd, std::string& data);
ssize_t send_file(const int& fd, const std::string& filename);
ssize_t recv_file(const int& fd, const std::string& filename);