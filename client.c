#include <errno.h>
#include <netdb.h>
#include <netinet/in.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#define BUFLEN 1024

typedef struct hostent hostent;
typedef struct sockaddr_in sockaddr_in;
typedef struct sockaddr sockaddr;

int main(int argc, char *argv[])
{
    int client_socket;
    hostent *hp;

    if (argc < 4)
    {
        printf("Usage: %s hostname port messege\n", argv[0]);
        return 1;
    }

    if ((client_socket = socket(AF_INET, SOCK_STREAM, 0)) < 0)
    {
        perror("Error: can't get socket");
        return 1;
    }

    sockaddr_in server_address;
    memset((char *)&server_address, '\0', sizeof(server_address));
    server_address.sin_family = AF_INET;
    hp = gethostbyname(argv[1]);
    memcpy(&server_address.sin_addr, hp->h_addr_list[0], hp->h_length);
    server_address.sin_port = htons(atoi(argv[2]));

    if (connect(client_socket, (sockaddr *)&server_address, sizeof(server_address)) < 0)
    {
        perror("Error: can't connect to server");
        close(client_socket);
        return 1;
    }

    printf("Connection established\n\n");

    int messege_number = atoi(argv[3]);

    signal(SIGPIPE, SIG_IGN);

    for (int i = 0; i < messege_number * messege_number; i++)
    {
        printf("Sending messege \"%s\"\n\n", argv[3]);
        if (send(client_socket, argv[3], strlen(argv[3]), 0) < 0)
        {
            perror("Error: can't send messege to server");
            close(client_socket);
            return 1;
        }
        sleep(messege_number);
    }

    close(client_socket);
    printf("Connection closed\n");
}