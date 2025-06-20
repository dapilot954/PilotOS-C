#include "../drivers/print.h"
#include "stdint-gcc.h"
#include <stdint.h>
#include "../drivers/bool.h"
#include "../drivers/fat32.h"
#include "../drivers/gui.h"
#include "../drivers/pci.h"
#include "../drivers/ahci.h"

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
    print("scanning PCI Ports\n");
    pci_scan();
    print("attempting to read cluster 0 of SATA drive\n");
    ahci_init(0);
    print("read SATA disk!!!\n");
    // sector 0 is BPB
    uint8_t ret1[512] = {0};
    if (sata_ahci_read(0, 0, 1, ret1) == 0) {
        print("First bytes: ");
        for (int i = 0; i < 8; ++i) {
            print_hex8(ret1[i]);
            print(" ");
        }
        print("\n");
    } else {
        print("AHCI read still failed\n");
    }


    /*
    uint32_t fat_lba = get_partition_start_lba(0);
    dump_sector(0, fat_lba);
    print_first_sector(0);*/
/*
    print("initialising file system\n");
    fat32_init(0);
    print("filesystem initialised\n");
*/
}

/*
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
*/
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




