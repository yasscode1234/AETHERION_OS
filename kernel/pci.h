#ifndef AETHERION_PCI_H
#define AETHERION_PCI_H

#include "kernel.h"

/* ---- PCI Configuration Space ---- */
#define PCI_CONFIG_ADDRESS  0xCF8
#define PCI_CONFIG_DATA     0xCFC

/* ---- PCI Class Codes ---- */
#define PCI_CLASS_UNCLASSIFIED      0x00
#define PCI_CLASS_MASS_STORAGE      0x01
#define PCI_CLASS_NETWORK           0x02
#define PCI_CLASS_DISPLAY           0x03
#define PCI_CLASS_MULTIMEDIA        0x04
#define PCI_CLASS_MEMORY            0x05
#define PCI_CLASS_BRIDGE            0x06
#define PCI_CLASS_COMMUNICATION     0x07
#define PCI_CLASS_BASE_PERIPHERAL   0x08
#define PCI_CLASS_INPUT             0x09
#define PCI_CLASS_DOCKING           0x0A
#define PCI_CLASS_PROCESSOR         0x0B
#define PCI_CLASS_SERIAL_BUS        0x0C
#define PCI_CLASS_WIRELESS          0x0D
#define PCI_CLASS_INTELLIGENT_IO    0x0E
#define PCI_CLASS_SATELLITE         0x0F
#define PCI_CLASS_ENCRYPTION        0x10
#define PCI_CLASS_SIGNAL_PROCESSING 0x11

/* ---- PCI Subclass Codes (Mass Storage) ---- */
#define PCI_SUBCLASS_SCSI           0x00
#define PCI_SUBCLASS_IDE            0x01
#define PCI_SUBCLASS_FLOPPY         0x02
#define PCI_SUBCLASS_IPI            0x03
#define PCI_SUBCLASS_RAID           0x04
#define PCI_SUBCLASS_ATA            0x05
#define PCI_SUBCLASS_SATA           0x06
#define PCI_SUBCLASS_SAS            0x07
#define PCI_SUBCLASS_NVME           0x08

/* ---- PCI Programming Interface (SATA) ---- */
#define PCI_PROG_IF_AHCI            0x01
#define PCI_PROG_IF_NVME            0x02

/* ---- PCI Header Type ---- */
#define PCI_HEADER_TYPE_NORMAL      0x00
#define PCI_HEADER_TYPE_BRIDGE      0x01
#define PCI_HEADER_TYPE_CARDBUS     0x02

/* ---- PCI Capability IDs ---- */
#define PCI_CAP_PM                  0x01  /* Power Management */
#define PCI_CAP_AGP                 0x02  /* AGP */
#define PCI_CAP_VPD                 0x03  /* Vital Product Data */
#define PCI_CAP_SLOTID              0x04  /* Slot Identification */
#define PCI_CAP_MSI                 0x05  /* Message Signaled Interrupts */
#define PCI_CAP_HOTPLUG             0x06  /* CompactPCI HotSwap */
#define PCI_CAP_PCIX                0x07  /* PCI-X */
#define PCI_CAP_EXPRESS             0x10  /* PCI Express */
#define PCI_CAP_MSIX                0x11  /* MSI-X */

/* ---- PCI Device Structure ---- */
typedef struct {
    u16 vendor_id;
    u16 device_id;
    u16 command;
    u16 status;
    u8  revision_id;
    u8  prog_if;
    u8  subclass;
    u8  class_code;
    u8  cache_line_size;
    u8  latency_timer;
    u8  header_type;
    u8  bist;
    u32 bar[6];
    u32 cardbus_cis;
    u16 subsystem_vendor_id;
    u16 subsystem_id;
    u32 expansion_rom;
    u8  capabilities_ptr;
    u8  reserved[3];
    u32 reserved2;
    u8  interrupt_line;
    u8  interrupt_pin;
    u8  min_grant;
    u8  max_latency;
    u8  bus;
    u8  device;
    u8  function;
} pci_device_t;

/* ---- PCI BAR Types ---- */
#define PCI_BAR_TYPE_MMIO   0x00
#define PCI_BAR_TYPE_IO     0x01
#define PCI_BAR_TYPE_64BIT  0x04
#define PCI_BAR_PREFETCH    0x08

/* ---- API PCI ---- */
void pci_init(void);
int pci_scan_bus(u8 bus);
pci_device_t* pci_find_device(u16 vendor_id, u16 device_id);
pci_device_t* pci_find_class(u8 class_code, u8 subclass, u8 prog_if);
pci_device_t* pci_get_device(u8 bus, u8 device, u8 function);
u32 pci_read_config(pci_device_t* dev, u8 reg);
void pci_write_config(pci_device_t* dev, u8 reg, u32 value);
void pci_enable_device(pci_device_t* dev);
void pci_enable_bus_master(pci_device_t* dev);
u64 pci_get_bar_address(pci_device_t* dev, int bar_index);
int pci_get_bar_size(pci_device_t* dev, int bar_index);
void pci_list_devices(void);
const char* pci_class_name(u8 class_code, u8 subclass);

#endif /* AETHERION_PCI_H */
