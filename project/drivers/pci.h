#ifndef PCI_H
#define PCI_H

#include <stdint.h>

// Reads 32 bits from the PCI configuration space
uint32_t pci_config_read(uint8_t bus, uint8_t device, uint8_t function, uint8_t offset);

// Basic device info
uint16_t pci_read_vendor_id(uint8_t bus, uint8_t device, uint8_t function);
uint16_t pci_read_device_id(uint8_t bus, uint8_t device, uint8_t function);
uint8_t pci_read_class_code(uint8_t bus, uint8_t device, uint8_t function);
uint8_t pci_read_subclass(uint8_t bus, uint8_t device, uint8_t function);
uint8_t pci_config_read8(uint8_t bus, uint8_t device, uint8_t function, uint8_t offset);


// Scanning routine
void pci_scan(void);

#endif // PCI_H
