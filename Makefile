all: server client

CXX=clang++
CXXFLAGS=-Wall -std=c++20
LDLIBS=-lsqlite3

server: server.cpp db.cpp

client: client.c
	clang -Wall -o client client.c

clean:
	rm -rf app chat.db client server