all: server client

CXX=clang++
CXXFLAGS=-Wall -std=c++20 -g
LDLIBS=-lsqlite3

server: server.cpp db.cpp task_queue.cpp message_types.cpp sendrecv.cpp

client: client.cpp message_types.cpp sendrecv.cpp

clean:
	rm -rf app chat.db client server