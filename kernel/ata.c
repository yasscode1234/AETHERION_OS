#include "ata.h"
#include "disk.h"
#include "vga.h"
#include "string.h"
#include "stdlib.h"

/* ---- I/O ports ---- */
static inline u8 inb(u16 port) {
    u8 ret;
    __asm__ volatile ("inb %w1, %0" : "=a"(ret) : "Nd"(port));
    return ret;
}

static inline void outb(u16 port, u8 val) {
    __asm__ volatile ("outb %0, %w1" : : "a"(val), "Nd"(port));
}

static inline u16 inw(u16 port) {
    u16 ret;
    __asm__ volatile ("inw %w1, %0" : "=a"(ret) : "Nd"(port));
    return ret;
}

static inline void outw(u16 port, u16 val) {
    __asm__ volatile ("outw %0, %w1" : : "a"(val), "Nd"(port));
}

static inline void insw(u16 port, void* buffer, u32 count) {
    __asm__ volatile ("rep insw" : "=D"(buffer), "=c"(count) : "0"(buffer), "1"(count), "d"(port) : "memory");
}

static inline void outsw(u16 port, const void* buffer, u32 count) {
    __asm__ volatile ("rep outsw" : "=S"(buffer), "=c"(count) : "0"(buffer), "1"(count), "d"(port) : "memory");
}

static inline void io_wait(void) {
    __asm__ volatile ("jmp 1f\n1: jmp 2f\n2:" : : : "memory");
}

/* ---- Attendre que le disque soit pr?t ---- */
static int ata_wait_ready(u16 base_port, u32 timeout_ms) {
    for (u32 i = 0; i < timeout_ms * 1000; i++) {
        u8 status = inb(base_port + ATA_REG_STATUS);
        if (!(status & ATA_STATUS_BSY) && (status & ATA_STATUS_DRDY))
            return 0;
        io_wait();
    }
    return -1;  /* Timeout */
}

/* ---- Attendre DRQ (Data Request) ---- */
static int ata_wait_drq(u16 base_port, u32 timeout_ms) {
    for (u32 i = 0; i < timeout_ms * 1000; i++) {
        u8 status = inb(base_port + ATA_REG_STATUS);
        if (status & ATA_STATUS_ERR)
            return -1;
        if (status & ATA_STATUS_DRQ)
            return 0;
        io_wait();
    }
    return -1;
}

/* ---- S?lectionner le disque (master/slave) ---- */
static void ata_select_drive(u16 base_port, int is_master, u64 lba, int lba48) {
    u8 dev = is_master ? ATA_DEV_MASTER : ATA_DEV_SLAVE;
    dev |= ATA_DEV_LBA;

    if (lba48) {
        /* LBA48: envoyer les 24 bits hauts en premier */
        outb(base_port + ATA_REG_FEATURES, 0);
        outb(base_port + ATA_REG_SECCOUNT, 0);
        outb(base_port + ATA_REG_LBA_HIGH, (lba >> 24) & 0xFF);
        outb(base_port + ATA_REG_LBA_MID, (lba >> 32) & 0xFF);
        outb(base_port + ATA_REG_LBA_LOW, (lba >> 40) & 0xFF);
    }

    outb(base_port + ATA_REG_DEVICE, dev | ((lba >> 24) & 0x0F));
    io_wait();
}

/* ---- Lire les donn?es IDENTIFY ---- */
static int ata_identify(u16 base_port, u16 ctrl_port, int is_master, u16* identify_buf) {
    (void)ctrl_port;
    ata_select_drive(base_port, is_master, 0, 0);
    outb(base_port + ATA_REG_COMMAND, ATA_CMD_IDENTIFY);
    io_wait();

    u8 status = inb(base_port + ATA_REG_STATUS);
    if (status == 0) return -1;  /* Pas de disque */

    if (ata_wait_ready(base_port, 1000) < 0) return -1;

    status = inb(base_port + ATA_REG_STATUS);
    if (status & ATA_STATUS_ERR) return -1;

    /* Lire 256 mots (512 octets) */
    for (int i = 0; i < 256; i++) {
        identify_buf[i] = inw(base_port + ATA_REG_DATA);
    }
    return 0;
}

/* ---- Lire secteurs PIO ---- */
static int ata_pio_read(u16 base_port, u64 lba, u32 count, void* buffer, int lba48) {
    u16* buf = (u16*)buffer;

    for (u32 sector = 0; sector < count; sector++) {
        u64 cur_lba = lba + sector;
        u8 sectors_this = 1;

        ata_select_drive(base_port, 0, cur_lba, lba48);

        if (lba48) {
            outb(base_port + ATA_REG_FEATURES, 0);
            outb(base_port + ATA_REG_SECCOUNT, sectors_this);
            outb(base_port + ATA_REG_LBA_LOW, cur_lba & 0xFF);
            outb(base_port + ATA_REG_LBA_MID, (cur_lba >> 8) & 0xFF);
            outb(base_port + ATA_REG_LBA_HIGH, (cur_lba >> 16) & 0xFF);
            outb(base_port + ATA_REG_COMMAND, ATA_CMD_READ_PIO_EXT);
        } else {
            outb(base_port + ATA_REG_SECCOUNT, sectors_this);
            outb(base_port + ATA_REG_LBA_LOW, cur_lba & 0xFF);
            outb(base_port + ATA_REG_LBA_MID, (cur_lba >> 8) & 0xFF);
            outb(base_port + ATA_REG_LBA_HIGH, (cur_lba >> 16) & 0xFF);
            outb(base_port + ATA_REG_COMMAND, ATA_CMD_READ_PIO);
        }

        if (ata_wait_drq(base_port, 500) < 0) return -1;

        /* Lire 256 mots (512 octets) */
        for (int i = 0; i < 256; i++) {
            buf[sector * 256 + i] = inw(base_port + ATA_REG_DATA);
        }
    }
    return 0;
}

/* ---- ?crire secteurs PIO ---- */
static int ata_pio_write(u16 base_port, u64 lba, u32 count, const void* buffer, int lba48) {
    const u16* buf = (const u16*)buffer;

    for (u32 sector = 0; sector < count; sector++) {
        u64 cur_lba = lba + sector;
        u8 sectors_this = 1;

        ata_select_drive(base_port, 0, cur_lba, lba48);

        if (lba48) {
            outb(base_port + ATA_REG_FEATURES, 0);
            outb(base_port + ATA_REG_SECCOUNT, sectors_this);
            outb(base_port + ATA_REG_LBA_LOW, cur_lba & 0xFF);
            outb(base_port + ATA_REG_LBA_MID, (cur_lba >> 8) & 0xFF);
            outb(base_port + ATA_REG_LBA_HIGH, (cur_lba >> 16) & 0xFF);
            outb(base_port + ATA_REG_COMMAND, ATA_CMD_WRITE_PIO_EXT);
        } else {
            outb(base_port + ATA_REG_SECCOUNT, sectors_this);
            outb(base_port + ATA_REG_LBA_LOW, cur_lba & 0xFF);
            outb(base_port + ATA_REG_LBA_MID, (cur_lba >> 8) & 0xFF);
            outb(base_port + ATA_REG_LBA_HIGH, (cur_lba >> 16) & 0xFF);
            outb(base_port + ATA_REG_COMMAND, ATA_CMD_WRITE_PIO);
        }

        if (ata_wait_drq(base_port, 500) < 0) return -1;

        /* ?crire 256 mots (512 octets) */
        for (int i = 0; i < 256; i++) {
            outw(base_port + ATA_REG_DATA, buf[sector * 256 + i]);
        }

        /* Flush cache */
        outb(base_port + ATA_REG_COMMAND, ATA_CMD_FLUSH_CACHE);
        ata_wait_ready(base_port, 1000);
    }
    return 0;
}

/* ---- Wrapper pour disk_t ---- */
static int ata_disk_read(disk_t* disk, u64 lba, u32 count, void* buffer) {
    ata_device_t* dev = (ata_device_t*)disk->private_data;
    return ata_pio_read(dev->base_port, lba, count, buffer, dev->is_lba48);
}

static int ata_disk_write(disk_t* disk, u64 lba, u32 count, const void* buffer) {
    ata_device_t* dev = (ata_device_t*)disk->private_data;
    return ata_pio_write(dev->base_port, lba, count, buffer, dev->is_lba48);
}

/* ---- Initialisation ATA ---- */
int ata_init(void) {
    vga_set_color(VGA_LIGHT_CYAN, VGA_BLACK);
    vga_write("[ATA] Initialisation du driver ATA/IDE...\n");
    vga_set_color(VGA_WHITE, VGA_BLACK);
    return ata_detect_disks();
}

/* ---- D?tecter les disques ATA ---- */
int ata_detect_disks(void) {
    u16 identify[256];
    int found = 0;

    /* Canaux ? tester */
    struct {
        u16 base;
        u16 ctrl;
        const char* name;
    } channels[] = {
        {ATA_PRIMARY_BASE, ATA_PRIMARY_CTRL, "Primary"},
        {ATA_SECONDARY_BASE, ATA_SECONDARY_CTRL, "Secondary"}
    };

    for (int ch = 0; ch < 2; ch++) {
        for (int drive = 0; drive < 2; drive++) {
            char name[32];
            snprintf(name, sizeof(name), "ata%d%d", ch, drive);

            if (ata_identify(channels[ch].base, channels[ch].ctrl, drive == 0, identify) < 0)
                continue;

            /* V?rifier que c'est un disque ATA (pas ATAPI) */
            if (identify[0] & 0x8000) continue;  /* ATAPI */

            /* Extraire infos */
            u64 sectors_28 = *(u32*)&identify[60];  /* LBA28 sectors */
            int lba48_supported = (identify[83] & (1 << 10)) != 0;
            u64 sectors_48 = 0;
            if (lba48_supported) {
                sectors_48 = *(u64*)&identify[100];  /* LBA48 sectors */
            }

            u64 total_sectors = lba48_supported ? sectors_48 : sectors_28;
            if (total_sectors == 0) continue;

            /* Nom du mod?le (identify[27..46]) */
            char model[41];
            for (int i = 0; i < 20; i++) {
                model[i*2] = identify[27+i] >> 8;
                model[i*2+1] = identify[27+i] & 0xFF;
            }
            model[40] = 0;

            /* Cr?er structure disque */
            disk_t* disk = (disk_t*)malloc(sizeof(disk_t));
            if (!disk) continue;
            memset(disk, 0, sizeof(disk_t));

            snprintf(disk->name, sizeof(disk->name), "%s", name);
            disk->type = DISK_TYPE_ATA_PIO;
            disk->sectors = total_sectors;
            disk->sector_size = 512;
            disk->size_bytes = total_sectors * 512;
            disk->read_sectors = ata_disk_read;
            disk->write_sectors = ata_disk_write;

            /* Donn?es priv?es ATA */
            ata_device_t* dev = (ata_device_t*)malloc(sizeof(ata_device_t));
            if (!dev) { free(disk); continue; }
            dev->base_port = channels[ch].base;
            dev->ctrl_port = channels[ch].ctrl;
            dev->is_master = (drive == 0);
            dev->is_lba48 = lba48_supported;
            dev->max_lba = total_sectors - 1;
            disk->private_data = dev;

            disk_register(disk);
            found++;
        }
    }

    if (found == 0) {
        vga_set_color(VGA_LIGHT_RED, VGA_BLACK);
        vga_write("[ATA] Aucun disque ATA detecte\n");
        vga_set_color(VGA_WHITE, VGA_BLACK);
    }

    return found;
}
