all: server client

CXX=clang++
CXXFLAGS=-Wall -std=c++20
LDLIBS=-lsqlite3

server: server.cpp db.cpp task_queue.cpp

client: client.cpp

clean:
	rm -rf app chat.db client server