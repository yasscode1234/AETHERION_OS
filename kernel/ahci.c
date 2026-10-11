#include "ahci.h"
#include "disk.h"
#include "vga.h"
#include "string.h"
#include "stdlib.h"

/* ---- PCI Configuration ---- */
#define PCI_CONFIG_ADDRESS  0xCF8
#define PCI_CONFIG_DATA     0xCFC

#define PCI_CLASS_STORAGE   0x01
#define PCI_SUBCLASS_SATA   0x06
#define PCI_PROG_IF_AHCI    0x01

/* ---- I/O ---- */
static inline u32 inl(u32 port) {
    u32 ret;
    __asm__ volatile ("inl %w1, %0" : "=a"(ret) : "Nd"(port));
    return ret;
}

static inline void outl(u32 port, u32 val) {
    __asm__ volatile ("outl %0, %w1" : : "a"(val), "Nd"(port));
}

static inline u8 inb(u16 port) {
    u8 ret;
    __asm__ volatile ("inb %w1, %0" : "=a"(ret) : "Nd"(port));
    return ret;
}

static inline void outb(u16 port, u8 val) {
    __asm__ volatile ("outb %0, %w1" : : "a"(val), "Nd"(port));
}

static inline u32 pci_read32(u8 bus, u8 dev, u8 func, u8 reg) {
    u32 addr = (1u << 31) | (bus << 16) | (dev << 11) | (func << 8) | (reg & 0xFC);
    outb(PCI_CONFIG_ADDRESS, addr >> 24);
    outb(PCI_CONFIG_ADDRESS + 1, addr >> 16);
    outb(PCI_CONFIG_ADDRESS + 2, addr >> 8);
    outb(PCI_CONFIG_ADDRESS + 3, addr);
    u32 val;
    __asm__ volatile ("inl %w1, %0" : "=a"(val) : "Nd"(PCI_CONFIG_DATA));
    return val;
}

static inline void pci_write32(u8 bus, u8 dev, u8 func, u8 reg, u32 val) {
    u32 addr = (1u << 31) | (bus << 16) | (dev << 11) | (func << 8) | (reg & 0xFC);
    outb(PCI_CONFIG_ADDRESS, addr >> 24);
    outb(PCI_CONFIG_ADDRESS + 1, addr >> 16);
    outb(PCI_CONFIG_ADDRESS + 2, addr >> 8);
    outb(PCI_CONFIG_ADDRESS + 3, addr);
    __asm__ volatile ("outl %0, %w1" : : "a"(val), "Nd"(PCI_CONFIG_DATA));
}

/* ---- Allocateur m?moire simple (align?) ---- */
static u64 g_mem_alloc_ptr = 0x1000000;  /* 16 Mo */

static void* ahci_alloc_aligned(size_t size, size_t align) {
    u64 ptr = (g_mem_alloc_ptr + align - 1) & ~(align - 1);
    g_mem_alloc_ptr = ptr + size;
    return (void*)ptr;
}

/* ---- HBA Global ---- */
static ahci_hba_t g_hba = {0};

/* ---- Lire registre HBA ---- */
static inline u32 hba_read(u32 offset) {
    return *(volatile u32*)(g_hba.abar + offset);
}

/* ---- ?crire registre HBA ---- */
static inline void hba_write(u32 offset, u32 val) {
    *(volatile u32*)(g_hba.abar + offset) = val;
}

/* ---- Lire registre Port ---- */
static inline u32 port_read(ahci_port_t* port, u32 offset) {
    return *(volatile u32*)(g_hba.abar + 0x100 + port->port_num * 0x80 + offset);
}

/* ---- ?crire registre Port ---- */
static inline void port_write(ahci_port_t* port, u32 offset, u32 val) {
    *(volatile u32*)(g_hba.abar + 0x100 + port->port_num * 0x80 + offset) = val;
}

/* ---- Attendre bit ---- */
static int ahci_wait_bit(volatile u32* reg, u32 mask, int expected, u32 timeout_us) {
    for (u32 i = 0; i < timeout_us; i++) {
        if ((*reg & mask) == (expected ? mask : 0))
            return 0;
        __asm__ volatile ("pause" : : : "memory");
    }
    return -1;
}

/* ---- D?marrer un port ---- */
int ahci_port_start(ahci_port_t* port) {
    /* Arr?ter d'abord */
    port_write(port, AHCI_PORT_CMD, port_read(port, AHCI_PORT_CMD) & ~AHCI_CMD_ST);
    ahci_wait_bit((volatile u32*)(g_hba.abar + 0x100 + port->port_num * 0x80 + AHCI_PORT_CMD),
                  AHCI_CMD_CR | AHCI_CMD_FR, 0, 1000000);

    /* Configurer Command List Base */
    port_write(port, AHCI_PORT_CLB, (u32)port->cmd_list_base);
    if (g_hba.cap_64bit)
        port_write(port, AHCI_PORT_CLBU, (u32)(port->cmd_list_base >> 32));

    /* Configurer FIS Base */
    port_write(port, AHCI_PORT_FB, (u32)port->fis_base);
    if (g_hba.cap_64bit)
        port_write(port, AHCI_PORT_FBU, (u32)(port->fis_base >> 32));

    /* Activer FIS Receive et Start */
    u32 cmd = port_read(port, AHCI_PORT_CMD);
    cmd |= AHCI_CMD_FRE | AHCI_CMD_ST;
    port_write(port, AHCI_PORT_CMD, cmd);

    /* Attendre que CR et FR soient actifs */
    if (ahci_wait_bit((volatile u32*)(g_hba.abar + 0x100 + port->port_num * 0x80 + AHCI_PORT_CMD),
                      AHCI_CMD_CR | AHCI_CMD_FR, 1, 1000000) < 0) {
        vga_write("[AHCI] Timeout demarrage port\n");
        return -1;
    }
    return 0;
}

/* ---- Arr?ter un port ---- */
int ahci_port_stop(ahci_port_t* port) {
    u32 cmd = port_read(port, AHCI_PORT_CMD);
    cmd &= ~(AHCI_CMD_ST | AHCI_CMD_FRE);
    port_write(port, AHCI_PORT_CMD, cmd);
    return ahci_wait_bit((volatile u32*)(g_hba.abar + 0x100 + port->port_num * 0x80 + AHCI_PORT_CMD),
                         AHCI_CMD_CR | AHCI_CMD_FR, 0, 1000000);
}

/* ---- Ex?cuter une commande ATA ---- */
static int ahci_exec_cmd(ahci_port_t* port, u8 cmd_fis[64], void* buffer, u32 count, int is_write) {
    ahci_cmd_header_t* hdr = &port->cmd_headers[port->cmd_slot];
    ahci_cmd_table_t* tbl = (ahci_cmd_table_t*)port->cmd_table_base;

    /* Pr?parer header */
    memset(hdr, 0, sizeof(ahci_cmd_header_t));
    hdr->cfl = 5;  /* 5 DWORDS = 20 bytes */
    hdr->w = is_write;
    hdr->prdtl = 1;  /* 1 PRDT entry */

    /* Pr?parer PRDT */
    ahci_prdt_entry_t* prdt = &tbl->prdt[0];
    prdt->dba = (u32)(u64)buffer;
    prdt->dbau = g_hba.cap_64bit ? (u32)((u64)buffer >> 32) : 0;
    prdt->dbc = count - 1;  /* 0-based */
    prdt->i = 1;

    /* Copier Command FIS */
    memcpy(tbl->cfis, cmd_fis, 64);

    /* Vider le slot */
    port_write(port, AHCI_PORT_CI, 1 << port->cmd_slot);

    /* Attendre completion */
    u32 timeout = 5000000;
    while (timeout--) {
        u32 ci = port_read(port, AHCI_PORT_CI);
        if (!(ci & (1 << port->cmd_slot)))
            break;
        __asm__ volatile ("pause" : : : "memory");
    }
    if (timeout == 0) return -1;

    /* V?rifier erreurs */
    u32 is = port_read(port, AHCI_PORT_IS);
    if (is & 0x40000000) {  /* TFES - Task File Error */
        vga_write("[AHCI] Task File Error\n");
        return -1;
    }
    return 0;
}

/* ---- Construire FIS Register H2D ---- */
static void ahci_build_reg_fis(u8* fis, u8 command, u64 lba, u32 count, int lba48) {
    memset(fis, 0, 64);
    fis[0] = AHCI_FIS_TYPE_REG_H2D;
    fis[1] = 1 << 7;  /* C=1 (Command) */
    fis[2] = command;
    fis[3] = 0;  /* Features */

    if (lba48) {
        fis[4] = (lba >> 24) & 0xFF;   /* LBA Low (high) */
        fis[5] = (lba >> 32) & 0xFF;   /* LBA Mid (high) */
        fis[6] = (lba >> 40) & 0xFF;   /* LBA High (high) */
        fis[7] = 0;                     /* Device */
        fis[8] = lba & 0xFF;            /* LBA Low */
        fis[9] = (lba >> 8) & 0xFF;     /* LBA Mid */
        fis[10] = (lba >> 16) & 0xFF;   /* LBA High */
        fis[11] = (count >> 8) & 0xFF;  /* Count High */
        fis[12] = count & 0xFF;         /* Count Low */
    } else {
        fis[4] = 0;
        fis[5] = 0;
        fis[6] = 0;
        fis[7] = 0x40 | ((lba >> 24) & 0x0F);  /* LBA mode */
        fis[8] = lba & 0xFF;
        fis[9] = (lba >> 8) & 0xFF;
        fis[10] = (lba >> 16) & 0xFF;
        fis[11] = 0;
        fis[12] = count & 0xFF;
    }
    fis[13] = 0;  /* Control */
}

/* ---- Wrapper read pour disk_t ---- */
int ahci_read_sectors(disk_t* disk, u64 lba, u32 count, void* buffer) {
    ahci_port_t* port = (ahci_port_t*)disk->private_data;
    u8 fis[64];
    int lba48 = (lba + count > 0xFFFFFFF);

    if (lba48)
        ahci_build_reg_fis(fis, 0x25, lba, count, 1);  /* READ DMA EXT */
    else
        ahci_build_reg_fis(fis, 0xC8, lba, count, 0);  /* READ DMA */

    return ahci_exec_cmd(port, fis, buffer, count * 512, 0);
}

/* ---- Wrapper write pour disk_t ---- */
int ahci_write_sectors(disk_t* disk, u64 lba, u32 count, const void* buffer) {
    ahci_port_t* port = (ahci_port_t*)disk->private_data;
    u8 fis[64];
    int lba48 = (lba + count > 0xFFFFFFF);

    if (lba48)
        ahci_build_reg_fis(fis, 0x35, lba, count, 1);  /* WRITE DMA EXT */
    else
        ahci_build_reg_fis(fis, 0xCA, lba, count, 0);  /* WRITE DMA */

    return ahci_exec_cmd(port, fis, (void*)buffer, count * 512, 1);
}

/* ---- Identifier un p?riph?rique ---- */
static int ahci_identify(ahci_port_t* port, u16* identify) {
    u8 fis[64];
    memset(fis, 0, 64);
    fis[0] = AHCI_FIS_TYPE_REG_H2D;
    fis[1] = 1 << 7;
    fis[2] = 0xEC;  /* IDENTIFY DEVICE */
    fis[3] = 0;
    fis[7] = 0x40;  /* LBA mode */

    return ahci_exec_cmd(port, fis, identify, 512, 0);
}

/* ---- Initialiser AHCI ---- */
int ahci_init(void) {
    vga_set_color(VGA_LIGHT_CYAN, VGA_BLACK);
    vga_write("[AHCI] Recherche controleur AHCI...\n");
    vga_set_color(VGA_WHITE, VGA_BLACK);

    /* Scanner le bus PCI pour trouver un controleur AHCI */
    for (u16 bus = 0; bus < 256; bus++) {
        for (u8 dev = 0; dev < 32; dev++) {
            for (u8 func = 0; func < 8; func++) {
                u32 class_code = pci_read32(bus, dev, func, 0x08);
                u8 class = (class_code >> 24) & 0xFF;
                u8 subclass = (class_code >> 16) & 0xFF;
                u8 prog_if = (class_code >> 8) & 0xFF;

                if (class == PCI_CLASS_STORAGE && subclass == PCI_SUBCLASS_SATA && prog_if == PCI_PROG_IF_AHCI) {
                    char buf[64];
                    snprintf(buf, sizeof(buf), "[AHCI] Controleur trouve: bus=%d dev=%d func=%d\n", bus, dev, func);
                    vga_write(buf);

                    /* Lire ABAR (Base Address Register 5) */
                    u32 bar5 = pci_read32(bus, dev, func, 0x24);
                    g_hba.abar = bar5 & ~0xF;
                    if (bar5 & 0x4) {  /* 64-bit BAR */
                        u32 bar5_upper = pci_read32(bus, dev, func, 0x28);
                        g_hba.abar |= ((u64)bar5_upper) << 32;
                    }

                    snprintf(buf, sizeof(buf), "[AHCI] ABAR = 0x%llx\n", g_hba.abar);
                    vga_write(buf);

                    /* Activer AHCI */
                    u32 ghc = hba_read(AHCI_HBA_GHC);
                    if (!(ghc & AHCI_GHC_AE)) {
                        hba_write(AHCI_HBA_GHC, ghc | AHCI_GHC_AE);
                    }
                    hba_write(AHCI_HBA_GHC, hba_read(AHCI_HBA_GHC) | AHCI_GHC_IE);

                    /* Lire capabilities */
                    u32 cap = hba_read(AHCI_HBA_CAP);
                    g_hba.cap_64bit = (cap & AHCI_CAP_S64A) != 0;
                    g_hba.cap_ncq = (cap & AHCI_CAP_SNCQ) != 0;
                    g_hba.ports_impl = hba_read(AHCI_HBA_PI);
                    g_hba.num_ports = 0;

                    for (int i = 0; i < 32; i++) {
                        if (g_hba.ports_impl & (1 << i))
                            g_hba.num_ports++;
                    }

                    snprintf(buf, sizeof(buf), "[AHCI] Ports: %d, 64-bit: %s, NCQ: %s\n",
                             g_hba.num_ports, g_hba.cap_64bit ? "oui" : "non", g_hba.cap_ncq ? "oui" : "non");
                    vga_write(buf);

                    return ahci_detect_ports();
                }
            }
        }
    }

    vga_set_color(VGA_LIGHT_RED, VGA_BLACK);
    vga_write("[AHCI] Aucun controleur AHCI trouve\n");
    vga_set_color(VGA_WHITE, VGA_BLACK);
    return 0;
}

/* ---- D?tecter les ports ---- */
int ahci_detect_ports(void) {
    int found = 0;

    for (int i = 0; i < 32; i++) {
        if (!(g_hba.ports_impl & (1 << i)))
            continue;

        ahci_port_t* port = &g_hba.ports[i];
        port->port_num = i;

        /* V?rifier signature */
        u32 sig = port_read(port, AHCI_PORT_SIG);
        if (sig != AHCI_SIG_ATA && sig != AHCI_SIG_ATAPI) {
            char buf[64];
            snprintf(buf, sizeof(buf), "[AHCI] Port %d: signature 0x%x (non ATA)\n", i, sig);
            vga_write(buf);
            continue;
        }

        /* Allouer m?moire pour structures */
        port->cmd_list_base = (u64)ahci_alloc_aligned(1024, 1024);  /* 1K aligned */
        port->fis_base = (u64)ahci_alloc_aligned(256, 256);         /* 256 aligned */
        port->cmd_table_base = (u64)ahci_alloc_aligned(4096, 4096); /* 4K aligned */

        port->cmd_headers = (ahci_cmd_header_t*)port->cmd_list_base;
        port->rfis = (ahci_received_fis_t*)port->fis_base;

        /* D?marrer le port */
        if (ahci_port_start(port) < 0) {
            vga_write("[AHCI] Echec demarrage port\n");
            continue;
        }

        /* Identifier le disque */
        u16 identify[256];
        if (ahci_identify(port, identify) < 0) {
            vga_write("[AHCI] Echec IDENTIFY\n");
            continue;
        }

        /* Extraire infos */
        u64 sectors_28 = *(u32*)&identify[60];
        int lba48_supported = (identify[83] & (1 << 10)) != 0;
        u64 sectors_48 = 0;
        if (lba48_supported) {
            sectors_48 = *(u64*)&identify[100];
        }
        u64 total_sectors = lba48_supported ? sectors_48 : sectors_28;
        if (total_sectors == 0) continue;

        /* Mod?le */
        char model[41];
        for (int j = 0; j < 20; j++) {
            model[j*2] = identify[27+j] >> 8;
            model[j*2+1] = identify[27+j] & 0xFF;
        }
        model[40] = 0;

        /* Cr?er disque */
        disk_t* disk = (disk_t*)malloc(sizeof(disk_t));
        if (!disk) continue;
        memset(disk, 0, sizeof(disk_t));

        char name[32];
        snprintf(name, sizeof(name), "ahci%d", i);
        snprintf(disk->name, sizeof(disk->name), "%s", name);
        disk->type = DISK_TYPE_SATA_AHCI;
        disk->sectors = total_sectors;
        disk->sector_size = 512;
        disk->size_bytes = total_sectors * 512;
        disk->read_sectors = ahci_read_sectors;
        disk->write_sectors = ahci_write_sectors;
        disk->private_data = port;

        disk_register(disk);
        found++;
    }

    return found;
}
