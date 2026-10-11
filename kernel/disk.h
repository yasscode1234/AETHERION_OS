#ifndef AETHERION_DISK_H
#define AETHERION_DISK_H

#include <stdint.h>
#include <stddef.h>

/* ---- Types de base ---- */
typedef uint8_t  u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;

/* ---- Types de disques ---- */
typedef enum {
    DISK_TYPE_UNKNOWN = 0,
    DISK_TYPE_ATA_PIO,
    DISK_TYPE_ATA_DMA,
    DISK_TYPE_SATA_AHCI,
    DISK_TYPE_NVME,
    DISK_TYPE_USB,
    DISK_TYPE_VIRTIO
} disk_type_t;

/* ---- Structure disque ---- */
typedef struct disk {
    char name[32];
    disk_type_t type;
    u64 sectors;           /* Nombre total de secteurs */
    u32 sector_size;       /* Taille secteur (g?n?ralement 512) */
    u64 size_bytes;        /* Taille totale en octets */
    int (*read_sectors)(struct disk* disk, u64 lba, u32 count, void* buffer);
    int (*write_sectors)(struct disk* disk, u64 lba, u32 count, const void* buffer);
    void* private_data;    /* Donn?es sp?cifiques au driver */
} disk_t;

/* ---- Registre global des disques ---- */
#define MAX_DISKS 8
extern disk_t* g_disks[MAX_DISKS];
extern int g_disk_count;

/* ---- API Disque ---- */
void disk_init(void);
int disk_register(disk_t* disk);
disk_t* disk_get(int index);
disk_t* disk_find_by_name(const char* name);
int disk_read(disk_t* disk, u64 lba, u32 count, void* buffer);
int disk_write(disk_t* disk, u64 lba, u32 count, const void* buffer);
void disk_list(void);
u64 disk_get_size(disk_t* disk);

#endif /* AETHERION_DISK_H */
