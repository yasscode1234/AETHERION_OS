#ifndef IO_H
#define IO_H

#include "types.h"

#define IO_OK    0
#define IO_ERR  -1

// Initialise la couche IO (redirige vers disk/AHCI...)
int io_init(void);

// Lecture/écriture d'un seul secteur (512 bytes)
int io_read_sector(uint32_t lba, uint8_t* buf);
int io_write_sector(uint32_t lba, const uint8_t* buf);

// Lecture/écriture multi-secteurs
int io_read_multi(uint32_t lba, uint32_t count, uint8_t* buf);
int io_write_multi(uint32_t lba, uint32_t count, const uint8_t* buf);

// Lecture/écriture de "fichiers" (wrappers pratiques)
int io_read_file(uint32_t start_lba, uint8_t* out_buf, uint32_t size_bytes);
int io_write_file(uint32_t start_lba, const uint8_t* data, uint32_t size_bytes);

// Vérifie si un secteur est vide (backend peut définir la sémantique)
int io_is_empty(uint32_t lba);

#endif
