#include "thread_handlers.hpp"
#include "interface.hpp"
#include "message_types.hpp"
#include "parsing.hpp"
#include "sendrecv.hpp"

#include <condition_variable>
#include <mutex>
#include <string>

std::condition_variable cv_chat;

bool end = false;

void drawing()
{
    for (;;) {
        std::unique_lock lock(chat_mutex);
        cv_chat.wait(lock);
        lock.unlock();
        if (end)
            break;

        if (get_global_buffer_substr(0)[0] == messages) {
            {
                if (get_current_state() == dialogue && is_current_user(get_global_buffer_substr(1))) {
                    print_messages(get_global_buffer_substr(1));
                    update_user_input();
                }
                if (get_current_state() == files_list && is_current_user(get_global_buffer_substr(1))) {
                    print_files(get_global_buffer_substr(1));
                    update_user_input();
                }
            }
        } else if (get_global_buffer_substr(0)[0] == users) {
            {
                if (get_current_state() == users_list) {
                    print_users(get_global_buffer_substr(1));
                    update_user_input();
                }
            }
        }
    }
}

void receiving(int client_socket)
{
    std::string received;
    size_t bytes_received;
    while (true) {
        bytes_received = my_recv(client_socket, received);
        if (bytes_received <= 0 && !end) {
            std::cerr << "Сервер не отвечает\n";
            end = true;
        }
        if (end)
            break;

        if (received == file_not_found) {
            print_notification("File not found");
            continue;
        } else if (received == login_dont_exists) {
            print_notification("Такого пользователя не существует");
            set_current_state(users_list);
        }

        if (received[0] == file) {
            print_notification("Receiving...");
            {
                std::lock_guard lock(chat_mutex);
                recv_file(client_socket, receiving_file);
            }
            update_user_input();
        }

        set_global_buffer(received);

        cv_chat.notify_one();
    }
}
