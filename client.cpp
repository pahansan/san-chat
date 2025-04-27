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

typedef struct hostent hostent;
typedef struct sockaddr_in sockaddr_in;
typedef struct sockaddr sockaddr;

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

void print_users(users_map users)
{
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

void drawing()
{
    for (;;) {
        std::unique_lock lock(chat_mutex);
        cv_chat.wait(lock);
        lock.unlock();
        if (global_buffer[0] == messages)
            print_messages(&global_buffer[1]);
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

void user_input(int client_socket)
{
    std::string sending;
    std::string getting;
    while (std::cin >> getting) {
        sending = "\004" + getting;
        send(client_socket, sending.c_str(), sending.size(), 0);
    }
}

int main(int argc, char* argv[])
{
    int client_socket;
    hostent* hp;

    if (argc != 3) {
        std::cout << std::format("Usage: {} hostname port\n", argv[0]);
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
    std::cout << "Введите 1, чтобы заргистрироваться, введите 2, чтобы войти\n";
    std::cin >> getting;

    if (getting == "1")
        sending += "\001";
    if (getting == "2")
        sending += "\002";

    std::cout << "Введите логин\n";
    std::cin >> getting;
    sending += getting + "\036";
    std::cout << "Введите пароль\n";
    std::cin >> getting;
    sending += getting;

    send(client_socket, sending.c_str(), sending.size(), 0);
    recv(client_socket, buffer, BUFLEN, 0);

    users_map users = parse_users(buffer);
    memset(buffer, 0, BUFLEN);
    print_users(users);

    std::cin >> getting;

    sending = "\003" + getting;
    send(client_socket, sending.c_str(), sending.size(), 0);
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

    while (std::cin >> getting) {
        sending = "\004" + getting;
        send(client_socket, sending.c_str(), sending.size(), 0);
    }

    close(client_socket);
    printf("Connection closed\n");
}