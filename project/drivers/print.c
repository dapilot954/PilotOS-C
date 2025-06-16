#include "stdint.h"
#include "print.h"
#include "port_io.h"

#define VGA_ADDRESS 0xB8000
#define VGA_WIDTH 80
#define VGA_HEIGHT 25

uint8_t cursor_row = 0;
uint8_t cursor_col = 0;
typedef unsigned int size_t;
uint16_t* vga_buffer = (uint16_t*) VGA_ADDRESS;
uint8_t vga_color = 0x0F; 

void print_char(char c) {
    if (c == '\r') {
        return; 
    }

    if (c == '\n') {
        cursor_col = 0;
        cursor_row++;
    } else if (c == '\b') {
        if (cursor_col > 0) {
            cursor_col--;
        } else if (cursor_row > 0) {
            cursor_row--;
            cursor_col = VGA_WIDTH - 1;
        }
        const size_t index = cursor_row * VGA_WIDTH + cursor_col;
        vga_buffer[index] = ((uint16_t)vga_color << 8) | ' ';
    } else {
        const size_t index = cursor_row * VGA_WIDTH + cursor_col;
        vga_buffer[index] = ((uint16_t)vga_color << 8) | c;
        cursor_col++;
        if (cursor_col >= VGA_WIDTH) {
            cursor_col = 0;
            cursor_row++;
        }
    }

    // Scroll if needed
    if (cursor_row >= VGA_HEIGHT) {
        for (int row = 1; row < VGA_HEIGHT; row++) {
            for (int col = 0; col < VGA_WIDTH; col++) {
                vga_buffer[(row - 1) * VGA_WIDTH + col] = vga_buffer[row * VGA_WIDTH + col];
            }
        }
        for (int col = 0; col < VGA_WIDTH; col++) {
            vga_buffer[(VGA_HEIGHT - 1) * VGA_WIDTH + col] = ((uint16_t)vga_color << 8) | ' ';
        }

        cursor_row = VGA_HEIGHT - 1;
    }
}


void print(const char* str) {
    while (*str) {
        print_char(*str++);
    }
}


void update_hardware_cursor(uint8_t row, uint8_t col) {
    uint16_t pos = row * VGA_WIDTH + col;
    outb(0x3D4, 0x0E);
    outb(0x3D5, (pos >> 8) & 0xFF);
    outb(0x3D4, 0x0F);
    outb(0x3D5, pos & 0xFF);
}

void print_uint(uint32_t n) {
    char buf[11];
    int i = 10;
    buf[i--] = '\0';
    if (n == 0) {
        buf[i] = '0';
        print(&buf[i]);
        return;
    }
    while (n && i >= 0) {
        buf[i--] = '0' + (n % 10);
        n /= 10;
    }
    print(&buf[i + 1]);
}

void print_hex16(uint16_t n) {
    char hex[5];
    hex[4] = '\0';
    for (int i = 3; i >= 0; i--) {
        hex[i] = "0123456789ABCDEF"[n & 0xF];
        n >>= 4;
    }
    print(hex);
}

void print_hex32(uint32_t val) {
    const char* hex = "0123456789ABCDEF";
    char str[9];
    for (int i = 0; i < 8; i++) {
        str[7 - i] = hex[(val >> (i * 4)) & 0xF];
    }
    str[8] = '\0';
    print(str);
}