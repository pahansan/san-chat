#pragma once

#include <condition_variable>
#include <mutex>
#include <string>

enum user_state { users_list, dialogue, files_list };

extern user_state current_state;

extern std::mutex chat_mutex;
extern std::condition_variable cv_chat;
extern std::string global_buffer;

extern std::string receiving_file;

extern bool end;

void drawing();
void receiving(int client_socket);