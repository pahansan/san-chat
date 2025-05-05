#include "db.hpp"
#include "message_types.hpp"
#include "net.hpp"
#include "sendrecv.hpp"
#include "task_queue.hpp"

#include <arpa/inet.h>
#include <cstring>
#include <filesystem>
#include <format>
#include <fstream>
#include <iostream>
#include <signal.h>
#include <sstream>
#include <stdexcept>
#include <thread>
#include <vector>

class server {
public:
    class client {
        int socket_;
        sockaddr_in meta_;

    public:
        client(int server_socket)
        {
            memset(&meta_, 0, sizeof(meta_));
            socklen_t length = sizeof(meta_);
            if ((socket_ = accept(server_socket, (sockaddr*)&meta_, &length)) < 0)
                throw std::runtime_error("Server can't accept client");
        }

        ~client()
        {
            if (socket_ != -1)
                close(socket_);
        }

        client(client&& other) noexcept : socket_(other.socket_), meta_(other.meta_)
        {
            other.socket_ = -1;
        }

        client& operator=(client&& other) noexcept
        {
            if (this != &other) {
                close(socket_);

                socket_ = other.socket_;
                meta_ = other.meta_;

                other.socket_ = -1;
            }
            return *this;
        }

        client(const client&) = delete;
        client& operator=(const client&) = delete;

        int get_port()
        {
            return ntohs(meta_.sin_port);
        }

        std::string get_ip()
        {
            const char* char_addr = inet_ntoa(meta_.sin_addr);
            return std::string(char_addr);
        }

        int get_socket()
        {
            return socket_;
        }

        static std::vector<int> state;
    };

    server(sa_family_t sin_family_ = AF_INET, in_addr_t address_ = INADDR_ANY, in_port_t port_ = 0, size_t queue_len = 10)
    {
        if ((socket_ = socket(sin_family_, SOCK_STREAM, 0)) < 0)
            throw std::runtime_error("Server can't get socket");

        memset(&meta_, 0, sizeof(meta_));

        meta_.sin_family = sin_family_;
        meta_.sin_addr.s_addr = htonl(address_);
        meta_.sin_port = htons(port_);
        if (bind(socket_, (sockaddr*)&meta_, sizeof(meta_)) < 0) {
            throw std::runtime_error("Bind failed: " + std::string(strerror(errno)));
        }

        socklen_t namelen = sizeof(meta_);

        if (getsockname(socket_, (sockaddr*)&meta_, &namelen) < 0)
            throw std::runtime_error("Server can't get socket name");

        if (listen(socket_, queue_len) < 0)
            throw std::runtime_error("Server can't listen socket");
    }

    ~server()
    {
        close(socket_);
    }

    int get_port()
    {
        return ntohs(meta_.sin_port);
    }

    std::string get_ip()
    {
        const char* char_addr = inet_ntoa(meta_.sin_addr);
        return std::string(char_addr);
    }

    int get_socket()
    {
        return socket_;
    }

    client accept_client()
    {
        return client(socket_);
    }

private:
    int socket_;
    sockaddr_in meta_;
};

std::vector<std::string> split_string(const std::string& str)
{
    std::stringstream ss(str);
    std::string token;
    std::vector<std::string> tokens;

    while (getline(ss, token, '\036'))
        tokens.push_back(token);

    return tokens;
}

std::string get_file_name_from_message(const std::string& message)
{
    return message.substr(1, message.find_first_of('\036') - 1);
}

std::vector<char> get_file_from_message(const std::string& message, ssize_t bytes_received)
{
    std::vector<char> raw_data;
    raw_data.resize(message.size() - (message.find_first_of('\036') + 1));
    for (const auto& byte : message.substr(message.find_first_of('\036') + 1))
        raw_data.push_back(byte);
    return raw_data;
}

void thread_func(server::client&& working)
{
    std::cout << "<[" << working.get_ip() << ':' << working.get_port() << "]\n";

    ssize_t bytes_received;
    std::string received;

    const int socket = working.get_socket();

    bytes_received = my_recv(working.get_socket(), received);
    if (bytes_received <= 0) {
        std::cout << ">[" << working.get_ip() << ':' << working.get_port() << "]\n";
        return;
    }
    switch (received[0]) {
    case registration: {
        std::vector<std::string> login_data = split_string(received.substr(1));
        std::string login = login_data[0];
        std::string password = login_data[1];

        if (user_exists(login)) {
            my_send(socket, login_exists);
            std::cout << ">[" << working.get_ip() << ':' << working.get_port() << "]\n";
            return;
        } else if (add_user(login, password)) {
            my_send(socket, db_fault);
            std::cout << ">[" << working.get_ip() << ':' << working.get_port() << "]\n";
            return;
        } else if (connect_user(login, socket)) {
            my_send(socket, user_already_connected);
            std::cout << ">[" << working.get_ip() << ':' << working.get_port() << "]\n";
            return;
        }
        break;
    }
    case logining: {
        std::vector<std::string> login_data = split_string(received.substr(1));
        std::string login = login_data[0];
        std::string password = login_data[1];
        std::cout << login << ":" << password << '\n';
        if (!user_exists(login)) {
            my_send(socket, login_dont_exists);
            std::cout << ">[" << working.get_ip() << ':' << working.get_port() << "]\n";
            return;
        }
        if (!verify_user(login, password)) {
            my_send(socket, incorrect_password);
            std::cout << ">[" << working.get_ip() << ':' << working.get_port() << "]\n";
            return;
        } else if (connect_user(login, socket)) {
            my_send(socket, user_already_connected);
            std::cout << ">[" << working.get_ip() << ':' << working.get_port() << "]\n";
            return;
        }
        break;
    }
    default:
        std::cout << ">[" << working.get_ip() << ':' << working.get_port() << "]\n";
        return;
    }

    std::string login = get_login_by_fd(socket);
    std::string receiver_login;

    std::string filename;
    std::string filepath;
    std::vector<char> raw_data;
    std::ofstream file_stream;

    change_user_status(login, ONLINE);
    send_status_to_all();

    do {
        bytes_received = my_recv(socket, received);

        switch (received[0]) {
        case get_users:
            send_status_to_one(login);
            break;
        case get_messages:
            receiver_login = received.substr(1);
            if (!user_exists(receiver_login)) {
                bytes_received = my_send(socket, login_dont_exists);
            } else {
                client_send_message_list(login, receiver_login);
            }
            break;
        case send_message:
            add_message(login, receiver_login, received.substr(1));
            client_send_message_list(login, receiver_login);
            break;
        case file:
            filename = received.substr(1);
            std::filesystem::create_directories("files/" + login);
            filepath = "./files/" + login + "/" + filename;
            bytes_received = recv_file(socket, filepath);
            if (bytes_received > 0) {
                add_file(login, receiver_login, filename);
                client_send_message_list(login, receiver_login);
            }
            break;
        case get_file:
            std::string filename = received.substr(1, received.find_first_of('\036') - 1);
            std::string username = received.substr(received.find_last_of('\036') + 1);
            std::string filepath = "files/" + username + "/" + filename;
            std::string to_send;
            to_send = file;
            if (!std::filesystem::exists(filepath)) {
                bytes_received = my_send(socket, file_not_found);
            } else {
                bytes_received = my_send(socket, to_send);
                bytes_received = send_file(socket, filepath);
            }
            break;
        }
    } while (bytes_received > 0);

    std::cout << ">[" << working.get_ip() << ':' << working.get_port() << "]\n";
    change_user_status(login, OFFLINE);
    disconnect_user(login);
    send_status_to_all();
}

int main()
{
    signal(SIGPIPE, SIG_IGN);
    try {
        if (init_database())
            return 1;

        server tcp(AF_INET, INADDR_ANY, PORT, 10);

        std::cout << std::format("Server up: [{}:{}]\n\n", tcp.get_ip(), tcp.get_port());
        std::thread(handler).detach();

        for (;;) {
            std::thread(thread_func, tcp.accept_client()).detach();
        }
    } catch (const std::exception& e) {
        std::cerr << e.what() << '\n';
    }
}
