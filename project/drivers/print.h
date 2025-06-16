#ifndef PRINT_H
#define PRINT_H

#include <stdint.h>

void print(const char* str);
void print_char(char c);
void update_hardware_cursor(uint8_t row, uint8_t col);
void print_uint(uint32_t n);
void print_hex16(uint16_t n);
void print_hex32(uint32_t val);


extern uint8_t cursor_row;
extern uint8_t cursor_col;

#endif
