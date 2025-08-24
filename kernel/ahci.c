#include "ahci.h"
#include "pci.h"
#include "types.h"
#include "vga.h"
#include <string.h>
#include <stddef.h>

// Pointer to HBA memory (MMIO)
static HBA_MEM* abar = NULL;

// Helper: read 32 from MMIO
static inline uint32_t mmio_read32(volatile uint32_t* addr) {
    return *addr;
}
static inline void mmio_write32(volatile uint32_t* addr, uint32_t val) {
    *addr = val;
}

// Utility: wait (naive)
static void delay(int cycles) {
    for(volatile int i=0;i<cycles;i++) __asm__ volatile("nop");
}

// Stop command engine on port
static void stop_cmd(HBA_PORT* port) {
    // Clear ST (start) bit
    port->cmd &= ~0x01;
    // Clear FRE (FIS Receive Enable)
    port->cmd &= ~0x10;
    // Wait until CR (command list running) and FR (FIS rx running) cleared
    while (port->cmd & (1<<15) || port->cmd & (1<<14)) {
        // wait
    }
}

// Start command engine on port
static void start_cmd(HBA_PORT* port) {
    // Set FRE and ST
    while (port->cmd & (1<<15)) ; // wait CR clear
    port->cmd |= 1<<4; // FRE = 1
    port->cmd |= 1<<0; // ST  = 1
}

// Rebase port: allocate CLB and FIS base
static void port_rebase(HBA_PORT* port, int portno) {
    stop_cmd(port);

    // For a real driver you'd allocate physical memory aligned.
    // Here we assume identity mapping and statically allocate buffers.
    // Each port uses:
    //  - Command list (1 KiB aligned)
    //  - FIS (256 bytes aligned)
    static uint8_t cmdlist_mem[32*1024];
    static uint8_t fis_mem[4*1024];

    memset(&cmdlist_mem[0], 0, sizeof(cmdlist_mem));
    memset(&fis_mem[0], 0, sizeof(fis_mem));

    uint64_t clb = (uint64_t)(uintptr_t)&cmdlist_mem;
    uint64_t fb  = (uint64_t)(uintptr_t)&fis_mem;

    port->clb = (uint32_t)(clb & 0xFFFFFFFF);
    port->clb_h = (uint32_t)((clb >> 32) & 0xFFFFFFFF);
    port->fb  = (uint32_t)(fb & 0xFFFFFFFF);
    port->fb_h = (uint32_t)((fb >> 32) & 0xFFFFFFFF);

    // Command headers are at clb
    HBA_CMD_HEADER* cmdheader = (HBA_CMD_HEADER*)(uintptr_t)clb;
    for(int i=0;i<32;i++){
        cmdheader[i].prdtl = 0;
    }

    start_cmd(port);
}

// Identify devices in ABAR
static void probe_ports(HBA_MEM* hba) {
    uint32_t pi = hba->pi;
    for(int i=0; i<32; i++) {
        if(pi & (1<<i)) {
            HBA_PORT* port = (HBA_PORT*)((uintptr_t)hba + 0x100 + i * 0x80);
            uint32_t sstatus = port->ssts;
            uint8_t ipm = (sstatus >> 8) & 0x0F;
            uint8_t det = sstatus & 0x0F;
            if(det != 3) continue; // no device
            if(ipm != 1) continue; // active state
            uint32_t sig = port->sig;
            if(sig == SATA_SIG_ATA) {
                // ATA drive
                char msg[64];
                snprintf(msg, sizeof(msg), "AHCI: port %d -> ATA device", i);
                vga_draw_text(0, 3+i, msg, 10);
                port_rebase(port, i);
            } else if(sig == SATA_SIG_ATAPI) {
                vga_draw_text(0,3+i,"AHCI: ATAPI device (not supported)", 12);
            } else {
                vga_draw_text(0,3+i,"AHCI: Unknown device", 12);
            }
        }
    }
}

int ahci_init(void) {
    // Find AHCI PCI device
    pci_device_t dev;
    if(pci_find_device(PCI_CLASS_MASS_STORAGE, PCI_SUBCLASS_SATA, PCI_PROGIF_AHCI, 0, &dev) != 0) {
        vga_draw_text(0,0,"AHCI PCI device not found", 12);
        return -1;
    }

    uint64_t abar_phys;
    if(pci_get_bar(&dev, 5, &abar_phys) != 0) {
        vga_draw_text(0,0,"AHCI ABAR not found", 12);
        return -1;
    }
    if(abar_phys == 0) {
        vga_draw_text(0,0,"AHCI ABAR is zero", 12);
        return -1;
    }

    // For now assume identity mapping: virtual == physical
    abar = (HBA_MEM*)(uintptr_t)abar_phys;

    uint32_t cap = abar->cap;
    char buf[64];
    snprintf(buf, sizeof(buf), "AHCI: CAP = 0x%08X", cap);
    vga_draw_text(0,1,buf,10);

    // Enable AHCI mode: set AE bit in GHC (bit 31), and HBA interrupt enable?
    abar->ghc |= (1<<31);
    // probe ports and rebase
    probe_ports(abar);
    return 0;
}

/**
 * ahci_read(portnum, start_lba, sector_count, buffer)
 *  - Very simplified: assumes buffer is physically contiguous (virt==phys)
 *  - Uses a single command slot and single PRDT entry if sector_count small.
 *
 * Returns 0 on success, -1 on error.
 */
int ahci_read(uint32_t portnum, uint64_t start_lba, uint32_t sector_count, void* buffer) {
    if(!abar) return -1;
    if(portnum >= 32) return -1;
    HBA_PORT* port = (HBA_PORT*)((uintptr_t)abar + 0x100 + portnum * 0x80);

    // check device presence
    if(!(abar->pi & (1<<portnum))) return -1;

    // find a free command slot
    uint32_t slots = port->sact | port->ci;
    int slot = -1;
    for(int i=0;i<32;i++) {
        if((slots & (1<<i)) == 0) { slot = i; break; }
    }
    if(slot == -1) return -1;

    // command header and table located in CLB
    uint64_t clb = ((uint64_t)port->clb_h << 32) | port->clb;
    HBA_CMD_HEADER* cmdheader = (HBA_CMD_HEADER*)(uintptr_t)clb;
    HBA_CMD_HEADER* header = &cmdheader[slot];
    header->flags = (sizeof(HBA_CMD_TBL)/sizeof(uint32_t)) & 0xFF;
    header->prdtl = 1; // one PRDT

    // allocate cmd table
    uint64_t ctba = ((uint64_t)header->ctba_high << 32) | header->ctba;
    if(ctba == 0) {
        // For simplicity, assume CLB space contiguous and reuse static buffer
        static uint8_t cmdtable_mem[4096];
        memset(cmdtable_mem, 0, sizeof(cmdtable_mem));
        ctba = (uint64_t)(uintptr_t)cmdtable_mem;
        header->ctba = (uint32_t)(ctba & 0xFFFFFFFF);
        header->ctba_high = (uint32_t)((ctba>>32) & 0xFFFFFFFF);
    }
    HBA_CMD_TBL* cmdtbl = (HBA_CMD_TBL*)(uintptr_t)ctba;
    memset((void*)cmdtbl,0,sizeof(HBA_CMD_TBL) + sizeof(HBA_PRDT_ENTRY)* (header->prdtl));

    // PRDT entry
    HBA_PRDT_ENTRY* prdt = &cmdtbl->prdt_entry[0];
    prdt->dba = (uint32_t)((uint64_t)(uintptr_t)buffer & 0xFFFFFFFF);
    prdt->dba_high = (uint32_t)(((uint64_t)(uintptr_t)buffer>>32) & 0xFFFFFFFF);
    prdt->dbc = (sector_count * 512) - 1; // byte count -1
    prdt->dbc |= 1u<<31; // interrupt on completion

    // Build command FIS in cfis
    uint8_t *cfis = cmdtbl->cfis;
    memset(cfis,0,64);
    // Host to device FIS - 0x27
    cfis[0] = 0x27;
    cfis[1] = 0x80; // command
    cfis[2] = 0x25; // ATA READ DMA EXT (0x25) (for LBA48) - might need LBA48 if start_lba > 0xFFFFFFFF
    // Fill LBA and sector count for LBA48:
    cfis[4] = (uint8_t)(start_lba & 0xFF);
    cfis[5] = (uint8_t)((start_lba>>8) & 0xFF);
    cfis[6] = (uint8_t)((start_lba>>16) & 0xFF);
    cfis[7] = 0; // device
    cfis[8] = (uint8_t)((start_lba>>24) & 0xFF);
    cfis[9] = (uint8_t)((start_lba>>32) & 0xFF);
    cfis[10] = (uint8_t)((start_lba>>40) & 0xFF);
    // sector count (low and high)
    uint16_t sc = (uint16_t)sector_count;
    cfis[12] = (uint8_t)(sc & 0xFF);
    cfis[13] = (uint8_t)((sc>>8) & 0xFF);

    // Issue command
    port->ci |= (1<<slot);

    // wait for completion (poll)
    while((port->ci & (1<<slot)) != 0) {
        // check for errors
        if(port->is & (1<<30)) {
            // task file error
            return -1;
        }
    }

    return 0;
}

// simplified write: similar to read but with command code WRITE DMA EXT (0x35)
int ahci_write(uint32_t portnum, uint64_t start_lba, uint32_t sector_count, const void* buffer) {
    if(!abar) return -1;
    if(portnum >= 32) return -1;
    HBA_PORT* port = (HBA_PORT*)((uintptr_t)abar + 0x100 + portnum * 0x80);

    uint32_t slots = port->sact | port->ci;
    int slot = -1;
    for(int i=0;i<32;i++) {
        if((slots & (1<<i)) == 0) { slot = i; break; }
    }
    if(slot == -1) return -1;

    uint64_t clb = ((uint64_t)port->clb_h << 32) | port->clb;
    HBA_CMD_HEADER* cmdheader = (HBA_CMD_HEADER*)(uintptr_t)clb;
    HBA_CMD_HEADER* header = &cmdheader[slot];
    header->flags = (sizeof(HBA_CMD_TBL)/sizeof(uint32_t)) & 0xFF;
    header->prdtl = 1;

    uint64_t ctba = ((uint64_t)header->ctba_high << 32) | header->ctba;
    if(ctba == 0) {
        static uint8_t cmdtable_mem[4096];
        memset(cmdtable_mem, 0, sizeof(cmdtable_mem));
        ctba = (uint64_t)(uintptr_t)cmdtable_mem;
        header->ctba = (uint32_t)(ctba & 0xFFFFFFFF);
        header->ctba_high = (uint32_t)((ctba>>32) & 0xFFFFFFFF);
    }
    HBA_CMD_TBL* cmdtbl = (HBA_CMD_TBL*)(uintptr_t)ctba;
    memset((void*)cmdtbl,0,sizeof(HBA_CMD_TBL) + sizeof(HBA_PRDT_ENTRY)*(header->prdtl));

    HBA_PRDT_ENTRY* prdt = &cmdtbl->prdt_entry[0];
    prdt->dba = (uint32_t)((uint64_t)(uintptr_t)buffer & 0xFFFFFFFF);
    prdt->dba_high = (uint32_t)(((uint64_t)(uintptr_t)buffer>>32) & 0xFFFFFFFF);
    prdt->dbc = (sector_count * 512) - 1;
    prdt->dbc |= 1u<<31;

    uint8_t *cfis = cmdtbl->cfis;
    memset(cfis,0,64);
    cfis[0] = 0x27;
    cfis[1] = 0x80;
    cfis[2] = 0x35; // WRITE DMA EXT
    cfis[4] = (uint8_t)(start_lba & 0xFF);
    cfis[5] = (uint8_t)((start_lba>>8) & 0xFF);
    cfis[6] = (uint8_t)((start_lba>>16) & 0xFF);
    cfis[7] = 0;
    cfis[8] = (uint8_t)((start_lba>>24) & 0xFF);
    cfis[9] = (uint8_t)((start_lba>>32) & 0xFF);
    cfis[10] = (uint8_t)((start_lba>>40) & 0xFF);
    uint16_t sc = (uint16_t)sector_count;
    cfis[12] = (uint8_t)(sc & 0xFF);
    cfis[13] = (uint8_t)((sc>>8) & 0xFF);

    port->ci |= (1<<slot);

    while((port->ci & (1<<slot)) != 0) {
        if(port->is & (1<<30)) return -1;
    }
    return 0;
}
