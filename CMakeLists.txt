cmake_minimum_required(VERSION 3.22)

project(san-chat)

set(CMAKE_CXX_STANDARD 20)
set(CXX_STANDARD_REQUIRED True)

set(SRC_DIR src)

find_library(LIBRARY_SQLITE3 sqlite3)

add_executable(server   ${SRC_DIR}/server.cpp
                        ${SRC_DIR}/db.cpp
                        ${SRC_DIR}/task_queue.cpp
                        ${SRC_DIR}/message_types.cpp
                        ${SRC_DIR}/sendrecv.cpp)

target_link_libraries(server PRIVATE ${LIBRARY_SQLITE3})

add_executable(client   ${SRC_DIR}/client.cpp
                        ${SRC_DIR}/message_types.cpp
                        ${SRC_DIR}/sendrecv.cpp
                        ${SRC_DIR}/utf8_string.cpp
                        ${SRC_DIR}/terminal.cpp
                        ${SRC_DIR}/parsing.cpp
                        ${SRC_DIR}/interface.cpp
                        ${SRC_DIR}/thread_handlers.cpp
                        ${SRC_DIR}/net.cpp)
