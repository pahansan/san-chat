# Компилятор и флаги
CXX := clang++
CXXFLAGS := -Wall -std=c++20 -MMD -MP

# Библиотеки
LDLIBS := -lsqlite3

# Каталоги
SRC_DIR := src
OBJ_DIR := obj
SRV_OBJ_DIR := $(OBJ_DIR)/server
CLT_OBJ_DIR := $(OBJ_DIR)/client

# Исходные файлы
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
				$(SRC_DIR)/thread_handlers.cpp

# Объектные файлы
SERVER_OBJS := $(patsubst $(SRC_DIR)/%.cpp,$(SRV_OBJ_DIR)/%.o,$(SERVER_SRCS))
CLIENT_OBJS := $(patsubst $(SRC_DIR)/%.cpp,$(CLT_OBJ_DIR)/%.o,$(CLIENT_SRCS))

# Цели по умолчанию
all: server client

# Сборка серверной части
server: $(SERVER_OBJS)
	$(CXX) $(CXXFLAGS) $^ -o $@ $(LDLIBS)

# Сборка клиентской части
client: $(CLIENT_OBJS)
	$(CXX) $(CXXFLAGS) $^ -o $@ $(LDLIBS)

# Правила компиляции исходных файлов в объектные
$(SRV_OBJ_DIR)/%.o: $(SRC_DIR)/%.cpp
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) -c $< -o $@

$(CLT_OBJ_DIR)/%.o: $(SRC_DIR)/%.cpp
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) -c $< -o $@

# Подключение файлов зависимостей
-include $(SERVER_OBJS:.o=.d)
-include $(CLIENT_OBJS:.o=.d)

# Очистка
clean:
	rm -rf $(OBJ_DIR) server client

.PHONY: all clean
