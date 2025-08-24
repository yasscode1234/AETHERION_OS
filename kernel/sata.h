#ifndef SATA_H
#define SATA_H

#include <stdint.h>
#include <stdbool.h>

#define SECTOR_SIZE 512

typedef struct {
    uint8_t  port_base;
    uint16_t device;
    bool     initialized;
} sata_device_t;

bool sata_init();
bool sata_read_sector(uint32_t lba, uint8_t* buffer, uint32_t count);
bool sata_write_sector(uint32_t lba, const uint8_t* buffer, uint32_t count);

#endif
