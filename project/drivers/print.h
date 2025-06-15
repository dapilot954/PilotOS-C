#ifndef PRINT_H
#define PRINT_H

#include <stdint.h>

void print(const char* str);
void print_char(char c);
void update_hardware_cursor(uint8_t row, uint8_t col);

extern uint8_t cursor_row;
extern uint8_t cursor_col;

#endif
