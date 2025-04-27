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

extern std::mutex event_mutex;
extern std::condition_variable event_cv;

enum event_type { change_status, send_message };

typedef struct event {
    event_type type;
    int sender_fd;
    int receiver_fd;
} event;

extern std::queue<event> event_queue;

void client_changed_status();
void client_send_message(int sender_fd, int receiver_fd);
std::string get_login_by_fd(const int& fd);
void process_event(const event& cur);
void handler();