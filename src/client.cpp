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
    int client_socket;
    hostent* hp;

    if (argc != 5) {
        std::cout << std::format("Usage: {} hostname <reg/log> <login> <password>\n", argv[0]);
        return 1;
    }

    if ((client_socket = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        std::cerr << "Error: can't get socket\n";
        return 1;
    }

    sockaddr_in server_address;
    memset((char*)&server_address, '\0', sizeof(server_address));
    server_address.sin_family = AF_INET;
    hp = gethostbyname(argv[1]);
    memcpy(&server_address.sin_addr, hp->h_addr_list[0], hp->h_length);
    server_address.sin_port = htons(PORT);

    if (connect(client_socket, (sockaddr*)&server_address, sizeof(server_address)) < 0) {
        std::cerr << "Error: can't connect to server\n";
        close(client_socket);
        return 1;
    }

    std::cout << "Connection established\n\n";

    signal(SIGPIPE, SIG_IGN);

    std::string received;

    std::string sending;
    std::string getting;

    std::string type(argv[2]);
    std::string login(argv[3]);
    std::string password(argv[4]);

    if (type == "reg")
        sending += registration;
    if (type == "log")
        sending += logining;

    sending += login + '\036' + password;
    std::cout << login << ":" << password << '\n';
    my_send(client_socket, sending);
    my_recv(client_socket, received);

    switch (sending[0]) {
    case registration:
        if (received == login_exists) {
            std::cout << "Пользователь с таким логином уже существует\n";
            close(client_socket);
            return 0;
        }
    case logining:
        if (received == login_dont_exists) {
            std::cout << "Пользователя с таким логином не существует\n";
            close(client_socket);
            return 0;
        } else if (received == incorrect_password) {
            std::cout << "Направильный пароль\n";
            close(client_socket);
            return 0;
        }
    }
    if (received == db_fault) {
        std::cout << "Ошибка в работе базы данных\n";
        close(client_socket);
        return 0;
    }

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
            if (input_buffer == "Cannot open file") {
                std::lock_guard lock(input_mutex);
                input_buffer = "";
            } else if (input_buffer == "Wrong username") {
                std::lock_guard lock(input_mutex);
                input_buffer = "";
            }

            std::string message;
            {
                std::lock_guard lock(input_mutex);
                message = input_buffer;
                input_buffer.clear();
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
                            input_buffer = "Cannot open file";
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
                            input_buffer = "Wrong username";
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
                        std::lock_guard lk(input_mutex);
                        move_cursor(w.ws_row, 0);
                        std::cout << "Sending...";
                        std::cout << std::string(w.ws_col - 10, ' ');
                        std::cout << input_buffer;
                        std::cout.flush();
                        send_file(client_socket, filepath);
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
        } else if (input_buffer == "Cannot open file") {
            std::lock_guard lock(input_mutex);
            input_buffer = c;
        } else if (input_buffer == "Wrong username") {
            std::lock_guard lock(input_mutex);
            input_buffer = c;
        } else {
            std::lock_guard lock(input_mutex);
            input_buffer += c;
        }

        update_user_input();
    }

    cv_chat.notify_all();
    drawing_thread.join();
    shutdown(client_socket, SHUT_RD);
    receiving_thread.join();
    printf("\nConnection closed\n");
}