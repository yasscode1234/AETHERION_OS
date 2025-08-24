// system.c
// Mini filesystem persistant : structure en mémoire + persistance via io_*
// On réserve une zone disque pour stocker la structure FileSystem entière.

#include "system.h"
#include "io.h"
#include "vga.h"
#include <string.h>

#define FS_START_LBA  16    // secteur de départ réservé pour le FS
#define FS_SECTORS    8     // nombre de secteurs réservés pour le FS (8 * 512 = 4096 bytes)
                            // augmente si besoin (doit tenir sizeof(FileSystem))

FileSystem fs;

// Internal helper : write raw FS struct to disk
static int _fs_write_raw_to_disk(void) {
    // taille en octets
    uint32_t size = sizeof(FileSystem);
    uint32_t sectors = (size + 511) / 512;
    if(sectors > FS_SECTORS) {
        vga_draw_text(0, 1, "FS_TOO_BIG", 12);
        return -1;
    }
    return io_write_multi(FS_START_LBA, sectors, (const uint8_t*)&fs) == IO_OK ? 0 : -1;
}

// Internal helper : read raw FS struct from disk
static int _fs_read_raw_from_disk(void) {
    uint32_t size = sizeof(FileSystem);
    uint32_t sectors = (size + 511) / 512;
    if(sectors > FS_SECTORS) {
        vga_draw_text(0, 1, "FS_TOO_BIG", 12);
        return -1;
    }
    // Read into temp buffer then memcpy to fs
    uint8_t tmp[FS_SECTORS * 512];
    if(io_read_multi(FS_START_LBA, sectors, tmp) != IO_OK) return -1;
    memcpy(&fs, tmp, sizeof(FileSystem));
    return 0;
}

void fs_init(void) {
    // Initialisation par défaut
    fs.file_count = 0;
    for(uint32_t i=0;i<MAX_FILES;i++) fs.files[i].name[0]=0;

    // Essaie de charger depuis le disque
    if(_fs_read_raw_from_disk() != 0) {
        // pas de FS trouvé => on crée un FS vide et on l'écrit
        fs.file_count = 0;
        _fs_write_raw_to_disk();
        vga_draw_text(0,1,"FS initialized (new)", 10);
    } else {
        vga_draw_text(0,1,"FS loaded from disk", 10);
    }
}

int fs_write_file(const char* name, const void* data, uint32_t size) {
    if(!name || !data) return -1;
    if(size > MAX_FILE_DATA) return -1;

    // Chercher existant
    for(uint32_t i=0;i<fs.file_count;i++) {
        if(strncmp(fs.files[i].name, name, MAX_FILE_NAME) == 0) {
            fs.files[i].size = size;
            memcpy(fs.files[i].data, data, size);
            // persister
            return _fs_write_raw_to_disk();
        }
    }

    // Nouveau fichier
    if(fs.file_count >= MAX_FILES) return -1;
    strncpy(fs.files[fs.file_count].name, name, MAX_FILE_NAME-1);
    fs.files[fs.file_count].name[MAX_FILE_NAME-1] = 0;
    fs.files[fs.file_count].size = size;
    memcpy(fs.files[fs.file_count].data, data, size);
    fs.file_count++;
    return _fs_write_raw_to_disk();
}

int fs_read_file(const char* name, void* buffer, uint32_t max_size) {
    if(!name || !buffer) return -1;
    for(uint32_t i=0;i<fs.file_count;i++) {
        if(strncmp(fs.files[i].name, name, MAX_FILE_NAME) == 0) {
            uint32_t to_copy = fs.files[i].size;
            if(to_copy > max_size) to_copy = max_size;
            memcpy(buffer, fs.files[i].data, to_copy);
            return (int)to_copy;
        }
    }
    return -1; // not found
}

int fs_flush_to_disk(void) {
    return _fs_write_raw_to_disk();
}
