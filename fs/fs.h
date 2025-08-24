#ifndef FS_H
#define FS_H
#include "types.h"
int fs_init(void);
int fs_read_file(const char* name, void* buf, uint32_t maxlen);
int fs_write_file(const char* name, const void* data, uint32_t len);
int fs_delete_file(const char* name);
int fs_list(void); // affiche via printf la liste
int fs_autocomplete(const char* prefix, char* out, int outlen);
#endif
