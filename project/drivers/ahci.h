// ahci.h

#ifndef AHCI_H
#define AHCI_H

#include <stdint.h>

void ahci_init(uint32_t abar);
int sata_ahci_read(uint32_t port, uint64_t lba, uint32_t sector_count, uint8_t* buffer);
int sata_ahci_write(uint32_t port, uint64_t lba, uint32_t sector_count, const uint8_t* buffer);

#endif
