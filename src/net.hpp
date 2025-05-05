#pragma once

#include <netdb.h>
#include <string>

#define PORT 8080

typedef struct hostent hostent;
typedef struct sockaddr_in sockaddr_in;
typedef struct sockaddr sockaddr;

int connect_to_server(const std::string& hostname);