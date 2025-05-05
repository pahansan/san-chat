#include "db.hpp"
#include "interface.hpp"
#include "message_types.hpp"
#include "net.hpp"
#include "parsing.hpp"
#include "sendrecv.hpp"
#include "terminal.hpp"
#include "thread_handlers.hpp"
#include "utf8_string.hpp"

#include <cstring>
#include <filesystem>
#include <format>
#include <netdb.h>
#include <signal.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <thread>

int main(int argc, char* argv[])
{
    if (argc != 5) {
        std::cout << std::format("Usage: {} hostname <reg/log> <login> <password>\n", argv[0]);
        return 1;
    }

    int client_socket = connect_to_server(argv[1]);

    if (client_socket < 0)
        return 1;

    signal(SIGPIPE, SIG_IGN);

    std::string received = intrance(client_socket, argv[0], argv[2], argv[3], argv[4]);
    if (received == "") {
        return 1;
    }

    std::string login(argv[3]);

    print_users(received.substr(1));
    update_user_input();

    std::thread drawing_thread(drawing);
    std::thread receiving_thread(receiving, client_socket);

    std::string filepath;

    enableRawMode();

    while (true) {
        struct winsize w;
        ioctl(STDOUT_FILENO, TIOCGWINSZ, &w);

        if (end)
            break;
        char c;
        if (read(STDIN_FILENO, &c, 1) != 1)
            break;

        if (c == '\n') {
            if (get_input_buffer() == "Cannot open file") {
                set_input_buffer("");
            } else if (get_input_buffer() == "Wrong username") {
                set_input_buffer("");
            }

            std::string message;
            {
                message = get_input_buffer();
                set_input_buffer("");
            }

            if (!message.empty()) {
                std::string sending;
                bool is_command = false;

                {
                    std::lock_guard lock(chat_mutex);

                    if (message == "/users") {
                        current_state = users_list;
                        current_user = "";
                        sending = get_users;
                        is_command = true;
                    } else if (message.substr(0, 7) == "/select" && current_state == users_list) {
                        sending = get_messages;
                        sending += skip_spaces(message.substr(7));
                        current_state = dialogue;
                        current_user = sending.substr(1);
                        is_command = true;
                    } else if (message.substr(0, 6) == "/files" && current_state == dialogue) {
                        sending = get_messages;
                        sending += current_user;
                        current_state = files_list;
                        is_command = true;
                    } else if (message.substr(0, 9) == "/messages" && current_state == files_list) {
                        sending = get_messages;
                        sending += current_user;
                        current_state = dialogue;
                        is_command = true;
                    } else if (message.substr(0, 5) == "/send" && (current_state == files_list || current_state == dialogue)) {
                        filepath = skip_spaces(message.substr(5));
                        if (!std::filesystem::exists(filepath)) {
                            set_input_buffer("Cannot open file");
                        } else {
                            sending = file;
                            sending += get_file_name(filepath);
                        }
                    } else if (message.substr(0, 4) == "/get" && (current_state == files_list || current_state == dialogue)) {
                        receiving_file = skip_spaces(message.substr(4));
                        receiving_file = receiving_file.substr(0, receiving_file.find_first_of(' '));
                        std::string username = message.substr(message.find_last_of(' ') + 1);
                        if (username == login || username == current_user) {
                            sending = get_file;
                            sending += receiving_file + '\036' + username;
                            is_command = false;
                        } else {
                            set_input_buffer("Wrong username");
                        }
                    } else if (message == "/exit") {
                        end = true;
                        break;
                    } else if (current_state == dialogue) {
                        sending = std::string(1, send_message) + message;
                        is_command = false;
                    } else if (current_state == users_list) {
                        sending = get_users;
                    }
                }

                if (!sending.empty()) {
                    my_send(client_socket, sending);
                    if (sending[0] == file) {
                        set_input_buffer("Sending...");
                        update_user_input();
                        send_file(client_socket, filepath);
                        set_input_buffer("");
                        update_user_input();
                    }
                    if (is_command) {
                        std::lock_guard lock(chat_mutex);
                        if (current_state == users_list) {
                            my_send(client_socket, sending);
                        } else if (current_state == dialogue) {
                            my_send(client_socket, sending);
                        }
                    }
                }
            }
        } else if (c == 127) {
            std::lock_guard lock(input_mutex);
            utf8_pop_back(input_buffer);
        } else if (c >= 1 && c <= 31) {
            continue;
        } else if (get_input_buffer() == "Cannot open file") {
            set_input_buffer(c);
        } else if (get_input_buffer() == "Wrong username") {
            set_input_buffer(c);
        } else {
            set_input_buffer(get_input_buffer() + c);
        }

        update_user_input();
    }

    cv_chat.notify_all();
    drawing_thread.join();
    shutdown(client_socket, SHUT_RD);
    receiving_thread.join();
    printf("\nConnection closed\n");
}