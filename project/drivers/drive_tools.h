#ifndef ATA_DRIVE_DETECT_H
#define ATA_DRIVE_DETECT_H

#include <stdint.h>

// List all ATA drives (Primary/Secondary Master/Slave)
void list_ata_drives(void);

// Internal use: sends ATA IDENTIFY command and checks response
int ata_identify_drive(uint16_t io_base, uint16_t ctrl_base, int slave);

// Internal use: waits until the drive is ready (BSY=0 and DRQ=1)
int ata_wait_ready(uint16_t io_base);

#endif // ATA_DRIVE_DETECT_H
