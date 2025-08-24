#ifndef FS_H
#define FS_H

#include "types.h"

typedef enum {
    FS_OK = 0,
    FS_ERR_IO = -1,
    FS_ERR_NOFS = -2,
    FS_ERR_INVAL = -3,
    FS_ERR_NOMEM = -4,
    FS_ERR_NOTFOUND = -5,
} fs_status_t;

typedef struct fs_file fs_file_t;

/* Init auto (détecte quel backend est dispo : “raw-partition”, FAT, etc.) */
fs_status_t fs_init(void);

/* API simple type “VFS” minimal */
fs_status_t fs_load_file(const char* path, void* buf, size_t bufsize, size_t* out_size);
fs_status_t fs_save_file(const char* path, const void* data, size_t size);

/* Handles (si plus tard tu fais un vrai VFS) */
fs_file_t* fs_open(const char* path, const char* mode);
size_t fs_read(void* ptr, size_t size, size_t nmemb, fs_file_t* f);
size_t fs_write(const void* ptr, size_t size, size_t nmemb, fs_file_t* f);
int fs_close(fs_file_t* f);

#endif
