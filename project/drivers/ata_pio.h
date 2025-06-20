#ifndef ATA_PIO_H
#define ATA_PIO_H

#include "stdint.h"

int ata_pio_read(uint32_t lba, uint8_t* buffer);
int ata_pio_write(uint32_t lba, const uint8_t* buffer);

#endif
