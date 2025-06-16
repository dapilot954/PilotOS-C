// ahci.c

#include "ahci.h"
#include "port_io.h"
#include "print.h"
#include "mem.h"

#define HBA_PORT_DEV_PRESENT 0x3
#define HBA_PORT_IPM_ACTIVE  0x1

#define HBA_PxCMD_ST   (1 << 0)
#define HBA_PxCMD_FRE  (1 << 4)
#define HBA_PxCMD_FR   (1 << 14)
#define HBA_PxCMD_CR   (1 << 15)

#define ATA_CMD_READ_DMA_EXT  0x25
#define ATA_CMD_WRITE_DMA_EXT 0x35

#define AHCI_BASE     0x400000
#define SECTOR_SIZE   512
#define MAX_PORTS     32

#define ALIGN_4K(addr) (((uintptr_t)(addr) + 0xFFF) & ~0xFFF)

typedef struct {
    uint8_t fis_type;
    uint8_t pmport:4;
    uint8_t rsv0:3;
    uint8_t c:1;
    uint8_t command;
    uint8_t featurel;
    uint8_t lba0;
    uint8_t lba1;
    uint8_t lba2;
    uint8_t device;
    uint8_t lba3;
    uint8_t lba4;
    uint8_t lba5;
    uint8_t featureh;
    uint8_t countl;
    uint8_t counth;
    uint8_t icc;
    uint8_t control;
    uint8_t rsv1[4];
} FIS_REG_H2D;

typedef struct {
    uint8_t  cfis[64];
    uint8_t  acmd[16];
    uint8_t  rsv[48];
    uint8_t  prdt_entry[8];
} HBA_CMD_TBL;

typedef struct {
    uint16_t flags;
    uint16_t prdbc;
    uint32_t dba;
    uint32_t dbau;
    uint32_t rsv0;
} HBA_PRDT_ENTRY;

typedef struct {
    uint16_t flags;
    uint16_t prdtl;
    uint32_t prdbc;
    uint32_t ctba;
    uint32_t ctbau;
    uint32_t rsv1[4];
} HBA_CMD_HEADER;

typedef volatile struct {
    uint32_t clb;
    uint32_t clbu;
    uint32_t fb;
    uint32_t fbu;
    uint32_t is;
    uint32_t ie;
    uint32_t cmd;
    uint32_t rsv0;
    uint32_t tfd;
    uint32_t sig;
    uint32_t ssts;
    uint32_t sctl;
    uint32_t serr;
    uint32_t sact;
    uint32_t ci;
    uint32_t sntf;
    uint32_t fbs;
    uint32_t rsv1[11];
    uint32_t vendor[4];
} HBA_PORT;

typedef volatile struct {
    uint32_t cap;
    uint32_t ghc;
    uint32_t is;
    uint32_t pi;
    uint32_t vs;
    uint32_t ccc_ctl;
    uint32_t ccc_pts;
    uint32_t em_loc;
    uint32_t em_ctl;
    uint32_t cap2;
    uint32_t bohc;
    uint8_t  rsv[0xA0 - 0x2C];
    uint8_t  vendor[0x100 - 0xA0];
    HBA_PORT ports[32];
} HBA_MEM;

static HBA_MEM* abar = 0;

void ahci_init(uint32_t abar_phys) {
    abar = (HBA_MEM*)(uintptr_t)(abar_phys & ~0xF);

    print("AHCI base address: ");
    print_hex32((uint32_t)(uintptr_t)abar);
    print("\n");

    abar->ghc &= ~0x2;  // disable interrupts
    abar->ghc |= (1 << 31); // enable AHCI mode

    for (int i = 0; i < MAX_PORTS; i++) {
        if (abar->pi & (1 << i)) {
            HBA_PORT* port = &abar->ports[i];
            uint32_t dt = port->ssts & 0x0F;
            uint32_t ipm = (port->ssts >> 8) & 0x0F;
            if (dt == HBA_PORT_DEV_PRESENT && ipm == HBA_PORT_IPM_ACTIVE) {
                print("Port ");
                print_uint(i);
                print(" device present\n");
            }
        }
    }
}

static int issue_ahci_cmd(HBA_PORT* port, uint8_t cmd, uint64_t lba, uint32_t sector_count, uint8_t* buf) {
    port->cmd &= ~HBA_PxCMD_ST;
    while (port->cmd & (HBA_PxCMD_FR | HBA_PxCMD_CR));
    port->cmd &= ~HBA_PxCMD_FRE;

    HBA_CMD_HEADER* cmd_header = (HBA_CMD_HEADER*)(uintptr_t)(port->clb);
    memset(cmd_header, 0, sizeof(HBA_CMD_HEADER) * 32);

    cmd_header[0].prdtl = 1;
    cmd_header[0].flags = (5 << 16);
    cmd_header[0].ctba = (uint32_t)(uintptr_t)(AHCI_BASE + 0x6000);
    cmd_header[0].ctbau = 0;

    HBA_CMD_TBL* cmd_tbl = (HBA_CMD_TBL*)(uintptr_t)(AHCI_BASE + 0x6000);
    memset(cmd_tbl, 0, sizeof(HBA_CMD_TBL));

    HBA_PRDT_ENTRY* prdt = (HBA_PRDT_ENTRY*)(&cmd_tbl->prdt_entry);
    prdt->dba = (uint32_t)(uintptr_t)buf;
    prdt->dbau = 0;
    prdt->rsv0 = 0;
    prdt->prdbc = 0;
    prdt->flags = (sector_count * SECTOR_SIZE) - 1;

    FIS_REG_H2D* fis = (FIS_REG_H2D*)(&cmd_tbl->cfis);
    fis->fis_type = 0x27;
    fis->c = 1;
    fis->command = cmd;

    fis->lba0 = (uint8_t)(lba);
    fis->lba1 = (uint8_t)(lba >> 8);
    fis->lba2 = (uint8_t)(lba >> 16);
    fis->device = 1 << 6;
    fis->lba3 = (uint8_t)(lba >> 24);
    fis->lba4 = (uint8_t)(lba >> 32);
    fis->lba5 = (uint8_t)(lba >> 40);
    fis->countl = sector_count & 0xFF;
    fis->counth = (sector_count >> 8) & 0xFF;

    port->cmd |= HBA_PxCMD_FRE;
    port->cmd |= HBA_PxCMD_ST;

    port->ci = 1;

    for (int i = 0; i < 1000000; i++) {
        if (!(port->ci & 1)) break;
    }

    if (port->ci & 1 || port->tfd & (1 << 7)) {
        print("AHCI command failed or timed out\n");
        return -1;
    }

    return 0;
}

int sata_ahci_read(uint32_t port, uint64_t start_lba, uint32_t sector_count, uint8_t* buf) {
    if (!abar || port >= MAX_PORTS) return -1;
    return issue_ahci_cmd(&abar->ports[port], ATA_CMD_READ_DMA_EXT, start_lba, sector_count, buf);
}

int sata_ahci_write(uint32_t port, uint64_t start_lba, uint32_t sector_count, const uint8_t* buf) {
    if (!abar || port >= MAX_PORTS) return -1;
    return issue_ahci_cmd(&abar->ports[port], ATA_CMD_WRITE_DMA_EXT, start_lba, sector_count, (uint8_t*)buf);
}
