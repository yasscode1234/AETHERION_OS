#include "pci.h"
#include "vga.h"
#include <string.h>

/* ---- PCI Device List ---- */
#define MAX_PCI_DEVICES 32
static pci_device_t pci_devices[MAX_PCI_DEVICES];
static int pci_device_count = 0;

/* ---- I/O Ports ---- */
static inline u32 inl(u16 port) {
    u32 ret;
    __asm__ volatile ("inl %1, %0" : "=a"(ret) : "Nd"(port));
    return ret;
}

static inline void outl(u16 port, u32 val) {
    __asm__ volatile ("outl %0, %1" : : "a"(val), "Nd"(port));
}

static inline u16 inw(u16 port) {
    u16 ret;
    __asm__ volatile ("inw %1, %0" : "=a"(ret) : "Nd"(port));
    return ret;
}

static inline void outw(u16 port, u16 val) {
    __asm__ volatile ("outw %0, %1" : : "a"(val), "Nd"(port));
}

static inline u8 inb(u16 port) {
    u8 ret;
    __asm__ volatile ("inb %1, %0" : "=a"(ret) : "Nd"(port));
    return ret;
}

static inline void outb(u16 port, u8 val) {
    __asm__ volatile ("outb %0, %1" : : "a"(val), "Nd"(port));
}

/* ---- Construire l'adresse de configuration PCI ---- */
static u32 pci_make_address(u8 bus, u8 device, u8 function, u8 reg) {
    return (1u << 31) | ((u32)bus << 16) | ((u32)device << 11) | ((u32)function << 8) | (reg & 0xFC);
}

/* ---- Lire un registre de configuration PCI (32-bit) ---- */
u32 pci_read_config(pci_device_t* dev, u8 reg) {
    u32 addr = pci_make_address(dev->bus, dev->device, dev->function, reg);
    outl(PCI_CONFIG_ADDRESS, addr);
    return inl(PCI_CONFIG_DATA);
}

/* ---- ?crire un registre de configuration PCI (32-bit) ---- */
void pci_write_config(pci_device_t* dev, u8 reg, u32 value) {
    u32 addr = pci_make_address(dev->bus, dev->device, dev->function, reg);
    outl(PCI_CONFIG_ADDRESS, addr);
    outl(PCI_CONFIG_DATA, value);
}

/* ---- Lire un registre 16-bit ---- */
u16 pci_read_config16(pci_device_t* dev, u8 reg) {
    u32 val = pci_read_config(dev, reg & ~3);
    return (val >> ((reg & 3) * 8)) & 0xFFFF;
}

/* ---- ?crire un registre 16-bit ---- */
void pci_write_config16(pci_device_t* dev, u8 reg, u16 value) {
    u32 val = pci_read_config(dev, reg & ~3);
    val &= ~(0xFFFFu << ((reg & 3) * 8));
    val |= (u32)value << ((reg & 3) * 8);
    pci_write_config(dev, reg & ~3, val);
}

/* ---- Lire un registre 8-bit ---- */
u8 pci_read_config8(pci_device_t* dev, u8 reg) {
    u32 val = pci_read_config(dev, reg & ~3);
    return (val >> ((reg & 3) * 8)) & 0xFF;
}

/* ---- ?crire un registre 8-bit ---- */
void pci_write_config8(pci_device_t* dev, u8 reg, u8 value) {
    u32 val = pci_read_config(dev, reg & ~3);
    val &= ~(0xFFu << ((reg & 3) * 8));
    val |= (u32)value << ((reg & 3) * 8);
    pci_write_config(dev, reg & ~3, val);
}

/* ---- V?rifier si un device existe ---- */
static int pci_device_exists(u8 bus, u8 device, u8 function) {
    u32 addr = pci_make_address(bus, device, function, 0);
    outl(PCI_CONFIG_ADDRESS, addr);
    u32 vendor_device = inl(PCI_CONFIG_DATA);
    return vendor_device != 0xFFFFFFFF && vendor_device != 0;
}

/* ---- Lire les infos de base d'un device ---- */
static void pci_read_device_info(pci_device_t* dev) {
    dev->vendor_id = pci_read_config16(dev, 0x00);
    dev->device_id = pci_read_config16(dev, 0x02);
    dev->command = pci_read_config16(dev, 0x04);
    dev->status = pci_read_config16(dev, 0x06);
    dev->revision_id = pci_read_config8(dev, 0x08);
    dev->prog_if = pci_read_config8(dev, 0x09);
    dev->subclass = pci_read_config8(dev, 0x0A);
    dev->class_code = pci_read_config8(dev, 0x0B);
    dev->cache_line_size = pci_read_config8(dev, 0x0C);
    dev->latency_timer = pci_read_config8(dev, 0x0D);
    dev->header_type = pci_read_config8(dev, 0x0E);
    dev->bist = pci_read_config8(dev, 0x0F);

    for (int i = 0; i < 6; i++) {
        dev->bar[i] = pci_read_config(dev, 0x10 + i * 4);
    }

    dev->cardbus_cis = pci_read_config(dev, 0x28);
    dev->subsystem_vendor_id = pci_read_config16(dev, 0x2C);
    dev->subsystem_id = pci_read_config16(dev, 0x2E);
    dev->expansion_rom = pci_read_config(dev, 0x30);
    dev->capabilities_ptr = pci_read_config8(dev, 0x34);
    dev->interrupt_line = pci_read_config8(dev, 0x3C);
    dev->interrupt_pin = pci_read_config8(dev, 0x3D);
    dev->min_grant = pci_read_config8(dev, 0x3E);
    dev->max_latency = pci_read_config8(dev, 0x3F);
}

/* ---- Activer un device PCI (Memory + IO + Bus Master) ---- */
void pci_enable_device(pci_device_t* dev) {
    u16 cmd = pci_read_config16(dev, 0x04);
    cmd |= (1 << 0) | (1 << 1) | (1 << 2);  /* IO | Memory | Bus Master */
    pci_write_config16(dev, 0x04, cmd);
}

/* ---- Activer Bus Master ---- */
void pci_enable_bus_master(pci_device_t* dev) {
    u16 cmd = pci_read_config16(dev, 0x04);
    cmd |= (1 << 2);  /* Bus Master Enable */
    pci_write_config16(dev, 0x04, cmd);
}

/* ---- Obtenir l'adresse physique d'un BAR ---- */
u64 pci_get_bar_address(pci_device_t* dev, int bar_index) {
    if (bar_index < 0 || bar_index >= 6) return 0;

    u32 bar = dev->bar[bar_index];
    if (bar == 0) return 0;

    if (bar & PCI_BAR_TYPE_IO) {
        return bar & ~0x3;  /* IO port */
    } else {
        u64 addr = bar & ~0xF;  /* MMIO */
        if ((bar & PCI_BAR_TYPE_64BIT) && bar_index + 1 < 6) {
            addr |= ((u64)dev->bar[bar_index + 1]) << 32;
        }
        return addr;
    }
}

/* ---- Obtenir la taille d'un BAR ---- */
int pci_get_bar_size(pci_device_t* dev, int bar_index) {
    if (bar_index < 0 || bar_index >= 6) return 0;

    u32 original_bar = dev->bar[bar_index];
    if (original_bar == 0) return 0;

    /* ?crire tout des 1 pour lire la taille */
    pci_write_config(dev, 0x10 + bar_index * 4, 0xFFFFFFFF);
    u32 size_mask = pci_read_config(dev, 0x10 + bar_index * 4);
    pci_write_config(dev, 0x10 + bar_index * 4, original_bar);

    if (original_bar & PCI_BAR_TYPE_IO) {
        size_mask &= ~0x3;
    } else {
        size_mask &= ~0xF;
    }

    if (size_mask == 0) return 0;
    return (~size_mask) + 1;
}

/* ---- Scanner un bus PCI ---- */
int pci_scan_bus(u8 bus) {
    int found = 0;

    for (u8 device = 0; device < 32; device++) {
        for (u8 function = 0; function < 8; function++) {
            if (!pci_device_exists(bus, device, function))
                continue;

            if (pci_device_count >= MAX_PCI_DEVICES)
                return found;

            pci_device_t* dev = &pci_devices[pci_device_count];
            memset(dev, 0, sizeof(pci_device_t));
            dev->bus = bus;
            dev->device = device;
            dev->function = function;

            pci_read_device_info(dev);

            char buf[128];
            snprintf(buf, sizeof(buf),
                "[PCI] %02x:%02x.%x %04x:%04x class=%02x/%02x/%02x",
                bus, device, function,
                dev->vendor_id, dev->device_id,
                dev->class_code, dev->subclass, dev->prog_if);
            vga_write(buf);

            if (dev->class_code == PCI_CLASS_BRIDGE && dev->subclass == 0x04) {
                vga_write(" [PCI-PCI Bridge]");
                u8 secondary_bus = pci_read_config8(dev, 0x19);
                snprintf(buf, sizeof(buf), " -> bus %d", secondary_bus);
                vga_write(buf);
                pci_scan_bus(secondary_bus);
            }
            vga_write("\n");

            pci_device_count++;
            found++;

            /* Si pas multi-function, arr?ter apr?s function 0 */
            if (function == 0 && (dev->header_type & 0x80) == 0)
                break;
        }
    }
    return found;
}

/* ---- Initialiser le sous-syst?me PCI ---- */
void pci_init(void) {
    vga_set_color(VGA_LIGHT_CYAN, VGA_BLACK);
    vga_write("[PCI] Initialisation du sous-systeme PCI...\n");
    vga_set_color(VGA_WHITE, VGA_BLACK);

    pci_device_count = 0;
    memset(pci_devices, 0, sizeof(pci_devices));

    pci_scan_bus(0);

    char buf[64];
    snprintf(buf, sizeof(buf), "[PCI] %d peripheriques detectes\n", pci_device_count);
    vga_write(buf);
}

/* ---- Trouver un device par Vendor/Device ID ---- */
pci_device_t* pci_find_device(u16 vendor_id, u16 device_id) {
    for (int i = 0; i < pci_device_count; i++) {
        if (pci_devices[i].vendor_id == vendor_id &&
            pci_devices[i].device_id == device_id) {
            return &pci_devices[i];
        }
    }
    return NULL;
}

/* ---- Trouver un device par classe/sous-classe/prog-if ---- */
pci_device_t* pci_find_class(u8 class_code, u8 subclass, u8 prog_if) {
    for (int i = 0; i < pci_device_count; i++) {
        if (pci_devices[i].class_code == class_code &&
            pci_devices[i].subclass == subclass &&
            (prog_if == 0xFF || pci_devices[i].prog_if == prog_if)) {
            return &pci_devices[i];
        }
    }
    return NULL;
}

/* ---- Obtenir un device par bus/device/function ---- */
pci_device_t* pci_get_device(u8 bus, u8 device, u8 function) {
    for (int i = 0; i < pci_device_count; i++) {
        if (pci_devices[i].bus == bus &&
            pci_devices[i].device == device &&
            pci_devices[i].function == function) {
            return &pci_devices[i];
        }
    }
    return NULL;
}

/* ---- Lister tous les devices PCI ---- */
void pci_list_devices(void) {
    vga_set_color(VGA_YELLOW, VGA_BLACK);
    vga_write("\n=== PCI Devices ===\n");
    vga_set_color(VGA_WHITE, VGA_BLACK);

    for (int i = 0; i < pci_device_count; i++) {
        pci_device_t* dev = &pci_devices[i];
        char buf[256];
        const char* class_name = pci_class_name(dev->class_code, dev->subclass);

        snprintf(buf, sizeof(buf),
            "[%d] %02x:%02x.%x %04x:%04x %s (prog_if=%02x)\n"
            "     CMD=%04x STATUS=%04x REV=%02x HDR=%02x\n"
            "     BAR0=%08x BAR1=%08x BAR2=%08x BAR3=%08x BAR4=%08x BAR5=%08x\n"
            "     INT=%02x/%02x CAP=%02x\n",
            i, dev->bus, dev->device, dev->function,
            dev->vendor_id, dev->device_id,
            class_name, dev->prog_if,
            dev->command, dev->status, dev->revision_id, dev->header_type,
            dev->bar[0], dev->bar[1], dev->bar[2], dev->bar[3], dev->bar[4], dev->bar[5],
            dev->interrupt_line, dev->interrupt_pin, dev->capabilities_ptr);
        vga_write(buf);
    }
    vga_write("\n");
}

/* ---- Noms des classes PCI ---- */
const char* pci_class_name(u8 class_code, u8 subclass) {
    switch (class_code) {
        case PCI_CLASS_UNCLASSIFIED: return "Unclassified";
        case PCI_CLASS_MASS_STORAGE:
            switch (subclass) {
                case PCI_SUBCLASS_SCSI: return "SCSI";
                case PCI_SUBCLASS_IDE: return "IDE";
                case PCI_SUBCLASS_FLOPPY: return "Floppy";
                case PCI_SUBCLASS_RAID: return "RAID";
                case PCI_SUBCLASS_ATA: return "ATA";
                case PCI_SUBCLASS_SATA: return "SATA";
                case PCI_SUBCLASS_SAS: return "SAS";
                case PCI_SUBCLASS_NVME: return "NVMe";
                default: return "Mass Storage";
            }
        case PCI_CLASS_NETWORK: return "Network";
        case PCI_CLASS_DISPLAY: return "Display";
        case PCI_CLASS_MULTIMEDIA: return "Multimedia";
        case PCI_CLASS_MEMORY: return "Memory";
        case PCI_CLASS_BRIDGE:
            switch (subclass) {
                case 0x00: return "Host Bridge";
                case 0x01: return "ISA Bridge";
                case 0x02: return "EISA Bridge";
                case 0x03: return "MCA Bridge";
                case 0x04: return "PCI-PCI Bridge";
                case 0x05: return "PCMCIA Bridge";
                case 0x06: return "NuBus Bridge";
                case 0x07: return "CardBus Bridge";
                default: return "Bridge";
            }
        case PCI_CLASS_COMMUNICATION: return "Communication";
        case PCI_CLASS_BASE_PERIPHERAL: return "Base Peripheral";
        case PCI_CLASS_INPUT: return "Input";
        case PCI_CLASS_DOCKING: return "Docking";
        case PCI_CLASS_PROCESSOR: return "Processor";
        case PCI_CLASS_SERIAL_BUS:
            switch (subclass) {
                case 0x00: return "FireWire";
                case 0x01: return "ACCESS.bus";
                case 0x02: return "SSA";
                case 0x03: return "USB";
                case 0x04: return "Fibre Channel";
                case 0x05: return "SMBus";
                case 0x06: return "InfiniBand";
                case 0x07: return "IPMI";
                case 0x08: return "SERCOS";
                case 0x09: return "CANbus";
                default: return "Serial Bus";
            }
        case PCI_CLASS_WIRELESS: return "Wireless";
        case PCI_CLASS_INTELLIGENT_IO: return "Intelligent I/O";
        case PCI_CLASS_SATELLITE: return "Satellite";
        case PCI_CLASS_ENCRYPTION: return "Encryption";
        case PCI_CLASS_SIGNAL_PROCESSING: return "Signal Processing";
        default: return "Unknown";
    }
}
