#include "port_io.h"
#include "stdint.h"

#define ATA_PRIMARY_IO  0x1F0
#define ATA_PRIMARY_CTRL 0x3F6

#define ATA_REG_DATA       0x00
#define ATA_REG_ERROR      0x01
#define ATA_REG_SECCOUNT0  0x02
#define ATA_REG_LBA0       0x03
#define ATA_REG_LBA1       0x04
#define ATA_REG_LBA2       0x05
#define ATA_REG_HDDEVSEL   0x06
#define ATA_REG_COMMAND    0x07
#define ATA_REG_STATUS     0x07

#define ATA_CMD_READ_PIO   0x20
#define ATA_SR_BSY         0x80
#define ATA_SR_DRQ         0x08

void ata_wait() {
    int timeout = 100000;
    while ((inb(ATA_PRIMARY_IO + ATA_REG_STATUS) & ATA_SR_BSY) && --timeout);
    timeout = 100000;
    while (!(inb(ATA_PRIMARY_IO + ATA_REG_STATUS) & ATA_SR_DRQ) && --timeout);
}


int ata_pio_read(uint32_t lba, uint8_t* buffer) {
    outb(ATA_PRIMARY_IO + ATA_REG_HDDEVSEL, 0xE0 | ((lba >> 24) & 0x0F));
    outb(ATA_PRIMARY_IO + ATA_REG_SECCOUNT0, 1);
    outb(ATA_PRIMARY_IO + ATA_REG_LBA0, (uint8_t)(lba));
    outb(ATA_PRIMARY_IO + ATA_REG_LBA1, (uint8_t)(lba >> 8));
    outb(ATA_PRIMARY_IO + ATA_REG_LBA2, (uint8_t)(lba >> 16));
    outb(ATA_PRIMARY_IO + ATA_REG_COMMAND, ATA_CMD_READ_PIO);

    ata_wait();

    for (int i = 0; i < 256; i++) {
        uint16_t data = inw(ATA_PRIMARY_IO + ATA_REG_DATA);
        buffer[i * 2] = (uint8_t)data;
        buffer[i * 2 + 1] = (uint8_t)(data >> 8);
    }

    return 1; // success
}

#define ATA_CMD_WRITE_PIO 0x30

int ata_pio_write(uint32_t lba, const uint8_t* buffer) {
    // Select drive and high LBA bits
    outb(ATA_PRIMARY_IO + ATA_REG_HDDEVSEL, 0xE0 | ((lba >> 24) & 0x0F));

    // Send sector count and LBA low/mid/high
    outb(ATA_PRIMARY_IO + ATA_REG_SECCOUNT0, 1); // 1 sector
    outb(ATA_PRIMARY_IO + ATA_REG_LBA0, (uint8_t)(lba));
    outb(ATA_PRIMARY_IO + ATA_REG_LBA1, (uint8_t)(lba >> 8));
    outb(ATA_PRIMARY_IO + ATA_REG_LBA2, (uint8_t)(lba >> 16));

    // Send write command
    outb(ATA_PRIMARY_IO + ATA_REG_COMMAND, ATA_CMD_WRITE_PIO);

    // Wait for BSY clear and DRQ set
    ata_wait();

    // Write 256 words (512 bytes)
    for (int i = 0; i < 256; i++) {
        uint16_t data = buffer[i * 2] | (buffer[i * 2 + 1] << 8);
        outw(ATA_PRIMARY_IO + ATA_REG_DATA, data);
    }

    // Flush the cache (send cache flush command if needed)
    outb(ATA_PRIMARY_IO + ATA_REG_COMMAND, 0xE7); // FLUSH CACHE
    ata_wait();

    return 1; // success
}
