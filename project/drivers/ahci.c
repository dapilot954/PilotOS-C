#include "ahci.h"
#include "pci.h"
#include "print.h"

volatile HBA_MEM* abar = (volatile HBA_MEM*)AHCI_BASE;

void ahci_init(uint32_t abar_addr) {
    abar = (volatile HBA_MEM*)abar_addr;
    print("AHCI controller found.\n");

    for (int i = 0; i < 32; i++) {
        if (abar->pi & (1 << i)) {
            HBA_PORT* port = &abar->ports[i];
            uint32_t ssts = port->ssts;
            uint8_t ipm = (ssts >> 8) & 0x0F;
            uint8_t det = ssts & 0x0F;
            if (det == 3 && ipm == 1) {
                print("SATA drive found at port ");
                print_uint(i);
                print("\n");
            }
        }
    }
}
