#include <stdint.h>
#include "gui.h"
#include "print.h"

static uint16_t* vga_buffer = (uint16_t*) VGA_ADDRESS;
extern uint8_t cursor_row;
extern uint8_t cursor_col;
static uint8_t vga_color = VGA_COLOR_WHITE_ON_BLACK;

void gui_clear_screen(void) {
    for (int row = 0; row < VGA_HEIGHT; row++) {
        for (int col = 0; col < VGA_WIDTH; col++) {
            vga_buffer[row * VGA_WIDTH + col] = ((uint16_t)vga_color << 8) | ' ';
        }
    }
    cursor_row = 0;
    cursor_col = 0;
}

void gui_set_color(uint8_t color) {
    vga_color = color;
}

void gui_set_cursor(uint8_t row, uint8_t col) {
    cursor_row = row;
    cursor_col = col;
}