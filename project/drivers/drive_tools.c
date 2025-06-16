#include "print.h"
#include "port_io.h"
#include <stdint.h>

#define ATA_PRIMARY_IO     0x1F0
#define ATA_PRIMARY_CTRL   0x3F6
#define ATA_SECONDARY_IO   0x170
#define ATA_SECONDARY_CTRL 0x376

#define ATA_REG_DATA       0x00
#define ATA_REG_STATUS     0x07
#define ATA_REG_COMMAND    0x07
#define ATA_REG_DEVICE     0x06

#define ATA_CMD_IDENTIFY   0xEC

// Wait for BSY=0 and DRQ=1
int ata_wait_ready(uint16_t io) {
    for (int i = 0; i < 100000; i++) {
        uint8_t status = inb(io + ATA_REG_STATUS);
        if (!(status & 0x80) && (status & 0x08)) {
            return 1;
        }
    }
    return 0;
}

int ata_identify_drive(uint16_t io_base, uint16_t ctrl_base, int slave) {
    outb(ctrl_base, 0);  // Disable IRQs
    outb(io_base + ATA_REG_DEVICE, 0xA0 | (slave << 4));  // Select master/slave
    outb(io_base + 2, 0);  // Sector count
    outb(io_base + 3, 0);  // LBA low
    outb(io_base + 4, 0);  // LBA mid
    outb(io_base + 5, 0);  // LBA high
    outb(io_base + ATA_REG_COMMAND, ATA_CMD_IDENTIFY);  // IDENTIFY

    if (!ata_wait_ready(io_base))
        return 0;

    // Read 256 words (512 bytes) of IDENTIFY response
    for (int i = 0; i < 256; i++) {
        inw(io_base + ATA_REG_DATA);
    }

    return 1;
}

void list_ata_drives() {
    const char* names[] = {
        "Primary Master",
        "Primary Slave",
        "Secondary Master",
        "Secondary Slave"
    };

    uint16_t ios[] = {
        ATA_PRIMARY_IO,
        ATA_PRIMARY_IO,
        ATA_SECONDARY_IO,
        ATA_SECONDARY_IO
    };

    uint16_t ctrls[] = {
        ATA_PRIMARY_CTRL,
        ATA_PRIMARY_CTRL,
        ATA_SECONDARY_CTRL,
        ATA_SECONDARY_CTRL
    };

    print("Detecting ATA drives...\n");

    for (int i = 0; i < 4; i++) {
        print(" - ");
        print(names[i]);
        print(": ");

        if (ata_identify_drive(ios[i], ctrls[i], i % 2)) {
            print("Present\n");
        } else {
            print("Not detected\n");
        }
    }
}
