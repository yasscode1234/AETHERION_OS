#ifndef AETHERION_ATA_H
#define AETHERION_ATA_H

#include "disk.h"
#include <stdint.h>

/* ---- Ports ATA/IDE ---- */
#define ATA_PRIMARY_BASE    0x1F0
#define ATA_SECONDARY_BASE  0x170
#define ATA_PRIMARY_CTRL    0x3F6
#define ATA_SECONDARY_CTRL  0x376

/* ---- Registres ATA ---- */
#define ATA_REG_DATA        0x00
#define ATA_REG_ERROR       0x01
#define ATA_REG_FEATURES    0x01
#define ATA_REG_SECCOUNT    0x02
#define ATA_REG_LBA_LOW     0x03
#define ATA_REG_LBA_MID     0x04
#define ATA_REG_LBA_HIGH    0x05
#define ATA_REG_DEVICE      0x06
#define ATA_REG_STATUS      0x07
#define ATA_REG_COMMAND     0x07
#define ATA_REG_ALT_STATUS  0x06  /* Control register */

/* ---- Bits de status ---- */
#define ATA_STATUS_BSY      0x80  /* Busy */
#define ATA_STATUS_DRDY     0x40  /* Drive Ready */
#define ATA_STATUS_DF       0x20  /* Drive Fault */
#define ATA_STATUS_DRQ      0x08  /* Data Request */
#define ATA_STATUS_ERR      0x01  /* Error */

/* ---- Commandes ATA ---- */
#define ATA_CMD_READ_PIO        0x20
#define ATA_CMD_READ_PIO_EXT    0x24
#define ATA_CMD_WRITE_PIO       0x30
#define ATA_CMD_WRITE_PIO_EXT   0x34
#define ATA_CMD_IDENTIFY        0xEC
#define ATA_CMD_FLUSH_CACHE     0xE7
#define ATA_CMD_FLUSH_CACHE_EXT 0xEA

/* ---- Device register ---- */
#define ATA_DEV_MASTER  0xA0
#define ATA_DEV_SLAVE   0xB0
#define ATA_DEV_LBA     0x40

/* ---- Structure priv?e ATA ---- */
typedef struct {
    u16 base_port;      /* 0x1F0 ou 0x170 */
    u16 ctrl_port;      /* 0x3F6 ou 0x376 */
    int is_master;      /* 0 = master, 1 = slave */
    int is_lba48;       /* Support LBA48 */
    u64 max_lba;        /* Dernier secteur accessible */
} ata_device_t;

/* ---- API ATA ---- */
int ata_init(void);
int ata_detect_disks(void);
int ata_read_sectors(disk_t* disk, u64 lba, u32 count, void* buffer);
int ata_write_sectors(disk_t* disk, u64 lba, u32 count, const void* buffer);

#endif /* AETHERION_ATA_H */
