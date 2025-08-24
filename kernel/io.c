// io.c
// Couche d'abstraction IO. Pour l'instant elle appelle disk.c
// Plus tard on pourra remplacer l'implémentation par AHCI/SATA.

#include "io.h"
#include "disk.h"

// Initialise la couche IO -> délègue à disk_init (disk.c doit exister)
int io_init(void) {
    return disk_init() == 0 ? IO_OK : IO_ERR;
}

int io_read_sector(uint32_t lba, uint8_t* buf) {
    if(!buf) return IO_ERR;
    return disk_read_sector(lba, buf) == 0 ? IO_OK : IO_ERR;
}

int io_write_sector(uint32_t lba, const uint8_t* buf) {
    if(!buf) return IO_ERR;
    return disk_write_sector(lba, (uint8_t*)buf) == 0 ? IO_OK : IO_ERR;
}

int io_read_multi(uint32_t lba, uint32_t count, uint8_t* buf) {
    if(!buf || count == 0) return IO_ERR;
    return disk_read_multi(lba, count, buf) == 0 ? IO_OK : IO_ERR;
}

int io_write_multi(uint32_t lba, uint32_t count, const uint8_t* buf) {
    if(!buf || count == 0) return IO_ERR;
    return disk_write_multi(lba, count, (uint8_t*)buf) == 0 ? IO_OK : IO_ERR;
}

int io_read_file(uint32_t start_lba, uint8_t* out_buf, uint32_t size_bytes) {
    if(!out_buf) return IO_ERR;
    if(size_bytes == 0) return IO_OK;
    uint32_t sectors = (size_bytes + 511) / 512;
    return io_read_multi(start_lba, sectors, out_buf);
}

int io_write_file(uint32_t start_lba, const uint8_t* data, uint32_t size_bytes) {
    if(!data) return IO_ERR;
    if(size_bytes == 0) return IO_OK;
    uint32_t sectors = (size_bytes + 511) / 512;
    return io_write_multi(start_lba, sectors, data);
}

int io_is_empty(uint32_t lba) {
    return disk_is_empty(lba);
}
