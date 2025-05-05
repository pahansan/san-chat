#pragma once

#include <condition_variable>
#include <mutex>
#include <string>

extern std::condition_variable cv_chat;

extern bool end;

void drawing();
void receiving(int client_socket);