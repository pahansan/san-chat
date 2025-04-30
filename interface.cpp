#include <iostream>
#include <ncurses.h>
#include <sstream>
#include <string>
#include <vector>

#include "db.hpp"

enum prevent { space, no_space };

void user_input_example()
{
    int ch = 0;
    std::string input;
    std::string char_str;
    int y = 0;
    int x = 0;

    initscr();
    cbreak();
    keypad(stdscr, TRUE);
    noecho();

    while (ch != '\n') {
        ch = getch();
        if (std::isalnum(ch)) {
            char_str = ch;
            input.insert(x, char_str);
            x++;
            mvaddstr(0, 0, input.c_str());
            move(y, x);
        }
        if (ch == KEY_LEFT) {
            if (x != 0)
                move(y, --x);
        }
        if (ch == KEY_RIGHT) {
            if (x != input.size())
                move(y, ++x);
        }
        if (ch == KEY_DOWN) {
            x = input.size();
            move(y, x);
        }
        refresh();
    }
    endwin();
}

WINDOW* create_newwin(int height, int width, int starty, int startx)
{
    WINDOW* local_win;

    local_win = newwin(height, width, starty, startx);
    box(local_win, 0, 0); /* 0, 0 gives default characters
                           * for the vertical and horizontal
                           * lines			*/
    wrefresh(local_win);  /* Show that box 		*/

    return local_win;
}

void destroy_win(WINDOW* local_win)
{
    /* box(local_win, ' ', ' '); : This won't produce the desired
     * result of erasing the window. It will leave it's four corners
     * and so an ugly remnant of window.
     */
    wborder(local_win, ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ');
    /* The parameters taken are
     * 1. win: the window on which to operate
     * 2. ls: character to be used for the left side of the window
     * 3. rs: character to be used for the right side of the window
     * 4. ts: character to be used for the top side of the window
     * 5. bs: character to be used for the bottom side of the window
     * 6. tl: character to be used for the top left corner of the window
     * 7. tr: character to be used for the top right corner of the window
     * 8. bl: character to be used for the bottom left corner of the window
     * 9. br: character to be used for the bottom right corner of the window
     */
    wrefresh(local_win);
    delwin(local_win);
}

int center_start_coordinate(int main_block_size, int sub_block_size)
{
    return main_block_size / 2 - sub_block_size / 2;
}

WINDOW* create_win_with_header(int height, int width, int starty, int startx, const std::string& header)
{
    WINDOW* local_win = create_newwin(height, width, starty, startx);
    mvwprintw(local_win, 0, center_start_coordinate(width, header.size()), "%s", header.c_str());
    wrefresh(local_win);
    return local_win;
}

WINDOW* create_users_win()
{
    return create_win_with_header(LINES, COLS / 3, 0, 0, "users");
}

WINDOW* create_messages_win(const std::string& login, int height = LINES - 3)
{
    if (login == "")
        return create_win_with_header(height, COLS / 3, 0, COLS / 3, "messages");
    else
        return create_win_with_header(height, COLS / 3, 0, COLS / 3, "messages~" + login);
}

WINDOW* create_input_win(int height = 3)
{
    return create_win_with_header(height, COLS / 3, LINES - height, COLS / 3, "input");
}

WINDOW* create_file_win()
{
    return create_win_with_header(LINES, COLS / 3, 0, COLS / 3 * 2, "files");
}

void blink_win(WINDOW* win)
{
    for (int x = 1; x < getmaxx(win) - 1; x++) {
        for (int y = 1; y < getmaxy(win) - 1; y++) {
            mvwaddch(win, y, x, ACS_CKBOARD);
        }
        wrefresh(win);
    }
    for (int x = 1; x < getmaxx(win) - 1; x++) {
        for (int y = 1; y < getmaxy(win) - 1; y++) {
            mvwaddch(win, y, x, ' ');
        }
        wrefresh(win);
    }
}

void my_clear(WINDOW* win)
{
    for (int x = 1; x < getmaxx(win) - 1; x++) {
        for (int y = 1; y < getmaxy(win) - 1; y++) {
            mvwaddch(win, y, x, ' ');
        }
    }
    wrefresh(win);
}

std::string list_users(WINDOW* win, int begin, users_map users_list, int selected)
{
    int x = 1, y = 1;

    std::vector<std::string> printable;
    std::string tmp;
    std::string selected_string;
    int n = 0;

    size_t width = getmaxx(win);

    for (const auto& [login, status] : users_list) {
        tmp = login + ":";
        if (status == OFFLINE)
            tmp += "offline";
        else
            tmp += "online";
        if (n++ == selected) {
            tmp += " <--";
            selected_string = login;
        }
        tmp.append(width - tmp.size() - 2, ' ');
        printable.push_back(tmp);
    }

    for (size_t i = begin; i < printable.size() && y < getmaxy(win) - 1; i++) {
        mvwprintw(win, y++, x, "%s", printable[i].c_str());
    }
    wrefresh(win);
    return selected_string;
}

std::string get_file_name(const std::string& path)
{
    std::stringstream ss(path);
    std::string tmp;
    while (getline(ss, tmp, '/'))
        ;
    return tmp;
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
                if (line.size() + word.size() > width - 2) {
                    fitted.push_back(line);
                    line = "";
                }
                line += word;
                word = "";
            } else {
                if (spaces.size() == width - 2) {
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
                if (line.size() + spaces.size() > width - 2) {
                    line.append(width - 2 - line.size(), ' ');
                    spaces = spaces.substr(width - 2 - line.size());
                    fitted.push_back(line);
                    line = "";
                }
                line += spaces;
                spaces = "";
            } else {
                if (word.size() == width - 2) {
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
        if (line.size() + word.size() > width - 2) {
            fitted.push_back(line);
            line = "";
        }
        line += word;
        fitted.push_back(line);
    }
    if (prev == space) {
        if (line.size() + spaces.size() > width - 2) {
            fitted.push_back(line);
            line = spaces.substr(width - 2 - line.size());
            fitted.push_back(line);
        } else {
            line += spaces;
            fitted.push_back(line);
        }
    }

    return fitted;
}

std::vector<std::string> format_messages(WINDOW* win, const messages_list& list)
{
    std::vector<std::string> messages;
    std::string tmp;
    std::vector<std::string> fitted_text;

    for (const auto& message : list) {
        tmp = "[" + message.sender + ":" + message.time;
        if (message.is_file)
            tmp += ":file";
        tmp += "]";
        messages.push_back(tmp);
        fitted_text = fit_text(message.text, getmaxx(win));
        messages.insert(messages.end(), fitted_text.begin(), fitted_text.end());
    }

    return messages;
}

void list_messages(WINDOW* win, int& begin, const std::vector<std::string>& formatted)
{
    int x = 1, y = 1;

    for (size_t i = begin; i < formatted.size() && y < getmaxy(win) - 1; i++) {
        mvwprintw(win, y++, x, "%s", formatted[i].c_str());
    }
    wrefresh(win);
}

std::string users_input(WINDOW* win, const users_map& users_list, int& begin, int& selected)
{
    std::string login = list_users(win, begin, users_list, selected);
    int ch = 0;
    while ((ch = getch()) != '\t') {
        switch (ch) {
        case KEY_DOWN:
            if (selected < users_list.size() - 1)
                ++selected;
            if (selected - begin > getmaxy(win) - 3)
                ++begin;
            login = list_users(win, begin, users_list, selected);
            break;
        case KEY_UP:
            if (selected > 0)
                --selected;
            if (selected - begin < 0)
                --begin;
            login = list_users(win, begin, users_list, selected);
            break;
        case '\n':
            return login;
        default:
            break;
        }
    }
    return "";
}

void messages_input(WINDOW* win, const messages_list& list, int& begin)
{
    std::vector<std::string> formatted = format_messages(win, list);

    if (begin == -1)
        begin = formatted.size() - getmaxy(win) + 2;

    begin = begin < 0 ? 0 : begin;

    list_messages(win, begin, formatted);

    int ch = 0;
    while ((ch = getch()) != '\t') {
        switch (ch) {
        case KEY_DOWN:
            if (formatted.size() - begin > getmaxy(win) - 2)
                ++begin;
            my_clear(win);
            list_messages(win, begin, formatted);
            break;
        case KEY_UP:
            if (begin > 0)
                --begin;
            my_clear(win);
            list_messages(win, begin, formatted);
            break;
        case '\t':
            return;
        default:
            break;
        }
    }
}

size_t actual_pos(std::vector<std::string> fitted_string, int x, int y)
{
    size_t size = 0;
    for (size_t i = 0; i < y - 1; i++)
        size += fitted_string[i].size();

    return size += x - 1;
}

std::string user_input(WINDOW* input_win, WINDOW* messages_win, const messages_list& list, int& begin, const std::string& login)
{
    int width = getmaxx(input_win);
    int height = getmaxy(input_win);
    curs_set(1);
    std::string input_string = "";
    std::vector<std::string> fitted_string;
    int ch = 0;
    std::string char_str;
    int x = 1, y = 1;
    size_t fitted_string_size = 1;

    while (ch = getch()) {
        if ((std::isalnum(ch) || std::ispunct(ch) || std::isspace(ch)) && ch != '\n') {
            char_str = ch;
            input_string.insert(actual_pos(fitted_string, x, y), char_str);
            fitted_string = fit_text(input_string, width);
            ++x;
            if (fitted_string.size() != fitted_string_size) {
                fitted_string_size = fitted_string.size();
                if (y + 1 == fitted_string_size) {
                    x = fitted_string[y].size() + 1;
                    ++y;
                }
            }
            // if (x == width - 1) {
            //     ++y;
            //     x = 1;
            // }

            my_clear(input_win);
            destroy_win(input_win);
            destroy_win(messages_win);
            messages_win = create_messages_win(login, LINES - 2 - fitted_string.size());
            input_win = create_input_win(fitted_string.size() + 2);

            std::vector<std::string> formatted = format_messages(messages_win, list);
            if (begin == -1)
                begin = formatted.size() - getmaxy(messages_win) + 2;
            begin = begin < 0 ? 0 : begin;
            list_messages(messages_win, begin, formatted);

            int output_x = 1, output_y = 1;

            for (size_t i = 0; i < fitted_string.size() && output_y < getmaxy(input_win) - 1; i++) {
                mvwprintw(input_win, output_y++, output_x, "%s", fitted_string[i].c_str());
            }

            wmove(input_win, y, x);
        }
        if (ch == KEY_LEFT) {
            if (x == 1 && y != 1) {
                --y;
                x = fitted_string[y - 1].size() + 1;
                wmove(input_win, y, x);
            } else if (x != 1)
                wmove(input_win, y, --x);
        }
        if (ch == KEY_RIGHT) {
            if ((x == (width - 2) || x == fitted_string[y - 1].size() + 1) && y != height - 1) {
                x = 1;
                ++y;
                wmove(input_win, y, x);
            } else if (x != (width - 2) && x != fitted_string[y - 1].size() + 1) {
                ++x;
                wmove(input_win, y, x);
            }
        }
        if (ch == '\t')
            return "";

        wrefresh(input_win);
    }
}

void GUI()
{
    initscr(); /* Start curses mode 		*/
    cbreak();
    keypad(stdscr, TRUE);
    noecho();
    clear();
    refresh();
    curs_set(0);
    WINDOW* users = create_users_win();
    WINDOW* messages = create_messages_win("");
    WINDOW* input = create_input_win();
    WINDOW* files = create_file_win();
    users_map users_list = {{"aboba", 1}, {"pasha", 1}, {"nikitka", 0}};
    std::string a;
    for (int i = 0; i < 40; i++) {
        users_list[std::to_string(i)] = i % 2;
    }

    messages_list list;

    for (int i = 0; i < 20; i++) {
        list.push_back(
                message{"15:00",
                        "jopa",
                        "hjfgj kljhlfgj klgj hfjglhj gkfhjl fgjhk lgjklhfklgjkh "
                        " lgfhkfg                                                                                                                         kj "
                        "lfdj "
                        "kfldfj gkfjdgopfdk "
                        "gkhjjfgghjklfgjhfldjhklgdfgjsdjgjdfkjhgjkdhgjkdhgkjsdhghdfghkdfghkdjghdfkjghdfghdiufghdfjgdhfjkgdfhgjkdfnggjkfbnnjdfk",
                        0});
    }

    list_users(users, 0, users_list, 0);
    int ch = 0;
    int begin = 0, selected = 0;
    int messages_begin = -1;

    blink_win(users);
    std::string login = users_input(users, users_list, begin, selected);
    blink_win(messages);
    destroy_win(messages);
    messages = create_messages_win(login);
    // messages_input(messages, list, messages_begin);
    user_input(input, messages, list, messages_begin, login);

    wrefresh(users);

    endwin();
}

int main(int argc, char* argv[])
{
    GUI();
    return 0;
}
