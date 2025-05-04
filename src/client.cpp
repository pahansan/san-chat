#include "db.hpp"
#include "message_types.hpp"
#include "sendrecv.hpp"

#include <codecvt>
#include <condition_variable>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <format>
#include <fstream>
#include <iostream>
#include <mutex>
#include <netdb.h>
#include <netinet/in.h>
#include <signal.h>
#include <sstream>
#include <string>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <termios.h>
#include <thread>
#include <unistd.h>

#define BUFLEN (1024 * 1024 * 10)

#define PORT 8080


bool end = false;

enum user_state { users_list, dialogue, files_list };
user_state current_state = users_list;

std::string input_buffer;
std::mutex input_mutex;

std::mutex chat_mutex;
std::condition_variable cv_chat;
std::string global_buffer;

enum prevent { space, no_space };

typedef struct hostent hostent;
typedef struct sockaddr_in sockaddr_in;
typedef struct sockaddr sockaddr;

std::string current_user = "";
std::string receiving_file = "";

struct termios orig_termios;

void disableRawMode()
{
    tcsetattr(STDIN_FILENO, TCSAFLUSH, &orig_termios);
}

void enableRawMode()
{
    tcgetattr(STDIN_FILENO, &orig_termios);
    atexit(disableRawMode);
    struct termios raw = orig_termios;
    raw.c_lflag &= ~(ECHO | ICANON);
    tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw);
}

users_map parse_users(const std::string& str)
{
    users_map users;
    std::stringstream ss(str);
    std::string name;
    std::string status;

    while (getline(ss, name, '\036')) {
        getline(ss, status, '\036');
        users[name] = std::stoi(status);
    }
    return users;
}

void move_cursor(int row, int col)
{
    std::cout << "\033[" << row << ";" << col << "H";
}

void moveCursorToEndOfLine()
{
    std::cout << "\033[999C";
}

size_t utf8_strlen(const std::string& utf8_string)
{
    std::wstring_convert<std::codecvt_utf8<wchar_t>> converter;
    std::wstring wide_string = converter.from_bytes(utf8_string);
    return wide_string.size();
}

void print_users(const std::string& users_string)
{
    system("clear");
    struct winsize w;
    ioctl(STDOUT_FILENO, TIOCGWINSZ, &w);
    size_t width = w.ws_col;
    size_t height = w.ws_row;

    users_map users = parse_users(users_string);
    std::cout << "Список пользователей\n";
    size_t lines_printed = 1;

    for (const auto& [login, status] : users) {
        if (lines_printed >= height - 3)
            break;
        std::cout << login << ":";
        if (status == ONLINE)
            std::cout << "online\n";
        else if (status == OFFLINE)
            std::cout << "offline\n";
        ++lines_printed;
    }

    std::lock_guard lock(input_mutex);
    move_cursor(height, 0);
    std::cout << input_buffer;
    std::cout << std::string(width - utf8_strlen(input_buffer), ' ');
    move_cursor(height, utf8_strlen(input_buffer) % width + 1);

    fflush(stdout);
}

std::vector<std::string> fit_text(const std::string& text, const int& width)
{
    std::vector<std::string> fitted;
    std::stringstream ss(text);
    std::string line;
    std::string word;
    std::string spaces;

    prevent prev;

    for (const auto& symbol : text) {
        if (symbol == ' ') {
            if (prev == no_space) {
                if (utf8_strlen(line) + utf8_strlen(word) > width) {
                    fitted.push_back(line);
                    line = "";
                }
                line += word;
                word = "";
            } else {
                if (spaces.length() == width) {
                    fitted.push_back(line);
                    line = spaces;
                    fitted.push_back(line);
                    line = "";
                    spaces = "";
                }
            }
            spaces += ' ';
            prev = space;
        } else {
            if (prev == space) {
                if (utf8_strlen(line) + spaces.length() > width) {
                    line.append(width - utf8_strlen(line), ' ');
                    spaces = spaces.substr(width - utf8_strlen(line));
                    fitted.push_back(line);
                    line = "";
                }
                line += spaces;
                spaces = "";
            } else {
                if (utf8_strlen(word) == width) {
                    fitted.push_back(line);
                    line = word;
                    fitted.push_back(line);
                    line = "";
                    word = "";
                }
            }

            word += symbol;
            prev = no_space;
        }
    }

    if (prev == no_space) {
        if (utf8_strlen(line) + utf8_strlen(word) > width) {
            fitted.push_back(line);
            line = "";
        }
        line += word;
        fitted.push_back(line);
    }
    if (prev == space) {
        if (utf8_strlen(line) + spaces.length() > width) {
            fitted.push_back(line);
            line = spaces.substr(width - utf8_strlen(line));
            fitted.push_back(line);
        } else {
            line += spaces;
            fitted.push_back(line);
        }
    }

    return fitted;
}

std::vector<std::string> parse_messages(const std::string& messages, const int& width)
{
    std::vector<std::string> parsed;
    std::stringstream ss(messages);
    std::string time;
    std::string name;
    std::string text;
    std::string is_file;
    std::string current;
    std::vector<std::string> fitted;

    getline(ss, name, '\036');
    getline(ss, name, '\036');

    while (getline(ss, time, '\036')) {
        getline(ss, name, '\036');
        getline(ss, text, '\036');
        getline(ss, is_file, '\036');

        current = '[' + time;
        current += ':' + name;
        if (is_file == "1")
            current += " (file)";
        current += ']';
        parsed.push_back(current);

        fitted = fit_text(text, width);
        parsed.insert(parsed.end(), fitted.begin(), fitted.end());
    }

    return parsed;
}

void print_messages(const std::string& messages)
{
    system("clear");
    struct winsize w;
    ioctl(STDOUT_FILENO, TIOCGWINSZ, &w);
    size_t width = w.ws_col;
    size_t height = w.ws_row;
    std::vector<std::string> parsed = parse_messages(messages, width);
    size_t start = parsed.size() >= (height - 3) ? parsed.size() - (height - 3) : 0;

    for (size_t i = start; i < parsed.size(); ++i) {
        std::cout << parsed[i] << '\n';
    }

    std::lock_guard lock(input_mutex);
    move_cursor(height, 0);
    std::cout << input_buffer;
    std::cout << std::string(width - utf8_strlen(input_buffer), ' ');
    move_cursor(height, utf8_strlen(input_buffer) % width + 1);

    fflush(stdout);
}

std::vector<std::string> parse_files(const std::string& messages, const int& width)
{
    std::vector<std::string> parsed;
    std::stringstream ss(messages);
    std::string time;
    std::string name;
    std::string text;
    std::string is_file;
    std::string current;
    std::vector<std::string> fitted;

    getline(ss, name, '\036');
    getline(ss, name, '\036');

    while (getline(ss, time, '\036')) {
        getline(ss, name, '\036');
        getline(ss, text, '\036');
        getline(ss, is_file, '\036');

        current = '[' + time;
        current += ':' + name;
        if (is_file == "1") {
            current += " (file)";
            current += ']';
            parsed.push_back(current);

            fitted = fit_text(text, width);
            parsed.insert(parsed.end(), fitted.begin(), fitted.end());
        }
    }

    return parsed;
}

void print_files(const std::string& messages)
{
    system("clear");
    struct winsize w;
    ioctl(STDOUT_FILENO, TIOCGWINSZ, &w);
    size_t width = w.ws_col;
    size_t height = w.ws_row;
    std::vector<std::string> parsed = parse_files(messages, width);
    size_t start = parsed.size() >= (height - 3) ? parsed.size() - (height - 3) : 0;

    for (size_t i = start; i < parsed.size(); ++i) {
        std::cout << parsed[i] << '\n';
    }

    std::lock_guard lock(input_mutex);
    move_cursor(height, 0);
    std::cout << input_buffer;
    std::cout << std::string(width - utf8_strlen(input_buffer), ' ');
    move_cursor(height, utf8_strlen(input_buffer) % width + 1);

    fflush(stdout);
}

bool is_current_user(const std::string& str)
{
    std::stringstream ss(str);
    std::string first;
    std::string second;

    getline(ss, first, '\036');
    getline(ss, second, '\036');

    return first == current_user || second == current_user;
}

void drawing()
{
    for (;;) {
        std::unique_lock lock(chat_mutex);
        cv_chat.wait(lock);
        lock.unlock();
        if (end)
            break;

        if (global_buffer[0] == messages) {
            {
                std::lock_guard lock(chat_mutex);
                if (current_state == dialogue && is_current_user(&global_buffer[1]))
                    print_messages(&global_buffer[1]);
                if (current_state == files_list && is_current_user(&global_buffer[1]))
                    print_files(&global_buffer[1]);
            }
        } else if (global_buffer[0] == users) {
            {
                std::lock_guard lock(chat_mutex);
                if (current_state == users_list)
                    print_users(&global_buffer[1]);
            }
        }
    }
}

void receiving(int client_socket)
{
    std::string received;
    while (true) {
        size_t bytes_received = my_recv(client_socket, received);
        if (bytes_received < 0 && !end) {
            std::cerr << "Сервер не отвечает\n";
            end = true;
        }
        if (end)
            break;

        if (received == file_not_found) {
            std::lock_guard lk(input_mutex);
            std::cout << "File not found";
            std::cout << input_buffer;
            std::cout.flush();
            continue;
        }

        if (received[0] == file) {
            recv_file(client_socket, receiving_file);
        }

        {
            std::lock_guard lock(chat_mutex);
            global_buffer = received;
        }
        cv_chat.notify_one();
    }
}

std::string skip_spaces(const std::string& str)
{
    for (size_t i = 0; i < str.size(); i++) {
        if (str[i] != ' ')
            return str.substr(i, str.size());
    }
    return str;
}

void utf8_pop_back(std::string& s)
{
    if (s.empty())
        return;

    size_t pos = s.size() - 1;
    size_t erase_len = 1;

    while (pos > 0 && (s[pos] & 0xC0) == 0x80) {
        --pos;
        ++erase_len;
    }

    if (pos < s.size()) {
        const unsigned char first = s[pos];
        if ((first & 0xF8) == 0xF0)
            erase_len = 4;
        else if ((first & 0xF0) == 0xE0)
            erase_len = 3;
        else if ((first & 0xE0) == 0xC0)
            erase_len = 2;
    }

    if (erase_len > s.size() - pos) {
        erase_len = s.size() - pos;
    }

    s.resize(s.size() - erase_len);
}

std::string get_file_name(const std::string& path)
{
    return path.substr(path.find_last_of('/') + 1);
}

int main(int argc, char* argv[])
{
    int client_socket;
    hostent* hp;

    if (argc != 5) {
        std::cout << std::format("Usage: {} hostname <reg/log> <login> <password>\n", argv[0]);
        return 1;
    }

    if ((client_socket = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        std::cerr << "Error: can't get socket\n";
        return 1;
    }

    sockaddr_in server_address;
    memset((char*)&server_address, '\0', sizeof(server_address));
    server_address.sin_family = AF_INET;
    hp = gethostbyname(argv[1]);
    memcpy(&server_address.sin_addr, hp->h_addr_list[0], hp->h_length);
    server_address.sin_port = htons(PORT);

    if (connect(client_socket, (sockaddr*)&server_address, sizeof(server_address)) < 0) {
        std::cerr << "Error: can't connect to server\n";
        close(client_socket);
        return 1;
    }

    std::cout << "Connection established\n\n";

    signal(SIGPIPE, SIG_IGN);

    std::string received;

    std::string sending;
    std::string getting;

    std::string type(argv[2]);
    std::string login(argv[3]);
    std::string password(argv[4]);

    if (type == "reg")
        sending += registration;
    if (type == "log")
        sending += logining;

    sending += login + '\036' + password;
    std::cout << login << ":" << password << '\n';
    my_send(client_socket, sending);
    my_recv(client_socket, received);

    switch (sending[0]) {
    case registration:
        if (received == login_exists) {
            std::cout << "Пользователь с таким логином уже существует\n";
            close(client_socket);
            return 0;
        }
    case logining:
        if (received == login_dont_exists) {
            std::cout << "Пользователя с таким логином не существует\n";
            close(client_socket);
            return 0;
        } else if (received == incorrect_password) {
            std::cout << "Направильный пароль\n";
            close(client_socket);
            return 0;
        }
    }
    if (received == db_fault) {
        std::cout << "Ошибка в работе базы данных\n";
        close(client_socket);
        return 0;
    }

    print_users(received.substr(1));

    std::thread drawing_thread(drawing);
    std::thread receiving_thread(receiving, client_socket);

    std::string filepath;

    enableRawMode();

    while (true) {
        struct winsize w;
        ioctl(STDOUT_FILENO, TIOCGWINSZ, &w);

        if (end)
            break;
        char c;
        if (read(STDIN_FILENO, &c, 1) != 1)
            break;

        if (c == '\n') {
            if (input_buffer == "Cannot open file") {
                std::lock_guard lock(input_mutex);
                input_buffer = "";
            } else if (input_buffer == "Wrong username") {
                std::lock_guard lock(input_mutex);
                input_buffer = "";
            }

            std::string message;
            {
                std::lock_guard lock(input_mutex);
                message = input_buffer;
                input_buffer.clear();
            }

            if (!message.empty()) {
                std::string sending;
                bool is_command = false;

                {
                    std::lock_guard lock(chat_mutex);

                    if (message == "/users") {
                        current_state = users_list;
                        current_user = "";
                        sending = get_users;
                        is_command = true;
                    } else if (message.substr(0, 7) == "/select" && current_state == users_list) {
                        sending = get_messages;
                        sending += skip_spaces(message.substr(7));
                        current_state = dialogue;
                        current_user = sending.substr(1);
                        is_command = true;
                    } else if (message.substr(0, 6) == "/files" && current_state == dialogue) {
                        sending = get_messages;
                        sending += current_user;
                        current_state = files_list;
                        is_command = true;
                    } else if (message.substr(0, 9) == "/messages" && current_state == files_list) {
                        sending = get_messages;
                        sending += current_user;
                        current_state = dialogue;
                        is_command = true;
                    } else if (message.substr(0, 5) == "/send" && (current_state == files_list || current_state == dialogue)) {
                        filepath = skip_spaces(message.substr(5));
                        if (!std::filesystem::exists(filepath)) {
                            input_buffer = "Cannot open file";
                        } else {
                            sending = file;
                            sending += get_file_name(filepath);
                        }
                    } else if (message.substr(0, 4) == "/get" && (current_state == files_list || current_state == dialogue)) {
                        receiving_file = skip_spaces(message.substr(4));
                        receiving_file = receiving_file.substr(0, receiving_file.find_first_of(' '));
                        std::string username = message.substr(message.find_last_of(' ') + 1);
                        if (username == login || username == current_user) {
                            sending = get_file;
                            sending += receiving_file + '\036' + username;
                            is_command = false;
                        } else {
                            input_buffer = "Wrong username";
                        }
                    } else if (message == "/exit") {
                        end = true;
                        break;
                    } else if (current_state == dialogue) {
                        sending = std::string(1, send_message) + message;
                        is_command = false;
                    } else if (current_state == users_list) {
                        sending = get_users;
                    }
                }

                if (!sending.empty()) {
                    my_send(client_socket, sending);
                    if (sending[0] == file) {
                        std::lock_guard lk(input_mutex);
                        move_cursor(w.ws_row, 0);
                        std::cout << "Sending...";
                        std::cout << std::string(w.ws_col - 10, ' ');
                        std::cout << input_buffer;
                        std::cout.flush();
                        send_file(client_socket, filepath);
                    }
                    if (is_command) {
                        std::lock_guard lock(chat_mutex);
                        if (current_state == users_list) {
                            my_send(client_socket, sending);
                        } else if (current_state == dialogue) {
                            my_send(client_socket, sending);
                        }
                    }
                }
            }
        } else if (c == 127) {
            std::lock_guard lock(input_mutex);
            utf8_pop_back(input_buffer);
        } else if (c >= 1 && c <= 31) {
            continue;
        } else if (input_buffer == "Cannot open file") {
            std::lock_guard lock(input_mutex);
            input_buffer = c;
        } else if (input_buffer == "Wrong username") {
            std::lock_guard lock(input_mutex);
            input_buffer = c;
        } else {
            std::lock_guard lock(input_mutex);
            input_buffer += c;
        }

        move_cursor(w.ws_row, 0);
        std::cout << std::string(w.ws_col, ' ');
        std::vector<std::string> fitted = fit_text(input_buffer, w.ws_col);
        move_cursor(w.ws_row - fitted.size() + 1, 0);
        for (size_t i = 0; i < fitted.size(); i++) {
            std::cout << fitted[i];
            if (i != fitted.size() - 1)
                std::cout << std::string(w.ws_col - utf8_strlen(fitted[i]), ' ');
        }

        fflush(stdout);
    }

    cv_chat.notify_all();
    drawing_thread.join();
    shutdown(client_socket, SHUT_RD);
    receiving_thread.join();
    printf("\nConnection closed\n");
}