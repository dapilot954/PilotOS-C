#include <stdint.h>
#include "port_io.h"
#include "print.h"
#include "ahci.h"

#define PCI_CONFIG_ADDRESS 0xCF8
#define PCI_CONFIG_DATA    0xCFC

uint32_t pci_config_read(uint8_t bus, uint8_t device, uint8_t function, uint8_t offset) {
    uint32_t address = (uint32_t)(
        (1 << 31) | (bus << 16) | (device << 11) | (function << 8) | (offset & 0xFC));
    outl(PCI_CONFIG_ADDRESS, address);
    return inl(PCI_CONFIG_DATA);
}

uint32_t pci_config_read32(uint8_t bus, uint8_t device, uint8_t function, uint8_t offset) {
    uint32_t address = (uint32_t)(
        (1 << 31)              |  // Enable bit
        ((uint32_t)bus << 16)  |
        ((uint32_t)device << 11) |
        ((uint32_t)function << 8) |
        (offset & 0xFC)        // aligned to 4 bytes
    );
    outl(0xCF8, address);
    return inl(0xCFC);
}

uint16_t pci_read_vendor_id(uint8_t bus, uint8_t device, uint8_t function) {
    return pci_config_read(bus, device, function, 0x00) & 0xFFFF;
}

uint16_t pci_read_device_id(uint8_t bus, uint8_t device, uint8_t function) {
    return (pci_config_read(bus, device, function, 0x00) >> 16) & 0xFFFF;
}

uint8_t pci_read_class_code(uint8_t bus, uint8_t device, uint8_t function) {
    return (pci_config_read(bus, device, function, 0x08) >> 24) & 0xFF;
}

uint8_t pci_read_subclass(uint8_t bus, uint8_t device, uint8_t function) {
    return (pci_config_read(bus, device, function, 0x08) >> 16) & 0xFF;
}
uint8_t pci_config_read8(uint8_t bus, uint8_t device, uint8_t function, uint8_t offset) {
    uint32_t address = (1 << 31)
                     | ((uint32_t)bus << 16)
                     | ((uint32_t)device << 11)
                     | ((uint32_t)function << 8)
                     | (offset & 0xFC);

    outl(0xCF8, address);
    uint32_t data = inl(0xCFC);

    return (data >> ((offset & 3) * 8)) & 0xFF;
}
void pci_scan() {
    for (int bus = 0; bus < 256; bus++) {
        for (int device = 0; device < 32; device++) {
            for (int function = 0; function < 8; function++) {
                uint16_t vendor = pci_read_vendor_id(bus, device, function);
                if (vendor == 0xFFFF) continue;

                uint16_t device_id = pci_read_device_id(bus, device, function);
                uint8_t class_code = pci_read_class_code(bus, device, function);
                uint8_t subclass = pci_read_subclass(bus, device, function);
                uint8_t prog_if = pci_config_read8(bus, device, function, 0x09);

                print("PCI Device found: ");
                print("Bus ");
                print_uint(bus);
                print(", Device ");
                print_uint(device);
                print(", Function ");
                print_uint(function);
                print(", Vendor: 0x");
                print_hex16(vendor);
                print(", Device ID: 0x");
                print_hex16(device_id);
                print(", Class: ");
                print_uint(class_code);
                print(", Subclass: ");
                print_uint(subclass);
                print(", ProgIF: ");
                print_uint(prog_if);
                print("\n");

                // Check for AHCI controller
                if (class_code == 0x01 && subclass == 0x06 && prog_if == 0x01) {
                    uint32_t abar = pci_config_read32(bus, device, function, 0x24);  // BAR5
                    print("AHCI controller detected at BAR5: 0x");
                    print_hex32(abar);
                    print("\n");

                    ahci_init(abar);
                }
            }
        }
    }
}
