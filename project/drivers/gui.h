#ifndef GUI_H
#define GUI_H

#include <stdint.h>

// Screen dimensions and color constants (customize as needed)
#define VGA_WIDTH 80
#define VGA_HEIGHT 25
#define VGA_ADDRESS 0xB8000
#define VGA_COLOR_WHITE_ON_BLACK 0x0F

// Function declarations
void gui_clear_screen(void);
void gui_set_color(uint8_t color);
void gui_set_cursor(uint8_t row, uint8_t col);

#endif // GUI_H
