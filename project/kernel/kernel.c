#include "../drivers/print.h"
#include "stdint-gcc.h"
#include <stdint.h>
#include "../drivers/bool.h"
#include "../drivers/fat32.h"
#include "../drivers/gui.h"

//#include "../system/terminal.h"

  // Correct type now

#define SECTOR_SIZE 512

void itoa(int value, char *str, int base) {
    char *rc = str;
    char *ptr;
    char *low;
    // Handle negative numbers for base 10
    if (base == 10 && value < 0) {
        *rc++ = '-';
        value = -value;
    }

    ptr = rc;
    do {
        int digit = value % base;
        *ptr++ = (digit < 10) ? ('0' + digit) : ('A' + digit - 10);
        value /= base;
    } while (value);

    *ptr-- = '\0';

    // Reverse
    for (low = rc; low < ptr; low++, ptr--) {
        char tmp = *low;
        *low = *ptr;
        *ptr = tmp;
    }
}





__attribute__((section(".multiboot")))
const unsigned int multiboot_header[] = {
    0x1BADB002,               // magic number
    0x00,                     // flags
    -(0x1BADB002 + 0x00)      // checksum
};

void byte_to_hex(uint8_t byte, char* out) {
    const char* hex = "0123456789ABCDEF";
    out[0] = hex[(byte >> 4) & 0x0F];
    out[1] = hex[byte & 0x0F];
    out[2] = '\0';
}

void print_hex(uint8_t* buffer, int length) {
    char hex_str[3];
    for (int i = 0; i < length; i++) {
        byte_to_hex(buffer[i], hex_str);
        print(hex_str);
        print(" ");
        if ((i + 1) % 16 == 0)
            print("\n");
    }
}


void startup_sequence()
{
    print("Hello from kernel with FAT support!\n");
    print("Attempting to read boot sector\n");

    uint8_t buffer1[512];
    int ret1 = ata_pio_read(0, buffer1); // sector 0 from the FAT32 disk
    if (ret1 == 0) {
        print("Read error\n");
        return;
    }
    else
    {
        print("Data read from disk (hex):\n");
        print_hex(buffer1, 64); 
    }
    print("Read complete\n");
    
    print("attempting to init FS\n");

    fat32_init();

    fat32_list_root_dir();

    fat32_list_directory("/testlol/");
    
    print("FS initialised\n");
    gui_clear_screen();
    print("PilotOS Kernel Version 0.1 Loaded successfully!\n");
}

void start()
{
}

void run()
{
    terminal_run();
}

void kernel_main() {
    startup_sequence();

    start();

    while (1) {run();}

    while (1) {}
}




