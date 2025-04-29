#include "db.hpp"
#include <condition_variable>
#include <cstdlib>
#include <cstring>
#include <format>
#include <iostream>
#include <mutex>
#include <netdb.h>
#include <netinet/in.h>
#include <signal.h>
#include <sstream>
#include <string>
#include <sys/socket.h>
#include <sys/types.h>
#include <thread>

#include "db.hpp"
#include "message_types.hpp"

#define BUFLEN (1024 * 1024)

enum user_state { users_list, dialogue, files_list };
user_state current_state = users_list;

typedef struct hostent hostent;
typedef struct sockaddr_in sockaddr_in;
typedef struct sockaddr sockaddr;

std::string current_user = "";

users_map parse_users(const std::string& str)
{
    users_map users;
    std::stringstream ss(str);
    std::string name;
    std::string status;

    while (getline(ss, name, '\036')) {
        getline(ss, status, '\036');
        users[name] = std::stoi(status);
    }
    return users;
}

void print_users(const std::string& users_string)
{
    system("clear");
    users_map users = parse_users(users_string);
    std::cout << "Список пользователей\n";
    for (const auto& [login, status] : users) {
        std::cout << login << ":";
        if (status == ONLINE)
            std::cout << "online\n";
        if (status == OFFLINE)
            std::cout << "offline\n";
    }
}

void print_messages(const std::string& messages)
{
    system("clear");
    std::stringstream ss(messages);
    std::string time;
    std::string name;
    std::string text;
    std::string is_file;

    getline(ss, name, '\036');
    getline(ss, name, '\036');

    while (getline(ss, time, '\036')) {
        std::cout << "[" << time;
        getline(ss, name, '\036');
        std::cout << ":" << name << "] ";
        getline(ss, text, '\036');
        std::cout << text << '\n';
        getline(ss, is_file, '\036');
    }
}

// void print_messages(const std::string& messages)
// {
//     system("clear");
//     std::stringstream ss(messages);
//     std::string text;

//     while (getline(ss, text, '\036')) {
//         std::cout << text << '\n';
//     }
// }

std::mutex chat_mutex;
std::condition_variable cv_chat;
std::string global_buffer;

bool is_current_user(const std::string& str)
{
    std::stringstream ss(str);
    std::string first;
    std::string second;

    getline(ss, first, '\036');
    getline(ss, second, '\036');

    return first == current_user || second == current_user;
}

void drawing()
{
    for (;;) {
        std::unique_lock lock(chat_mutex);
        cv_chat.wait(lock);
        lock.unlock();
        if (global_buffer[0] == messages) {
            {
                std::lock_guard lock(chat_mutex);
                if (current_state == dialogue && is_current_user(&global_buffer[1]))
                    print_messages(&global_buffer[1]);
            }
        } else if (global_buffer[0] == users) {
            {
                std::lock_guard lock(chat_mutex);
                if (current_state == users_list)
                    print_users(&global_buffer[1]);
            }
        }
    }
}

void receiving(int client_socket)
{
    int message_len = BUFLEN;
    char buffer[BUFLEN];
    memset(buffer, 0, BUFLEN);
    while (message_len > 0) {
        recv(client_socket, buffer, BUFLEN, 0);
        {
            std::lock_guard lock(chat_mutex);
            global_buffer = buffer;
        }
        cv_chat.notify_one();
        memset(buffer, 0, BUFLEN);
    }
}

std::string skip_spaces(const std::string& str)
{
    for (size_t i = 0; i < str.size(); i++) {
        if (str[i] != ' ')
            return str.substr(i, str.size());
    }
    return str;
}

void user_input(int client_socket)
{
    std::string sending;
    std::string getting;

    while (std::getline(std::cin, getting)) {
        if (getting == "/users") {
            {
                std::lock_guard lock(chat_mutex);
                current_state = users_list;
                current_user = "";
            }
            sending = get_users;
        } else if (getting.substr(0, 7) == "/select") {
            sending = get_messages;
            sending += skip_spaces(getting.substr(7));
            {
                std::lock_guard lock(chat_mutex);
                current_state = dialogue;
                current_user = sending.substr(1);
            }
        } else {
            {
                std::lock_guard lock(chat_mutex);
                if (current_state == dialogue) {
                    sending = send_message;
                    sending += getting;
                } else {
                    continue;
                }
            }
        }
        send(client_socket, sending.c_str(), sending.size(), 0);
    }
    // while (std::cin >> getting) {
    //     sending = "\004" + getting;
    //     send(client_socket, sending.c_str(), sending.size(), 0);
    // }
}

int main(int argc, char* argv[])
{
    int client_socket;
    hostent* hp;

    if (argc != 6) {
        std::cout << std::format("Usage: {} hostname port <reg/log> <login> <password>\n", argv[0]);
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
    server_address.sin_port = htons(atoi(argv[2]));

    if (connect(client_socket, (sockaddr*)&server_address, sizeof(server_address)) < 0) {
        std::cerr << "Error: can't connect to server\n";
        close(client_socket);
        return 1;
    }

    std::cout << "Connection established\n\n";

    signal(SIGPIPE, SIG_IGN);

    char buffer[BUFLEN];
    memset(buffer, 0, BUFLEN);

    std::string sending;
    std::string getting;

    std::string type(argv[3]);
    std::string login(argv[4]);
    std::string password(argv[5]);

    if (type == "reg")
        sending += registration;
    if (type == "log")
        sending += logining;

    sending += login + '\036' + password;

    send(client_socket, sending.c_str(), sending.size(), 0);
    recv(client_socket, buffer, BUFLEN, 0);

    std::string receiving_buf(buffer);

    switch (sending[0]) {
    case registration:
        if (receiving_buf == login_exists) {
            std::cout << "Пользователь с таким логином уже существует\n";
            close(client_socket);
            return 0;
        }
    case logining:
        if (receiving_buf == login_dont_exists) {
            std::cout << "Пользователя с таким логином не существует\n";
            close(client_socket);
            return 0;
        } else if (receiving_buf == incorrect_password) {
            std::cout << "Направильный пароль\n";
            close(client_socket);
            return 0;
        }
    }
    if (receiving_buf == db_fault) {
        std::cout << "Ошибка в работе базы данных\n";
        close(client_socket);
        return 0;
    }

    print_users(&buffer[1]);

    // std::cin >> getting;

    // std::cin.clear();

    // sending = get_messages;
    // sending += getting;

    // send(client_socket, sending.c_str(), sending.size(), 0);
    // recv(client_socket, buffer, BUFLEN, 0);
    // print_messages(buffer);
    // memset(buffer, 0, BUFLEN);

    // while (std::cin >> getting) {
    //     sending = "\004" + getting;
    //     send(client_socket, sending.c_str(), sending.size(), 0);
    //     recv(client_socket, buffer, BUFLEN, 0);
    //     print_messages(buffer);
    //     memset(buffer, 0, BUFLEN);
    // }

    std::thread(drawing).detach();
    std::thread(receiving, client_socket).detach();
    // std::thread(user_input, client_socket);

    while (std::getline(std::cin, getting)) {
        if (getting == "/users") {
            {
                std::lock_guard lock(chat_mutex);
                current_state = users_list;
                current_user = "";
            }
            sending = get_users;
        } else if (getting.substr(0, 7) == "/select") {
            sending = get_messages;
            sending += skip_spaces(getting.substr(7));
            {
                std::lock_guard lock(chat_mutex);
                current_state = dialogue;
                current_user = sending.substr(1);
            }
        } else {
            {
                std::lock_guard lock(chat_mutex);
                if (current_state == dialogue) {
                    sending = send_message;
                    sending += getting;
                } else {
                    continue;
                }
            }
        }
        send(client_socket, sending.c_str(), sending.size(), 0);
    }

    // while (std::cin >> getting) {
    //     sending = "\004" + getting;
    //     send(client_socket, sending.c_str(), sending.size(), 0);
    // }

    close(client_socket);
    printf("Connection closed\n");
}