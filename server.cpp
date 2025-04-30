#include <arpa/inet.h>
#include <cstring>
#include <format>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <thread>
#include <vector>
// #include <netinet/in.h>

#include "db.hpp"
#include "message_types.hpp"
#include "task_queue.hpp"

#define BUFLEN (1024 * 1024)

std::mutex server_mutex;

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
            if ((socket_ = accept(server_socket, (struct sockaddr*)&meta_, &length)) < 0)
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
        if (bind(socket_, (struct sockaddr*)&meta_, sizeof(meta_)) < 0) {
            throw std::runtime_error("Bind failed: " + std::string(strerror(errno)));
        }

        socklen_t namelen = sizeof(meta_);

        if (getsockname(socket_, (struct sockaddr*)&meta_, &namelen) < 0)
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
    struct sockaddr_in meta_;
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

void thread_func(server::client&& working)
{
    std::cout << "<[" << working.get_ip() << ':' << working.get_port() << "]\n";
    char buffer[BUFLEN];
    int message_len = BUFLEN;
    memset(buffer, '\0', BUFLEN);

    while (buffer[0] == 0) {
        message_len = recv(working.get_socket(), buffer, BUFLEN, 0);
        if (message_len <= 0) {
            std::cout << ">[" << working.get_ip() << ':' << working.get_port() << "]\n";
            return;
        }
        switch (buffer[0]) {
        case registration: {
            std::vector<std::string> login_data = split_string(&buffer[1]);
            std::string login = login_data[0];
            std::string password = login_data[1];
            if (user_exists(login)) {
                send(working.get_socket(), login_exists.c_str(), login_exists.size(), 0);
                std::cout << ">[" << working.get_ip() << ':' << working.get_port() << "]\n";
                return;
            }
            if (add_user(login, password)) {
                send(working.get_socket(), db_fault.c_str(), db_fault.size(), 0);
                std::cout << ">[" << working.get_ip() << ':' << working.get_port() << "]\n";
                return;
            }
            {
                std::lock_guard lock(server_mutex);
                fd_list.push_back(working.get_socket());
                login_list.push_back(login);
            }
            break;
        }
        case logining: {
            std::vector<std::string> login_data = split_string(&buffer[1]);
            std::string login = login_data[0];
            std::string password = login_data[1];
            if (!user_exists(login)) {
                send(working.get_socket(), login_dont_exists.c_str(), login_dont_exists.size(), 0);
                std::cout << ">[" << working.get_ip() << ':' << working.get_port() << "]\n";
                return;
            }
            if (!verify_user(login, password)) {
                send(working.get_socket(), incorrect_password.c_str(), incorrect_password.size(), 0);
                std::cout << ">[" << working.get_ip() << ':' << working.get_port() << "]\n";
                return;
            }
            {
                std::lock_guard lock(server_mutex);
                fd_list.push_back(working.get_socket());
                login_list.push_back(login);
            }
            break;
        }
        default:
            std::cout << ">[" << working.get_ip() << ':' << working.get_port() << "]\n";
            return;
        }
    }

    std::string login = get_login_by_fd(working.get_socket());
    std::string receiver_login;

    change_user_status(login, ONLINE);
    send_status_to_all();

    do {
        memset(buffer, '\0', BUFLEN);
        message_len = recv(working.get_socket(), buffer, BUFLEN, 0);

        switch (buffer[0]) {
        case get_users:
            send_status_to_one(login);
            break;
        case get_messages:
            receiver_login = &buffer[1];
            if (!user_exists(receiver_login)) {
                send(working.get_socket(), login_dont_exists.c_str(), login_dont_exists.size(), 0);
            } else
                client_send_message_list(login, receiver_login);
            break;
        case send_message:
            add_message(login, receiver_login, &buffer[1]);
            client_send_message_list(login, receiver_login);
            break;
        case disconnect:
            send(working.get_socket(), login.c_str(), login.size(), 0);
        }
    } while (message_len > 0);

    std::cout << ">[" << working.get_ip() << ':' << working.get_port() << "]\n";
    change_user_status(login, OFFLINE);
    erase_user(working.get_socket());
    send_status_to_all();
}

int main()
{
    try {
        if (init_database())
            return 1;

        server tcp(AF_INET, INADDR_ANY, 8080, 10);

        std::cout << std::format("Server up: [{}:{}]\n\n", tcp.get_ip(), tcp.get_port());
        std::thread(handler).detach();

        for (;;) {
            std::thread(thread_func, tcp.accept_client()).detach();
        }
    } catch (const std::exception& e) {
        std::cerr << e.what() << '\n';
    }
}
