#pragma once

#include <algorithm>
#include <arpa/inet.h>
#include <condition_variable>
#include <map>
#include <mutex>
#include <queue>
#include <string>
#include <vector>

#include "db.hpp"

extern std::vector<int> fd_list;
extern std::vector<std::string> login_list;

extern std::mutex server_mutex;
extern std::mutex event_mutex;
extern std::condition_variable event_cv;

enum event_type { send_status, send_message_list, send_file, to_one, to_all };

typedef struct event {
    event_type type;
    int subtype;
    std::string sender_login;
    std::string receiver_login;
} event;

extern std::queue<event> event_queue;

void erase_user(const int& fd);
void send_status_to_all();
void send_status_to_one(const std::string& login);
void client_send_message_list(const std::string& sender, const std::string& receiver);
std::string get_login_by_fd(const int& fd);
void process_event(const event& cur);
void handler();