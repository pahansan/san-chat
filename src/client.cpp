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

    while (!end) {
        char c;
        if (read(STDIN_FILENO, &c, 1) != 1)
            break;

        if (c == '\n') {
            std::string message = get_input_buffer();
            set_input_buffer("");

            if (!message.empty()) {
                std::string sending;
                user_state cur_state = get_current_state();
                std::string cur_user = get_current_user();

                if (message == "/users") {
                    set_current_state(users_list);
                    set_current_user("");
                    sending = get_users;
                } else if (message == "/exit") {
                    end = true;
                    break;
                } else if (cur_state == users_list) {
                    if (message.substr(0, 7) == "/select") {
                        sending = get_messages;
                        sending += skip_spaces(message.substr(7));
                        set_current_state(dialogue);
                        set_current_user(sending.substr(1));
                    } else {
                        sending = "";
                    }
                } else if (cur_state == dialogue || cur_state == files_list) {
                    if (message.substr(0, 6) == "/files") {
                        sending = get_messages;
                        sending += cur_user;
                        set_current_state(files_list);
                    } else if (message.substr(0, 9) == "/messages") {
                        sending = get_messages;
                        sending += cur_user;
                        set_current_state(dialogue);
                    } else if (message.substr(0, 5) == "/send") {
                        filepath = skip_spaces(message.substr(5));
                        if (!std::filesystem::exists(filepath)) {
                            print_notification("Cannot open file");
                            continue;
                        } else {
                            sending = file;
                            sending += get_file_name(filepath);
                        }
                    } else if (message.substr(0, 4) == "/get") {
                        std::string to_receive = skip_spaces(message.substr(4));
                        to_receive = to_receive.substr(0, to_receive.find_first_of(' '));
                        set_receiving_file(to_receive);
                        std::string username = message.substr(message.find_last_of(' ') + 1);
                        if (username == login || username == cur_user) {
                            sending = get_file;
                            sending += to_receive + '\036' + username;
                        } else {
                            print_notification("Wrong username");
                            continue;
                        }
                    } else if (cur_state == dialogue) {
                        sending = std::string(1, send_message);
                        sending += message;
                    }
                }

                if (!sending.empty()) {
                    my_send(client_socket, sending);
                    if (sending[0] == file) {
                        print_notification("Sending...");
                        send_file(client_socket, filepath);
                    }
                }
            }
        } else if (c == 127) {
            std::lock_guard lock(input_mutex);
            utf8_pop_back(input_buffer);
        } else if (c >= 1 && c <= 31) {
            continue;
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