#include "sata.h"
#include "io.h"      // pour inb/outb
#include "disk.h"
#include <string.h>

static sata_device_t sata_dev;

bool sata_init() {
    // Pour l'instant: simulation en VM/émulateur
    sata_dev.port_base = 0x1F0;   // ATA primaire
    sata_dev.device = 0;
    sata_dev.initialized = true;
    return true;
}

bool sata_read_sector(uint32_t lba, uint8_t* buffer, uint32_t count) {
    if(!sata_dev.initialized) return false;

    // Simulation: lecture depuis le disque émulé
    for(uint32_t i=0; i<count; i++) {
        disk_read_sector(lba + i, buffer + i * SECTOR_SIZE);
    }
    return true;
}

bool sata_write_sector(uint32_t lba, const uint8_t* buffer, uint32_t count) {
    if(!sata_dev.initialized) return false;

    // Simulation: écriture vers le disque émulé
    for(uint32_t i=0; i<count; i++) {
        disk_write_sector(lba + i, buffer + i * SECTOR_SIZE);
    }
    return true;
}
