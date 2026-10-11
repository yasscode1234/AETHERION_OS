#ifndef AETHERION_AHCI_H
#define AETHERION_AHCI_H

#include "disk.h"
#include <stdint.h>

/* ---- AHCI Register Offsets (HBA Memory) ---- */
#define AHCI_HBA_CAP        0x00
#define AHCI_HBA_GHC        0x04
#define AHCI_HBA_IS         0x08
#define AHCI_HBA_PI         0x0C
#define AHCI_HBA_VS         0x10
#define AHCI_HBA_CCC_CTL    0x14
#define AHCI_HBA_CCC_PORTS  0x18
#define AHCI_HBA_EM_LOC     0x1C
#define AHCI_HBA_EM_CTL     0x20
#define AHCI_HBA_CAP2       0x24
#define AHCI_HBA_BOHC       0x28

/* ---- Port Register Offsets (per port, base = 0x100 + port*0x80) ---- */
#define AHCI_PORT_CLB       0x00  /* Command List Base */
#define AHCI_PORT_CLBU      0x04  /* Command List Base Upper */
#define AHCI_PORT_FB        0x08  /* FIS Base */
#define AHCI_PORT_FBU       0x0C  /* FIS Base Upper */
#define AHCI_PORT_IS        0x10  /* Interrupt Status */
#define AHCI_PORT_IE        0x14  /* Interrupt Enable */
#define AHCI_PORT_CMD       0x18  /* Command */
#define AHCI_PORT_TFD       0x20  /* Task File Data */
#define AHCI_PORT_SIG       0x24  /* Signature */
#define AHCI_PORT_SSTS      0x28  /* SATA Status */
#define AHCI_PORT_SCTL      0x2C  /* SATA Control */
#define AHCI_PORT_SERR      0x30  /* SATA Error */
#define AHCI_PORT_SACT      0x34  /* SATA Active */
#define AHCI_PORT_CI        0x38  /* Command Issue */
#define AHCI_PORT_SNTF      0x3C  /* SNotification */
#define AHCI_PORT_FBS       0x40  /* FIS-based Switching */

/* ---- HBA Capabilities ---- */
#define AHCI_CAP_S64A       (1 << 31)  /* 64-bit addressing */
#define AHCI_CAP_SNCQ       (1 << 30)  /* Native Command Queuing */
#define AHCI_CAP_SSS        (1 << 27)  /* Staggered Spin-up */
#define AHCI_CAP_SMPS       (1 << 26)  /* Mechanical Presence Switch */
#define AHCI_CAP_SSC        (1 << 25)  /* Slumber Capable */
#define AHCI_CAP_PSC        (1 << 24)  /* Partial State Capable */
#define AHCI_CAP_NCS_MASK   0x1F       /* Number of Command Slots */
#define AHCI_CAP_NP_MASK    0xF00      /* Number of Ports */

/* ---- Global HBA Control ---- */
#define AHCI_GHC_AE         (1 << 31)  /* AHCI Enable */
#define AHCI_GHC_IE         (1 << 1)   /* Interrupt Enable */
#define AHCI_GHC_HR         (1 << 0)   /* HBA Reset */

/* ---- Port Command ---- */
#define AHCI_CMD_ST         (1 << 0)   /* Start */
#define AHCI_CMD_SUD        (1 << 1)   /* Spin-Up Device */
#define AHCI_CMD_POD        (1 << 2)   /* Power On Device */
#define AHCI_CMD_CLO        (1 << 3)   /* Command List Override */
#define AHCI_CMD_FRE        (1 << 4)   /* FIS Receive Enable */
#define AHCI_CMD_FR         (1 << 14)  /* FIS Receive Running */
#define AHCI_CMD_CR         (1 << 15)  /* Command List Running */

/* ---- Port Signature ---- */
#define AHCI_SIG_ATA        0x00000101
#define AHCI_SIG_ATAPI      0xEB140101
#define AHCI_SIG_SEMB       0xC33C0101
#define AHCI_SIG_PM         0x96690101

/* ---- SATA Status (SSTS) ---- */
#define AHCI_SSTS_DET_MASK  0xF
#define AHCI_SSTS_DET_NONE  0x0
#define AHCI_SSTS_DET_PHY   0x1
#define AHCI_SSTS_DET_DEV   0x3
#define AHCI_SSTS_DET_OFF   0x4

/* ---- FIS Types ---- */
#define AHCI_FIS_TYPE_REG_H2D   0x27  /* Register H2D */
#define AHCI_FIS_TYPE_REG_D2H   0x34  /* Register D2H */
#define AHCI_FIS_TYPE_DMA_ACT   0x39  /* DMA Activate */
#define AHCI_FIS_TYPE_DMA_SETUP 0x41  /* DMA Setup */
#define AHCI_FIS_TYPE_DATA      0x46  /* Data */
#define AHCI_FIS_TYPE_BIST      0x58  /* BIST */
#define AHCI_FIS_TYPE_PIO_SETUP 0x5F  /* PIO Setup */
#define AHCI_FIS_TYPE_DEV_BITS  0xA1  /* Device Bits */

/* ---- Command Header ---- */
typedef struct {
    u8  cfl:5;      /* Command FIS Length */
    u8  a:1;        /* ATAPI */
    u8  w:1;        /* Write (1=H2D, 0=D2H) */
    u8  p:1;        /* Prefetchable */
    u8  r:1;        /* Reset */
    u8  b:1;        /* BIST */
    u8  c:1;        /* Clear Busy */
    u8  rsv0:1;
    u8  pmp:4;      /* Port Multiplier Port */
    u16 prdtl;      /* PRDT Length */
    u32 prdbc;      /* PRD Byte Count */
    u32 ctba;       /* Command Table Base Address */
    u32 ctbau;      /* Command Table Base Address Upper */
    u32 rsv1[4];
} __attribute__((packed)) ahci_cmd_header_t;

/* ---- PRD Table Entry ---- */
typedef struct {
    u32 dba;        /* Data Base Address */
    u32 dbau;       /* Data Base Address Upper */
    u32 rsv0;
    u32 dbc:22;     /* Data Byte Count */
    u32 rsv1:9;
    u32 i:1;        /* Interrupt on Completion */
} __attribute__((packed)) ahci_prdt_entry_t;

/* ---- Command Table ---- */
typedef struct {
    u8  cfis[64];           /* Command FIS */
    u8  acmd[16];           /* ATAPI Command */
    u8  rsv[48];
    ahci_prdt_entry_t prdt[];  /* PRDT Entries */
} __attribute__((packed)) ahci_cmd_table_t;

/* ---- Received FIS Structure ---- */
typedef struct {
    u8  dsfis[256];   /* DMA Setup FIS */
    u8  psfis[256];   /* PIO Setup FIS */
    u8  rsfis[256];   /* Register D2H FIS */
    u8  sdbfis[256];  /* Set Device Bits FIS */
    u8  ufis[64];     /* Unknown FIS */
    u8  rsv[96];
} __attribute__((packed)) ahci_received_fis_t;

/* ---- Port Private Data ---- */
typedef struct {
    u32 port_num;
    u32 cmd_slot;
    u64 cmd_list_base;
    u64 fis_base;
    u64 cmd_table_base;
    ahci_cmd_header_t* cmd_headers;
    ahci_received_fis_t* rfis;
    int sata_spd;  /* SATA speed */
} ahci_port_t;

/* ---- HBA Private Data ---- */
typedef struct {
    u64 abar;           /* ABAR (HBA Memory Base) */
    u32 ports_impl;     /* Ports Implemented bitmap */
    u32 num_ports;      /* Number of ports */
    int cap_64bit;      /* 64-bit addressing support */
    int cap_ncq;        /* NCQ support */
    ahci_port_t ports[32];
} ahci_hba_t;

/* ---- API AHCI ---- */
int ahci_init(void);
int ahci_detect_ports(void);
int ahci_port_start(ahci_port_t* port);
int ahci_port_stop(ahci_port_t* port);
int ahci_read_sectors(disk_t* disk, u64 lba, u32 count, void* buffer);
int ahci_write_sectors(disk_t* disk, u64 lba, u32 count, const void* buffer);

#endif /* AETHERION_AHCI_H */
