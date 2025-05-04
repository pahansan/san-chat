

#include "interface.hpp"
#include "parsing.hpp"
#include "terminal.hpp"
#include "utf8_string.hpp"

#include "mutex"
#include <string>
#include <vector>

std::mutex input_mutex;
std::string input_buffer;

size_t user_input_height = 1;

void update_user_input()
{
    const auto [width, height] = get_terminal_size();

    std::lock_guard lock(input_mutex);
    std::vector<std::string> fitted = fit_text(input_buffer, width - 1);

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
    system("clear");

    const auto [width, height] = get_terminal_size();
    const users_map users = parse_users(users_string);

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
    system("clear");
    const auto [width, height] = get_terminal_size();

    std::vector<std::string> parsed = parse_messages(messages, width);
    size_t start = parsed.size() >= (height - 3) ? parsed.size() - (height - 3) : 0;

    for (size_t i = start; i < parsed.size(); ++i) {
        std::cout << parsed[i] << '\n';
    }

    fflush(stdout);
}

void print_files(const std::string& messages)
{
    system("clear");
    const auto [width, height] = get_terminal_size();

    std::vector<std::string> parsed = parse_files(messages, width);
    size_t start = parsed.size() >= (height - 3) ? parsed.size() - (height - 3) : 0;

    for (size_t i = start; i < parsed.size(); ++i) {
        std::cout << parsed[i] << '\n';
    }

    fflush(stdout);
}