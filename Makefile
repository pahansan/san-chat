CXX = clang++
CXXFLAGS = -Wall -std=c++20 -O3 -MMD -MP
LDLIBS = -lsqlite3

SERVER = server
CLIENT = san-chat

SRC_DIR = src
OBJ_DIR = obj

SERVER_SRCS = $(SRC_DIR)/server.cpp $(SRC_DIR)/db.cpp $(SRC_DIR)/task_queue.cpp $(SRC_DIR)/message_types.cpp $(SRC_DIR)/sendrecv.cpp
CLIENT_SRCS = $(SRC_DIR)/client.cpp $(SRC_DIR)/message_types.cpp $(SRC_DIR)/sendrecv.cpp

SERVER_OBJS = $(SERVER_SRCS:$(SRC_DIR)/%.cpp=$(OBJ_DIR)/%.o)
CLIENT_OBJS = $(CLIENT_SRCS:$(SRC_DIR)/%.cpp=$(OBJ_DIR)/%.o)

-include $(SERVER_OBJS:$(OBJ_DIR)/%.o=$(OBJ_DIR)/%.d)
-include $(CLIENT_OBJS:$(OBJ_DIR)/%.o=$(OBJ_DIR)/%.d)

all: $(SERVER) $(CLIENT)

$(SERVER): $(SERVER_OBJS)
	$(CXX) $(SERVER_OBJS) -o $(SERVER) $(LDLIBS)

$(CLIENT): $(CLIENT_OBJS)
	$(CXX) $(CLIENT_OBJS) -o $(CLIENT) $(LDLIBS)

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.cpp
	@mkdir -p $(OBJ_DIR)
	$(CXX) $(CXXFLAGS) -c $< -o $@

clean:
	rm -rf $(OBJ_DIR) $(SERVER) $(CLIENT)

.PHONY: all clean
