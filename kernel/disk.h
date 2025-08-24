#ifndef DISK_H
#define DISK_H

#include "types.h"

#define SECTOR_SIZE 512
#define MAX_DISK_SECTORS 65536  // 32 Mo pour test VM

// Codes retour
#define DISK_OK       0
#define DISK_ERR      1
#define DISK_NOMEDIA  2

// Abstraction du type de disque
typedef enum {
    DISK_TYPE_VM,
    DISK_TYPE_SATA
} disk_type_t;

// Initialiser le driver disque selon le type
int disk_init(disk_type_t type);

// Lire `count` secteurs depuis le disque dans buffer
int disk_read(uint32_t lba, void* buffer, uint32_t count);

// Écrire `count` secteurs sur le disque depuis buffer
int disk_write
