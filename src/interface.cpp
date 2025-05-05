

#include "interface.hpp"
#include "message_types.hpp"
#include "parsing.hpp"
#include "sendrecv.hpp"
#include "terminal.hpp"
#include "utf8_string.hpp"

#include <format>
#include <mutex>
#include <string>
#include <unistd.h>
#include <vector>

std::string current_user = "";

user_state current_state = users_list;
std::mutex chat_mutex;

std::mutex input_mutex;
std::string input_buffer;

std::string global_buffer;
std::string receiving_file = "";

size_t user_input_height = 1;

void update_user_input()
{
    const auto [width, height] = get_terminal_size();
    std::vector<std::string> fitted = fit_text(get_input_buffer(), width - 1);

    std::lock_guard lock(chat_mutex);

    move_cursor(height, 0);
    std::cout << std::string(width, ' ');

    move_cursor(height - user_input_height + 1, 0);

    for (size_t i = user_input_height; i > fitted.size(); i--)
        std::cout << std::string(width, ' ');

    move_cursor(height - fitted.size() + 1, 0);

    for (size_t i = 0; i < fitted.size(); i++) {
        std::cout << fitted[i];
        if (i != fitted.size() - 1)
            std::cout << std::string(width - utf8_strlen(fitted[i]), ' ');
    }

    user_input_height = fitted.size();

    fflush(stdout);
}

void print_users(const std::string& users_string)
{
    const auto [width, height] = get_terminal_size();
    const users_map users = parse_users(users_string);

    std::lock_guard lock(chat_mutex);

    system("clear");

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

    fflush(stdout);
}

void print_messages(const std::string& messages)
{
    const auto [width, height] = get_terminal_size();

    std::lock_guard lock(chat_mutex);

    system("clear");

    std::vector<std::string> parsed = parse_messages(messages, width);
    size_t start = parsed.size() >= (height - 3) ? parsed.size() - (height - 3) : 0;

    for (size_t i = start; i < parsed.size(); ++i) {
        std::cout << parsed[i] << '\n';
    }

    fflush(stdout);
}

void print_files(const std::string& messages)
{
    const auto [width, height] = get_terminal_size();

    std::lock_guard lock(chat_mutex);

    system("clear");

    std::vector<std::string> parsed = parse_files(messages, width);
    size_t start = parsed.size() >= (height - 3) ? parsed.size() - (height - 3) : 0;

    for (size_t i = start; i < parsed.size(); ++i) {
        std::cout << parsed[i] << '\n';
    }

    fflush(stdout);
}

void set_input_buffer(const std::string& str)
{
    std::lock_guard lock(input_mutex);
    input_buffer = str;
}

void set_input_buffer(const char& c)
{
    std::lock_guard lock(input_mutex);
    input_buffer = c;
}

std::string get_input_buffer()
{
    std::lock_guard lock(input_mutex);
    return input_buffer;
}

std::string intrance(const int& fd, const std::string& app_name, const std::string& type, const std::string& login, const std::string& password)
{
    std::string sending;
    std::string received;

    if (type == "reg") {
        sending += registration;
    } else if (type == "log") {
        sending += logining;
    } else {
        std::cout << std::format("Usage: {} hostname <reg/log> <login> <password>\n", app_name);
        return "";
    }

    sending += login + '\036' + password;

    my_send(fd, sending);
    my_recv(fd, received);

    if (received == login_exists) {
        std::cout << "Пользователь с таким логином уже существует\n";
        close(fd);
        return "";
    } else if (received == login_dont_exists) {
        std::cout << "Пользователя с таким логином не существует\n";
        close(fd);
        return "";
    } else if (received == incorrect_password) {
        std::cout << "Направильный пароль\n";
        close(fd);
        return "";
    } else if (received == db_fault) {
        std::cout << "Ошибка в работе базы данных\n";
        close(fd);
        return "";
    } else if (received == user_already_connected) {
        std::cout << "Пользователь уже подключен в другом месте\n";
        close(fd);
        return "";
    } else if (received[0] != users) {
        std::cout << "Некорректный ответ от сервера\n";
        close(fd);
        return "";
    }

    return received;
}

user_state get_current_state()
{
    std::lock_guard lock(chat_mutex);
    return current_state;
}

void set_current_state(const user_state& state)
{
    std::lock_guard lock(chat_mutex);
    current_state = state;
}

std::string get_global_buffer_substr(const size_t& start)
{
    std::lock_guard lock(chat_mutex);
    return global_buffer.substr(start);
}

void set_global_buffer(const std::string& str)
{
    std::lock_guard lock(chat_mutex);
    global_buffer = str;
}

std::string get_receiving_file()
{
    std::lock_guard lock(chat_mutex);
    return receiving_file;
}

void set_receiving_file(const std::string& str)
{
    std::lock_guard lock(chat_mutex);
    receiving_file = str;
}

std::string get_current_user()
{
    std::lock_guard lock(chat_mutex);
    return current_user;
}

void set_current_user(const std::string& str)
{
    std::lock_guard lock(chat_mutex);
    current_user = str;
}

void print_notification(const std::string& str)
{
    set_input_buffer(str);
    update_user_input();
    set_input_buffer("");
}
