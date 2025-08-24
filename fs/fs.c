#include "fs.h"
#include "disk.h"
#include "string.h"
#include "stdio.h"

#define FS_INDEX_LBA 1
#define FS_INDEX_ENTRIES 64
#define FS_NAME_LEN 32
#define SECTOR_SIZE 512
typedef struct { char name[FS_NAME_LEN]; uint32_t lba; uint32_t size; } fs_entry_t;

// read index from disk into memory
static fs_entry_t index_table[FS_INDEX_ENTRIES];

int fs_init(void){
    uint8_t buf[SECTOR_SIZE];
    if(disk_read( FS_INDEX_LBA, buf, 1) != 0) { 
        // index empty => init
        for(int i=0;i<FS_INDEX_ENTRIES;i++) index_table[i].name[0]=0;
        // write back
        disk_write(FS_INDEX_LBA, index_table, 1);
        return 0;
    }
    memcpy(index_table, buf, sizeof(index_table));
    return 0;
}

static int find_entry(const char* name){
    for(int i=0;i<FS_INDEX_ENTRIES;i++){
        if(index_table[i].name[0] && strcmp(index_table[i].name, name)==0) return i;
    }
    return -1;
}

int fs_read_file(const char* name, void* buf, uint32_t maxlen){
    int idx = find_entry(name);
    if(idx < 0) return -1;
    uint32_t toread = (index_table[idx].size < maxlen) ? index_table[idx].size : maxlen;
    uint32_t sectors = (toread + SECTOR_SIZE -1)/SECTOR_SIZE;
    if(disk_read(index_table[idx].lba, buf, sectors) != 0) return -1;
    return (int)toread;
}

int fs_write_file(const char* name, const void* data, uint32_t len){
    // find free index entry
    int freeidx = -1;
    for(int i=0;i<FS_INDEX_ENTRIES;i++) if(index_table[i].name[0]==0){ freeidx = i; break; }
    if(freeidx<0) return -1;
    // find free LBA (simple: find first empty sector after index)
    uint32_t start = 2; // reserve sector 0 MBR, 1 index
    // naive: find first continuous block large enough
    uint32_t needed = (len + SECTOR_SIZE -1)/SECTOR_SIZE;
    // naive scan: assume free (no fragmentation) -> place at next free offset by checking existing entries
    uint32_t candidate = start;
    for(int i=0;i<FS_INDEX_ENTRIES;i++){
        if(index_table[i].name[0]){
            uint32_t end = index_table[i].lba + (index_table[i].size + SECTOR_SIZE-1)/SECTOR_SIZE;
            if(end > candidate) candidate = end;
        }
    }
    // write data
    if(disk_write(candidate, (uint8_t*)data, (uint32_t)needed) != 0) return -1;
    // fill index
    strncpy(index_table[freeidx].name, name, FS_NAME_LEN-1);
    index_table[freeidx].lba = candidate;
    index_table[freeidx].size = len;
    // commit index back to disk
    disk_write(FS_INDEX_LBA, (uint8_t*)index_table, 1);
    return 0;
}

int fs_list(void){
    for(int i=0;i<FS_INDEX_ENTRIES;i++){
        if(index_table[i].name[0]) printf("%s (%u bytes)\n", index_table[i].name, index_table[i].size);
    }
    return 0;
}

int fs_delete_file(const char* name){
    int idx = find_entry(name);
    if(idx<0) return -1;
    index_table[idx].name[0]=0;
    disk_write(FS_INDEX_LBA, (uint8_t*)index_table, 1);
    return 0;
}

int fs_autocomplete(const char* prefix, char* out, int outlen){
    int plen = strlen(prefix);
    for(int i=0;i<FS_INDEX_ENTRIES;i++){
        if(index_table[i].name[0] && strncmp(index_table[i].name, prefix, plen)==0){
            strncpy(out, index_table[i].name, outlen-1); out[outlen-1]=0; return 0;
        }
    }
    return -1;
}
