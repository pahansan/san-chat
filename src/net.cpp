#include "net.hpp"

#include <cstring>
#include <iostream>
#include <netdb.h>
#include <unistd.h>

int connect_to_server(const std::string& hostname)
{
    int client_socket;
    hostent* hp;

    if ((client_socket = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        std::cerr << "Error: can't get socket\n";
        return -1;
    }

    sockaddr_in server_address;
    memset((char*)&server_address, '\0', sizeof(server_address));
    server_address.sin_family = AF_INET;
    hp = gethostbyname(hostname.c_str());
    memcpy(&server_address.sin_addr, hp->h_addr_list[0], hp->h_length);
    server_address.sin_port = htons(PORT);

    if (connect(client_socket, (sockaddr*)&server_address, sizeof(server_address)) < 0) {
        std::cerr << "Error: can't connect to server\n";
        close(client_socket);
        return -1;
    }

    return client_socket;
}