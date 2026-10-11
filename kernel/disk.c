#include "disk.h"
#include "vga.h"
#include "string.h"
#include "stdlib.h"

/* ---- Registre global des disques ---- */
disk_t* g_disks[MAX_DISKS] = {0};
int g_disk_count = 0;

/* ---- Initialisation du sous-syst?me disque ---- */
void disk_init(void) {
    vga_set_color(VGA_LIGHT_CYAN, VGA_BLACK);
    vga_write("[DISK] Initialisation du sous-systeme disque...\n");
    vga_set_color(VGA_WHITE, VGA_BLACK);
    g_disk_count = 0;
    memset(g_disks, 0, sizeof(g_disks));
}

/* ---- Enregistrer un disque ---- */
int disk_register(disk_t* disk) {
    if (!disk || g_disk_count >= MAX_DISKS) {
        vga_write("[DISK] Erreur: impossible d'enregistrer le disque\n");
        return -1;
    }

    g_disks[g_disk_count] = disk;
    g_disk_count++;

    vga_set_color(VGA_LIGHT_GREEN, VGA_BLACK);
    vga_write("[DISK] Disque enregistre: ");
    vga_write(disk->name);
    vga_write(" (");
    char buf[32];
    snprintf(buf, sizeof(buf), "%llu", disk->size_bytes / (1024*1024));
    vga_write(buf);
    vga_write(" MB)\n");
    vga_set_color(VGA_WHITE, VGA_BLACK);

    return g_disk_count - 1;
}

/* ---- Obtenir un disque par index ---- */
disk_t* disk_get(int index) {
    if (index < 0 || index >= g_disk_count)
        return NULL;
    return g_disks[index];
}

/* ---- Trouver un disque par nom ---- */
disk_t* disk_find_by_name(const char* name) {
    for (int i = 0; i < g_disk_count; i++) {
        if (g_disks[i] && strcmp(g_disks[i]->name, name) == 0)
            return g_disks[i];
    }
    return NULL;
}

/* ---- Lire des secteurs ---- */
int disk_read(disk_t* disk, u64 lba, u32 count, void* buffer) {
    if (!disk || !disk->read_sectors) {
        vga_write("[DISK] Erreur: lecture non supportee\n");
        return -1;
    }
    return disk->read_sectors(disk, lba, count, buffer);
}

/* ---- ?crire des secteurs ---- */
int disk_write(disk_t* disk, u64 lba, u32 count, const void* buffer) {
    if (!disk || !disk->write_sectors) {
        vga_write("[DISK] Erreur: ecriture non supportee\n");
        return -1;
    }
    return disk->write_sectors(disk, lba, count, buffer);
}

/* ---- Lister les disques ---- */
void disk_list(void) {
    vga_set_color(VGA_YELLOW, VGA_BLACK);
    vga_write("\n=== Disques detectes ===\n");
    vga_set_color(VGA_WHITE, VGA_BLACK);

    if (g_disk_count == 0) {
        vga_write("  Aucun disque detecte\n");
        return;
    }

    for (int i = 0; i < g_disk_count; i++) {
        disk_t* d = g_disks[i];
        if (!d) continue;

        char buf[128];
        const char* type_str = "Unknown";
        switch (d->type) {
            case DISK_TYPE_ATA_PIO: type_str = "ATA PIO"; break;
            case DISK_TYPE_ATA_DMA: type_str = "ATA DMA"; break;
            case DISK_TYPE_SATA_AHCI: type_str = "SATA AHCI"; break;
            case DISK_TYPE_NVME: type_str = "NVMe"; break;
            case DISK_TYPE_USB: type_str = "USB"; break;
            case DISK_TYPE_VIRTIO: type_str = "VirtIO"; break;
        }

        snprintf(buf, sizeof(buf),
            "  [%d] %s | %s | %llu secteurs | %u octets/secteur | %llu MB\n",
            i, d->name, type_str, d->sectors, d->sector_size,
            d->size_bytes / (1024*1024));
        vga_write(buf);
    }
    vga_write("\n");
}

/* ---- Obtenir la taille du disque ---- */
u64 disk_get_size(disk_t* disk) {
    if (!disk) return 0;
    return disk->size_bytes;
}
