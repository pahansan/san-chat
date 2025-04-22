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
public:
    class client
    {
        int socket_;
        sockaddr_in meta_;

    public:
        client(int server_socket)
        {
            memset(&meta_, 0, sizeof(meta_));
            socklen_t length = sizeof(meta_);
            if ((socket_ = accept(server_socket, (struct sockaddr *)&meta_, &length)) < 0)
                throw std::runtime_error("Server can't accept client");
        }

        ~client()
        {
            if (socket_ != -1)
                close(socket_);
        }

        client(client &&other) noexcept
            : socket_(other.socket_), meta_(other.meta_)
        {
            other.socket_ = -1;
        }

        client &operator=(client &&other) noexcept
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

        client(const client &) = delete;
        client &operator=(const client &) = delete;

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

        static std::vector<int> state;
    };

    server(sa_family_t sin_family_ = AF_INET, in_addr_t address_ = INADDR_ANY, in_port_t port_ = 0, size_t queue_len = 10)
    {
        if ((socket_ = socket(sin_family_, SOCK_STREAM, 0)) < 0)
            throw std::runtime_error("Server can't get socket");

        memset(&meta_, 0, sizeof(meta_));

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

    client accept_client()
    {
        return client(socket_);
    }

private:
    int socket_;
    struct sockaddr_in meta_;
};

bool status = true;
     
void thread_func(server::client &&working)
{
    char buffer[BUFLEN];
    int message_len = 1;
    while (message_len > 0)
    {
        memset(buffer, '\0', BUFLEN);
        message_len = recv(working.get_socket(), buffer, BUFLEN, 0);

        std::cout << std::format("[{}:{}]: ", working.get_ip(), working.get_port());
        if (message_len == 0)
            std::cout << std::format("Client disconnected\n\n");
        else
            std::cout << std::format("{}\n\n", buffer);
    }
    status = false;
}

int main()
{
    try
    {
        server tcp(AF_INET, INADDR_ANY, 0, 10);

        std::cout << std::format("Server up: [{}:{}]\n\n", tcp.get_ip(), tcp.get_port());

        for (;;)
        {
            std::thread(thread_func, tcp.accept_client()).detach();
        }
    }
    catch (const std::exception &e)
    {
        std::cerr << e.what() << '\n';
    }
}
