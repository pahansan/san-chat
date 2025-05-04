#pragma once

#include <cstddef>
#include <tuple>

void disableRawMode();
void enableRawMode();
void move_cursor(int row, int col);
std::tuple<size_t, size_t> get_terminal_size();
