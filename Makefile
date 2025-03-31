all: server client

server: server.cpp
	clang++ -Wall -std=c++20 -o server server.cpp

client: client.c
	clang -Wall -o client client.c
