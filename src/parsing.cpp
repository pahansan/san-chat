#include "parsing.hpp"
#include "db.hpp"
#include "utf8_string.hpp"

#include <sstream>
#include <string>
#include <vector>

std::string current_user = "";

std::vector<std::string> get_words_n_spaces(const std::string& text)
{
    std::vector<std::string> words_n_spaces;
    std::string word, spaces;
    prevent prev = text[0] == ' ' ? space : no_space;
    for (const auto& symbol : text) {
        if (symbol == ' ') {
            if (prev == no_space) {
                words_n_spaces.push_back(word);
                word = "";
            }
            prev = space;
            spaces += symbol;
        } else {
            if (prev == space) {
                words_n_spaces.push_back(spaces);
                spaces = "";
            }
            prev = no_space;
            word += symbol;
        }
    }
    if (word != "")
        words_n_spaces.push_back(word);
    else if (spaces != "")
        words_n_spaces.push_back(spaces);

    return words_n_spaces;
}

std::vector<std::string> split_word(const std::string& word, const int& width)
{
    std::vector<std::string> splitted;
    std::string line;
    for (const auto& symbol : word) {
        line += symbol;
        if (utf8_strlen(line) == width) {
            splitted.push_back(line);
            line = "";
        }
    }
    if (line != "")
        splitted.push_back(line);
    return splitted;
}

std::vector<std::string> fit_text(const std::string& text, const int& width)
{
    std::vector<std::string> fitted;
    std::string line;

    std::vector<std::string> words_n_spaces = get_words_n_spaces(text);

    for (size_t i = 0; i < words_n_spaces.size(); i++) {
        if (utf8_strlen(words_n_spaces[i]) + utf8_strlen(line) > width) {
            if (line != "") {
                fitted.push_back(line);
                line = "";
            }
            if (utf8_strlen(words_n_spaces[i]) == width) {
                fitted.push_back(words_n_spaces[i]);
                continue;
            } else if (utf8_strlen(words_n_spaces[i]) > width) {
                std::vector<std::string> splitted = split_word(words_n_spaces[i], width);
                fitted.insert(fitted.end(), splitted.begin(), splitted.end() - 1);
                line = splitted[splitted.size() - 1];
                continue;
            }
            line += words_n_spaces[i];
        } else {
            line += words_n_spaces[i];
        }
    }
    if (line != "") {
        fitted.push_back(line);
    }
    return fitted;
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

std::string skip_spaces(const std::string& str)
{
    for (size_t i = 0; i < str.size(); i++) {
        if (str[i] != ' ')
            return str.substr(i);
    }
    return str;
}

std::string get_file_name(const std::string& path)
{
    return path.substr(path.find_last_of('/') + 1);
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