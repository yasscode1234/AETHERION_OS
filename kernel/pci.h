#ifndef PCI_H
#define PCI_H

#include "types.h"

#define PCI_CONFIG_ADDRESS 0xCF8
#define PCI_CONFIG_DATA    0xCFC

typedef struct {
    uint8_t bus;
    uint8_t device;
    uint8_t function;
    uint16_t vendor_id;
    uint16_t device_id;
    uint8_t class_code;
    uint8_t subclass;
    uint8_t prog_if;
    uint32_t bar[6];
} pci_device_t;

/**
 * pci_config_read32(bus, device, function, offset)
 * pci_config_write32(...)
 *  - accès au config space PCI via ports 0xCF8/0xCFC
 */
uint32_t pci_config_read32(uint8_t bus, uint8_t device, uint8_t function, uint8_t offset);
void     pci_config_write32(uint8_t bus, uint8_t device, uint8_t function, uint8_t offset, uint32_t value);

/**
 * Scan PCI bus entier et appelle callback pour chaque device détecté.
 * callback reçoit pointeur vers pci_device_t rempli.
 */
typedef void (*pci_device_callback_t)(const pci_device_t* dev);
void pci_scan_bus(pci_device_callback_t cb);

/**
 * Trouve le i-ème device avec class/subclass/prog_if.
 * Retourne 0 si trouvé et remplit dev_out, -1 sinon.
 */
int pci_find_device(uint8_t class_code, uint8_t subclass, uint8_t prog_if, int index, pci_device_t* dev_out);

/**
 * Récupère la BAR phys (32/64) pour un device.
 * bar_index 0..5
 * Retourne 0 si ok (et remplit *bar) ou -1 si erreur.
 */
int pci_get_bar(const pci_device_t* dev, int bar_index, uint64_t* bar);

#endif
