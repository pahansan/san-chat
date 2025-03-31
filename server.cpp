#include <string>
#include <netinet/in.h>
#include <iostream>
#include <cstring>  // Замена <strings.h> на <cstring> для переносимости
#include <stdexcept>
#include <arpa/inet.h>

class server
{
public:
    server(sa_family_t sin_family_ = AF_INET, in_addr_t address_ = INADDR_ANY, in_port_t port_ = 0)
    {
        if ((main_socket_ = socket(sin_family_, SOCK_STREAM, 0)) < 0)
        {
            throw std::runtime_error("Server can't get socket");
        }
        bzero(&meta_, sizeof(meta_));
        meta_.sin_family = sin_family_;
        meta_.sin_addr.s_addr = address_;
        meta_.sin_port = htons(port_);  // Преобразование порядка байтов
        if (bind(main_socket_, (struct sockaddr *)&meta_, sizeof(meta_)) < 0)
        {
            throw std::runtime_error("Server can't bind socket");
        }

        socklen_t namelen = sizeof(meta_);

        if (getsockname(main_socket_, (struct sockaddr *)&meta_, &namelen) < 0)
        {
            throw std::runtime_error("Server can't get socket name");
        }
    }

    int get_port()
    {
        return ntohs(meta_.sin_port);  // Преобразование порядка байтов
    }

    std::string get_ip()
    {
        const char *char_addr = inet_ntoa(meta_.sin_addr);
        return std::string(char_addr);
    }

private:
    int main_socket_;
    struct sockaddr_in meta_;
};

int main()
{
    server a;

    std::cout << "Port: " << a.get_port() << '\n';
    std::cout << "Address: " << a.get_ip() << '\n';
}
