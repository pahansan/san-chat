CXX := clang++
CXXFLAGS := -Wall -std=c++20 -MMD -MP

LDLIBS := -lsqlite3

SRC_DIR := src
OBJ_DIR := obj
SRV_OBJ_DIR := $(OBJ_DIR)/server
CLT_OBJ_DIR := $(OBJ_DIR)/client

SERVER_SRCS := 	$(SRC_DIR)/server.cpp \
				$(SRC_DIR)/db.cpp \
				$(SRC_DIR)/task_queue.cpp \
				$(SRC_DIR)/message_types.cpp \
				$(SRC_DIR)/sendrecv.cpp

CLIENT_SRCS := 	$(SRC_DIR)/client.cpp \
				$(SRC_DIR)/message_types.cpp \
				$(SRC_DIR)/sendrecv.cpp \
				$(SRC_DIR)/utf8_string.cpp \
				$(SRC_DIR)/terminal.cpp \
				$(SRC_DIR)/parsing.cpp \
				$(SRC_DIR)/interface.cpp \
				$(SRC_DIR)/thread_handlers.cpp \
 				$(SRC_DIR)/net.cpp

SERVER_OBJS := $(patsubst $(SRC_DIR)/%.cpp,$(SRV_OBJ_DIR)/%.o,$(SERVER_SRCS))
CLIENT_OBJS := $(patsubst $(SRC_DIR)/%.cpp,$(CLT_OBJ_DIR)/%.o,$(CLIENT_SRCS))

all: server client

server: $(SERVER_OBJS)
	$(CXX) $(CXXFLAGS) $^ -o $@ $(LDLIBS)

client: $(CLIENT_OBJS)
	$(CXX) $(CXXFLAGS) $^ -o $@ $(LDLIBS)

$(SRV_OBJ_DIR)/%.o: $(SRC_DIR)/%.cpp
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) -c $< -o $@

$(CLT_OBJ_DIR)/%.o: $(SRC_DIR)/%.cpp
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) -c $< -o $@

-include $(SERVER_OBJS:.o=.d)
-include $(CLIENT_OBJS:.o=.d)

clean:
	rm -rf $(OBJ_DIR) server client

.PHONY: all clean
