#include <iostream>
#include <ncurses.h>
#include <string>

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

WINDOW* create_newwin(int height, int width, int starty, int startx);
void destroy_win(WINDOW* local_win);

void GUI()
{
    initscr(); /* Start curses mode 		*/
    cbreak();
    keypad(stdscr, TRUE);
    noecho();
    clear();
    refresh();
    WINDOW* users = create_newwin(LINES, COLS / 3, 0, 0);
    WINDOW* messages = create_newwin(LINES - 3, COLS / 3, 0, COLS / 3);
    WINDOW* input = create_newwin(3, COLS / 3, LINES - 3, COLS / 3);
    WINDOW* files = create_newwin(LINES, COLS / 3, 0, COLS / 3 * 2);
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