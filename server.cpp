#include <string>
#include <netinet/in.h>
#include <iostream>
#include <cstring>
#include <stdexcept>
#include <arpa/inet.h>
#include <unistd.h>
#include <vector>
#include <thread>
#include <format>
#include <algorithm>

#define BUFLEN 1024

class server
{
private:
    class client_
    {
        int socket_;
        sockaddr_in meta_;

    public:
        client_(int server_socket)
        {
            std::cout << "aboba\n";
            socklen_t client_meta_len = sizeof(meta_);
            if ((socket_ = accept(server_socket, (struct sockaddr *)&meta_, &client_meta_len)) < 0)
                throw std::runtime_error("Server can't accept client");
        }

        void run()
        {
            char buffer[BUFLEN];
            int message_len = 1;
            while (message_len > 0)
            {
                memset(buffer, '\0', BUFLEN);
                message_len = recv(socket_, buffer, BUFLEN, 0);

                std::cout << std::format("[{}:{}]: ", get_ip(), get_port());
                if (message_len == 0)
                    std::cout << std::format("Client disconnected\n\n");
                else
                    std::cout << std::format("{}\n\n", buffer);
            }
        }

        ~client_()
        {
            std::cout << "jops\n";
            close(socket_);
        }

        client_(client_ &&other) noexcept
            : socket_(other.socket_), meta_(other.meta_)
        {
            other.socket_ = -1;
        }

        client_ &operator=(client_ &&other) noexcept
        {
            if (this != &other)
            {
                close(socket_);

                socket_ = other.socket_;
                meta_ = other.meta_;

                other.socket_ = -1;
            }
            return *this;
        }

        client_(const client_ &) = delete;
        client_ &operator=(const client_ &) = delete;

        int get_port()
        {
            return ntohs(meta_.sin_port);
        }

        std::string get_ip()
        {
            const char *char_addr = inet_ntoa(meta_.sin_addr);
            return std::string(char_addr);
        }
    };

public:
    server(sa_family_t sin_family_ = AF_INET, in_addr_t address_ = INADDR_ANY, in_port_t port_ = 0, size_t queue_len = 10)
    {
        if ((socket_ = socket(sin_family_, SOCK_STREAM, 0)) < 0)
            throw std::runtime_error("Server can't get socket");

        bzero(&meta_, sizeof(meta_));

        meta_.sin_family = sin_family_;
        meta_.sin_addr.s_addr = address_;
        meta_.sin_port = htons(port_);
        if (bind(socket_, (struct sockaddr *)&meta_, sizeof(meta_)) < 0)
            throw std::runtime_error("Server can't bind socket");

        socklen_t namelen = sizeof(meta_);

        if (getsockname(socket_, (struct sockaddr *)&meta_, &namelen) < 0)
            throw std::runtime_error("Server can't get socket name");

        if (listen(socket_, queue_len) < 0)
            throw std::runtime_error("Server can't listen socket");
    }

    ~server()
    {
        close(socket_);
    }

    void thread_func(client_ client)
    {
        client.run();
    }

    int get_port()
    {
        return ntohs(meta_.sin_port);
    }

    std::string get_ip()
    {
        const char *char_addr = inet_ntoa(meta_.sin_addr);
        return std::string(char_addr);
    }

    int get_socket()
    {
        return socket_;
    }

    void accept_client()
    {
        client_ client(socket_);
        threads.push_back(std::thread(&server::thread_func, this, std::move(client)));
    }

private:
    int socket_;
    struct sockaddr_in meta_;
    std::vector<std::thread> threads;
    std::vector<client_> clients_;
};

int main()
{
    try
    {
        server tcp(AF_INET, INADDR_ANY, 0, 10);

        std::cout << std::format("Server up: [{}:{}]\n\n", tcp.get_ip(), tcp.get_port());

        for (;;)
        {
            tcp.accept_client();
        }
    }
    catch (const std::exception &e)
    {
        std::cerr << e.what() << '\n';
    }
}
