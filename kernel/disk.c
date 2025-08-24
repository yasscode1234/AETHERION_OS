#include "disk.h"
#include "types.h"
#include "io.h"
#include "vga.h"

#define SECTOR_SIZE 512
#define MAX_SECTORS 1024*1024  // Support jusqu'à ~512 MB pour test VM

typedef struct {
    uint8_t lba_lo[4];   // LBA lower
    uint8_t lba_hi[4];   // LBA higher (64-bit)
    uint16_t status;     // Status
} ahci_port_t;

// Structure disque virtuel pour test
static uint8_t disk_buffer[MAX_SECTORS * SECTOR_SIZE];

int disk_initialized = 0;

// Initialisation du disque (SATA/AHCI simulé pour VM)
int disk_init() {
    // Ici tu peux ajouter la détection réelle AHCI + init pilote
    for(uint32_t i = 0; i < MAX_SECTORS*SECTOR_SIZE; i++)
        disk_buffer[i] = 0xFF; // Reset disque virtuel
    disk_initialized = 1;
    return 0;
}

// Lecture d’un secteur
int disk_read_sector(uint32_t lba, uint8_t* buf) {
    if(!disk_initialized) return -1;
    if(lba >= MAX_SECTORS) return -2;
    for(int i=0; i<SECTOR_SIZE; i++)
        buf[i] = disk_buffer[lba*SECTOR_SIZE + i];
    return 0;
}

// Écriture d’un secteur
int disk_write_sector(uint32_t lba, uint8_t* buf) {
    if(!disk_initialized) return -1;
    if(lba >= MAX_SECTORS) return -2;
    for(int i=0; i<SECTOR_SIZE; i++)
        disk_buffer[lba*SECTOR_SIZE + i] = buf[i];
    return 0;
}

// Lecture multiple secteurs
int disk_read_multi(uint32_t lba, uint32_t count, uint8_t* buf) {
    if(!disk_initialized) return -1;
    if(lba + count > MAX_SECTORS) return -2;
    for(uint32_t s=0; s<count; s++)
        disk_read_sector(lba+s, buf + s*SECTOR_SIZE);
    return 0;
}

// Écriture multiple secteurs
int disk_write_multi(uint32_t lba, uint32_t count, uint8_t* buf) {
    if(!disk_initialized) return -1;
    if(lba + count > MAX_SECTORS) return -2;
    for(uint32_t s=0; s<count; s++)
        disk_write_sector(lba+s, buf + s*SECTOR_SIZE);
    return 0;
}

// Vérification si secteur vide
int disk_is_empty(uint32_t lba) {
    if(!disk_initialized) return -1;
    for(int i=0; i<SECTOR_SIZE; i++)
        if(disk_buffer[lba*SECTOR_SIZE + i] != 0xFF)
            return 0; // Secteur occupé
    return 1; // Secteur vide
}

// Fonction de test pour écrire kernel / bootloader sur disque
int disk_write_file(uint32_t start_lba, uint8_t* data, uint32_t size_bytes) {
    uint32_t sectors_needed = (size_bytes + SECTOR_SIZE - 1)/SECTOR_SIZE;
    return disk_write_multi(start_lba, sectors_needed, data);
}

// Fonction de test pour lire kernel / bootloader depuis disque
int disk_read_file(uint32_t start_lba, uint8_t* out_buf, uint32_t size_bytes) {
    uint32_t sectors_needed = (size_bytes + SECTOR_SIZE - 1)/SECTOR_SIZE;
    return disk_read_multi(start_lba, sectors_needed, out_buf);
}
