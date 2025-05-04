#include "thread_handlers.hpp"
#include "interface.hpp"
#include "message_types.hpp"
#include "parsing.hpp"
#include "sendrecv.hpp"

#include <condition_variable>
#include <mutex>
#include <string>

user_state current_state = users_list;

std::mutex chat_mutex;
std::condition_variable cv_chat;
std::string global_buffer;
std::string receiving_file = "";

bool end = false;

void drawing()
{
    for (;;) {
        std::unique_lock lock(chat_mutex);
        cv_chat.wait(lock);
        lock.unlock();
        if (end)
            break;

        if (global_buffer[0] == messages) {
            {
                std::lock_guard lock(chat_mutex);
                if (current_state == dialogue && is_current_user(&global_buffer[1])) {
                    print_messages(&global_buffer[1]);
                    update_user_input();
                }
                if (current_state == files_list && is_current_user(&global_buffer[1])) {
                    print_files(&global_buffer[1]);
                    update_user_input();
                }
            }
        } else if (global_buffer[0] == users) {
            {
                std::lock_guard lock(chat_mutex);
                if (current_state == users_list) {
                    print_users(&global_buffer[1]);
                    update_user_input();
                }
            }
        }
    }
}

void receiving(int client_socket)
{
    std::string received;
    while (true) {
        size_t bytes_received = my_recv(client_socket, received);
        if (bytes_received < 0 && !end) {
            std::cerr << "Сервер не отвечает\n";
            end = true;
        }
        if (end)
            break;

        if (received == file_not_found) {
            std::lock_guard lk(input_mutex);
            std::cout << "File not found";
            std::cout << input_buffer;
            std::cout.flush();
            continue;
        }

        if (received[0] == file) {
            recv_file(client_socket, receiving_file);
        }

        {
            std::lock_guard lock(chat_mutex);
            global_buffer = received;
        }
        cv_chat.notify_one();
    }
}
