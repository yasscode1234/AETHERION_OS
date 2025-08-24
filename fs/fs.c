#include "fs.h"
#include "disk.h"
#include "string.h"

/* Pour l’instant : stub mémoire / raw.
   Tu brancheras ici FAT32/EXT2 plus tard. */

fs_status_t fs_init(void){
    /* TODO: inspecter MBR/GPT, détecter FS, initialiser le backend */
    return FS_OK;
}

fs_status_t fs_load_file(const char* path, void* buf, size_t bufsize, size_t* out_size){
    (void)path; (void)buf; (void)bufsize;
    if(out_size) *out_size = 0;
    return FS_ERR_NOTFOUND; /* stub */
}

fs_status_t fs_save_file(const char* path, const void* data, size_t size){
    (void)path; (void)data; (void)size;
    return FS_ERR_IO; /* stub */
}

/* Handles : stubs */
struct fs_file { int dummy; };
fs_file_t* fs_open(const char* p, const char* m){ (void)p;(void)m; return 0; }
size_t fs_read(void* p,size_t s,size_t n,fs_file_t* f){ (void)p;(void)s;(void)n;(void)f; return 0; }
size_t fs_write(const void* p,size_t s,size_t n,fs_file_t* f){ (void)p;(void)s;(void)n;(void)f; return 0; }
int fs_close(fs_file_t* f){ (void)f; return 0; }
