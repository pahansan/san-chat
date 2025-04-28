#include <iostream>
#include <ncurses.h>
#include <string>
#include <vector>

#include "db.hpp"

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
    mvwprintw(local_win, starty, center_start_coordinate(width, header.size()), "%s", header.c_str());
    wrefresh(local_win);
    return local_win;
}

WINDOW* create_users_win()
{
    return create_win_with_header(LINES, COLS / 3, 0, 0, "users");
}

WINDOW* create_messages_win()
{
    return create_win_with_header(LINES - 3, COLS / 3, 0, COLS / 3, "messages");
}

WINDOW* create_input_win()
{
    return create_win_with_header(3, COLS / 3, LINES - 3, COLS / 3, "input");
}

WINDOW* create_file_win()
{
    return create_win_with_header(LINES, COLS / 3, 0, COLS / 3 * 2, "files");
}

std::string list_users(WINDOW* users_win, int begin, users_map users_list, int selected)
{
    int x, y;
    getbegyx(users_win, y, x);
    ++x;
    ++y;
    std::vector<std::string> printable;
    std::string tmp;
    std::string selected_string;
    int n = 0;

    for (const auto& [login, status] : users_list) {
        tmp = login + ":";
        if (status == OFFLINE)
            tmp += "offline";
        else
            tmp += "online";
        printable.push_back(tmp);
        if (n++ == selected)
            selected_string = login;
    }

    // init_pair(1, COLOR_RED, COLOR_BLUE);

    for (size_t i = begin; i < printable.size() && y < getmaxy(users_win) - 1; i++) {
        if (i == selected) {
            mvwprintw(users_win, y++, x, "%s <", printable[i].c_str());
        } else {
            mvwprintw(users_win, y++, x, "%s", printable[i].c_str());
        }
    }
    wrefresh(users_win);
    return selected_string;
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
    WINDOW* users = create_win_with_header(LINES, COLS / 3, 0, 0, "users");
    WINDOW* messages = create_win_with_header(LINES - 3, COLS / 3, 0, COLS / 3, "messages");
    WINDOW* input = create_win_with_header(3, COLS / 3, LINES - 3, COLS / 3, "input");
    WINDOW* files = create_win_with_header(LINES, COLS / 3, 0, COLS / 3 * 2, "files");
    users_map users_list = {{"aboba", 1}, {"pasha", 1}, {"nikitka", 0}};
    list_users(users, 0, users_list, 0);
    int ch;
    while ((ch = getch()) != 'q') {
        // Wait for 'q' to exit
        wrefresh(users); // Ensure the window remains visible
    }

    endwin();
}

int main(int argc, char* argv[])
{
    // WINDOW* my_win;
    // int startx, starty, width, height;
    // int ch;

    // initscr();            /* Start curses mode 		*/
    // cbreak();             /* Line buffering disabled, Pass on
    //                        * everty thing to me 		*/
    // keypad(stdscr, TRUE); /* I need that nifty F1 	*/

    // height = 3;
    // width = 10;
    // starty = (LINES - height) / 2; /* Calculating for a center placement */
    // startx = (COLS - width) / 2;   /* of the window		*/
    // printw("Press F1 to exit");
    // refresh();
    // my_win = create_newwin(height, width, starty, startx);

    // while ((ch = getch()) != KEY_F(1)) {
    //     switch (ch) {
    //     case KEY_LEFT:
    //         destroy_win(my_win);
    //         my_win = create_newwin(height, width, starty, --startx);
    //         break;
    //     case KEY_RIGHT:
    //         destroy_win(my_win);
    //         my_win = create_newwin(height, width, starty, ++startx);
    //         break;
    //     case KEY_UP:
    //         destroy_win(my_win);
    //         my_win = create_newwin(height, width, --starty, startx);
    //         break;
    //     case KEY_DOWN:
    //         destroy_win(my_win);
    //         my_win = create_newwin(height, width, ++starty, startx);
    //         break;
    //     }
    // }

    // endwin(); /* End curses mode		  */
    GUI();
    return 0;
}
