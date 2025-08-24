#ifndef AHCI_H
#define AHCI_H

#include "types.h"

// AHCI class code in PCI: class=0x01 (mass storage), subclass=0x06 (SATA), prog_if=0x01 (AHCI)
#define PCI_CLASS_MASS_STORAGE 0x01
#define PCI_SUBCLASS_SATA      0x06
#define PCI_PROGIF_AHCI        0x01

// HBA memory map (partial) - layout from AHCI spec
typedef volatile struct {
    uint32_t cap;       // 0x00 host capabilities
    uint32_t ghc;       // 0x04 global host control
    uint32_t is;        // 0x08 interrupt status
    uint32_t pi;        // 0x0C ports implemented
    uint32_t vs;        // 0x10 version
    uint32_t ccc_ctl;   // 0x14 command completion coalescing
    uint32_t ccc_pts;   // 0x18
    uint32_t em_loc;    // 0x1C
    uint32_t em_ctl;    // 0x20
    uint32_t cap2;      // 0x24
    uint32_t bohc;      // 0x28
    uint8_t  reserved[0xA0 - 0x2C];
    uint8_t  vendor[0x100 - 0xA0];
    // ports follow
} HBA_MEM;

typedef volatile struct {
    uint32_t clb;       // 0x00 command list base address
    uint32_t clb_h;     // 0x04
    uint32_t fb;        // 0x08 FIS base address
    uint32_t fb_h;      // 0x0C
    uint32_t is;        // 0x10 interrupt status
    uint32_t ie;        // 0x14 interrupt enable
    uint32_t cmd;       // 0x18 command and status
    uint32_t rsv0;      // 0x1C
    uint32_t tfd;       // 0x20 task file data
    uint32_t sig;       // 0x24 signature
    uint32_t ssts;      // 0x28 sata status (SCR0:SStatus)
    uint32_t sctl;      // 0x2C sata control (SCR2:SControl)
    uint32_t serr;      // 0x30 sata error (SCR1:SError)
    uint32_t sact;      // 0x34 sata active (SCR3:SActive)
    uint32_t ci;        // 0x38 command issue
    uint32_t sntf;      // 0x3C sata notification (SCR4:SNotification)
    uint32_t fbs;       // 0x40 FIS-based switching
    uint32_t rsv1[11];  // 0x44 ~ 0x6F
    uint32_t vendor[4]; // 0x70 ~ 0x7F
} HBA_PORT;

#define SATA_SIG_ATA    0x00000101
#define SATA_SIG_ATAPI  0xEB140101

// Command list / table structures
typedef struct {
    uint16_t flags;
    uint16_t prdtl; // prdt length
    uint32_t prdbc;
    uint32_t ctba;
    uint32_t ctba_high;
    uint32_t reserved[4];
} HBA_CMD_HEADER;

typedef struct {
    uint32_t dba;
    uint32_t dba_high;
    uint32_t reserved;
    uint32_t dbc; // lower 22 bits valid, bit31 interrupt on completion
} HBA_PRDT_ENTRY;

typedef struct {
    uint8_t  cfis[64];
    uint8_t  acmd[16];
    uint8_t  reserved[48];
    HBA_PRDT_ENTRY prdt_entry[1]; // variable
} HBA_CMD_TBL;

// Public AHCI API
int ahci_init(void); // probe PCI, map ABAR, init ports
int ahci_read(uint32_t portnum, uint64_t start_lba, uint32_t sector_count, void* buffer);
int ahci_write(uint32_t portnum, uint64_t start_lba, uint32_t sector_count, const void* buffer);

#endif
