#include "keyboard.h"
#include "port_io.h"
#include "print.h"

#define KEYBOARD_DATA_PORT 0x60

static char scancode_to_ascii[128] = {
    0,  27, '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '-', '=', '\b',
    '\t', 'q', 'w', 'e', 'r', 't', 'y', 'u', 'i', 'o', 'p', '[', ']', '\n', 0,
    'a', 's', 'd', 'f', 'g', 'h', 'j', 'k', 'l', ';', '\'', '`',  0, '\\',
    'z', 'x', 'c', 'v', 'b', 'n', 'm', ',', '.', '/', 0, '*',
    0,  ' ', 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
};

char keyboard_getchar(void) {
    uint8_t scancode = 0;
    while (1) {
        scancode = inb(KEYBOARD_DATA_PORT);
        if ((scancode & 0x80) == 0) break; 
    }

    char c = scancode_to_ascii[scancode];
    while ((inb(KEYBOARD_DATA_PORT) & 0x80) == 0) {
    }

    return c;
}


void input(const char* prompt, char* buffer, int max_length) {
    print(prompt);
    update_hardware_cursor(cursor_row, cursor_col);

    int i = 0;
    while (i < max_length - 1) {
        char c = keyboard_getchar();

        if (c == '\n') {
            print_char('\n');
            break;
        } else if (c == '\b' && i > 0) {
            i--;
            print_char('\b');
            print_char(' ');
            print_char('\b');
        } else if (c >= 32 && c <= 126) {
            buffer[i++] = c;
            print_char(c);
        }

        update_hardware_cursor(cursor_row, cursor_col);
    }

    buffer[i] = '\0';
}

