/*
 * Paramètres système
 * Copyright (C) 2025 yasscode
 * Licence GPLv2
 */

#ifndef SYSTEM_H
#define SYSTEM_H

#include "types.h"

#define MAX_FILE_NAME 32
#define MAX_FILE_DATA 1024
#define MAX_FILES 64

typedef struct {
    char name[MAX_FILE_NAME];
    uint32_t size;
    uint8_t data[MAX_FILE_DATA];
} FileEntry;

typedef struct {
    FileEntry files[MAX_FILES];
    uint32_t file_count;
} FileSystem;

extern FileSystem fs;

// Initialise le filesystem (charge depuis disque)
void fs_init(void);

// Ecrit un fichier dans le FS (en mémoire puis persistance sur disque)
int fs_write_file(const char* name, const void* data, uint32_t size);

// Lit un fichier depuis le FS (en mémoire)
int fs_read_file(const char* name, void* buffer, uint32_t max_size);

// Sauvegarde entière du FS sur disque (force)
int fs_flush_to_disk(void);

#endif
