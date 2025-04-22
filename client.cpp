#include "db.hpp"
#include <cstring>
#include <format>
#include <iostream>
#include <netdb.h>
#include <netinet/in.h>
#include <signal.h>
#include <string>
#include <sys/socket.h>
#include <sys/types.h>

#define BUFLEN 1024

typedef struct hostent hostent;
typedef struct sockaddr_in sockaddr_in;
typedef struct sockaddr sockaddr;

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

    char message[1024];

    while (std::cin >> message) {
        if (send(client_socket, message, strlen(message), 0) < 0) {
            perror("Error: can't send messege to server");
            close(client_socket);
            return 1;
        }
    }

    close(client_socket);
    printf("Connection closed\n");
}