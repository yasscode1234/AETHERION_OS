#include "pci.h"
#include "types.h"

// fonctions IO (port) - en asm inline
static inline void outl(uint16_t port, uint32_t val) {
    __asm__ volatile ("outl %0, %1" :: "a"(val), "Nd"(port));
}
static inline uint32_t inl(uint16_t port) {
    uint32_t val;
    __asm__ volatile ("inl %1, %0" : "=a"(val) : "Nd"(port));
    return val;
}

// Construire adresse config
static inline uint32_t pci_address(uint8_t bus, uint8_t device, uint8_t function, uint8_t offset) {
    uint32_t addr = (uint32_t)(
        (1U << 31) |                     // enable bit
        ((uint32_t)bus << 16) |
        ((uint32_t)device << 11) |
        ((uint32_t)function << 8) |
        (offset & 0xFC)
    );
    return addr;
}

uint32_t pci_config_read32(uint8_t bus, uint8_t device, uint8_t function, uint8_t offset) {
    outl(PCI_CONFIG_ADDRESS, pci_address(bus, device, function, offset));
    return inl(PCI_CONFIG_DATA);
}

void pci_config_write32(uint8_t bus, uint8_t device, uint8_t function, uint8_t offset, uint32_t value) {
    outl(PCI_CONFIG_ADDRESS, pci_address(bus, device, function, offset));
    outl(PCI_CONFIG_DATA, value);
}

static void fill_basic_pci_info(uint8_t bus, uint8_t device, uint8_t function, pci_device_t* dev) {
    uint32_t v;

    v = pci_config_read32(bus, device, function, 0x00);
    dev->vendor_id = (uint16_t)(v & 0xFFFF);
    dev->device_id = (uint16_t)((v >> 16) & 0xFFFF);

    v = pci_config_read32(bus, device, function, 0x08);
    dev->class_code = (uint8_t)((v >> 24) & 0xFF);
    dev->subclass   = (uint8_t)((v >> 16) & 0xFF);
    dev->prog_if    = (uint8_t)((v >> 8)  & 0xFF);

    dev->bus = bus;
    dev->device = device;
    dev->function = function;

    // lire BARs
    for(int i=0;i<6;i++){
        uint8_t off = 0x10 + i*4;
        dev->bar[i] = pci_config_read32(bus, device, function, off);
    }
}

void pci_scan_bus(pci_device_callback_t cb) {
    for(uint8_t bus=0; bus<256; bus++){
        for(uint8_t dev=0; dev<32; dev++){
            for(uint8_t func=0; func<8; func++){
                uint32_t v = pci_config_read32(bus, dev, func, 0x00);
                uint16_t vendor = (uint16_t)(v & 0xFFFF);
                if(vendor == 0xFFFF) {
                    if(func==0) break; // no device in this slot
                    else continue;
                }
                pci_device_t info;
                fill_basic_pci_info(bus, dev, func, &info);
                if(cb) cb(&info);
                // If single function device and func==0 no multi functions -> skip funcs
                if(func==0) {
                    uint32_t hdr = pci_config_read32(bus, dev, func, 0x0C);
                    // bit 7..11 of header defines multi-function? actually check header type
                    uint8_t header_type = (hdr >> 16) & 0xFF;
                    if((header_type & 0x80) == 0) break;
                }
            }
        }
    }
}

int pci_find_device(uint8_t class_code, uint8_t subclass, uint8_t prog_if, int index, pci_device_t* dev_out) {
    int found = 0;
    void cb(const pci_device_t* d){
        if(d->class_code == class_code && d->subclass == subclass && (prog_if==0xFF || d->prog_if == prog_if)) {
            if(found == index) {
                if(dev_out) *dev_out = *d;
            }
            found++;
        }
    }
    pci_scan_bus(cb);
    return (found > index) ? 0 : -1;
}

int pci_get_bar(const pci_device_t* dev, int bar_index, uint64_t* bar) {
    if(!dev || bar_index < 0 || bar_index > 5) return -1;
    uint32_t raw = dev->bar[bar_index];
    if((raw & 1) == 1) {
        // I/O port BAR (not handled)
        *bar = (uint64_t)(raw & ~0x3);
        return 0;
    } else {
        // MMIO BAR
        if((raw & 0xFFFFFFF0u) == 0) {
            // need to probe by writing all 1s then read — skipping for simplicity
            *bar = 0;
            return -1;
        }
        // detect 64-bit BAR (type 2)
        uint8_t type = (raw >> 1) & 0x3;
        if(type == 2) {
            // 64-bit: low is at this BAR, high at next BAR
            uint32_t low = raw & 0xFFFFFFF0u;
            // read upper dword
            // Note: we assume dev->bar cached contains the upper dword in dev->bar[bar_index+1]
            uint32_t high = dev->bar[bar_index+1];
            *bar = ((uint64_t)high << 32) | low;
            return 0;
        } else {
            *bar = (uint64_t)(raw & 0xFFFFFFF0u);
            return 0;
        }
    }
}
