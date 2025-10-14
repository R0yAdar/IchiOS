#ifndef PCI_H
#define PCI_H

#include "types.h"

#define PCI_CONFIG_ADDR_PORT 0xCF8
#define PCI_CONFIG_DATA_PORT 0xCFC

typedef enum {
    OLD = 0x0,
    MASS_STORAGE = 0x1,
    NETWORK = 0x2,
    DISPLAY = 0x3,
    MULTIMEDIA = 0x4,
    MEMORY = 0x5,
    BRIDGE = 0x6,
    SIMPLE_COM = 0x7,
    BASE_SYSTEM = 0x8,
    INPUT = 0x9,
    DOCKING_STATION = 0xA,
    PROCESSOR = 0xB,
    SERIAL = 0xC,
    WIRELESS = 0xD,
    SMART_IO = 0xE,
    SATELLITE = 0xF,
    ENCRYPTION = 0x10,
    DATA_ACQUISITION = 0x11,
    RESERVED = 0x12,
    UNKNOWN = 0x13
} DEVICE_CLASS_TYPE;

typedef struct {
    uint8_t bus;
    uint8_t device;
    uint8_t function;

    uint16_t vendor_id;
    uint16_t device_id;
    uint8_t class_code;
    uint8_t sub_class;
    uint8_t prog_if;

    uint8_t header_type;
    uint32_t bar[6];
    uint8_t irq;
} pci_device;

pci_device pci_get_device(uint16_t vendor, uint16_t device);

pci_device pci_get_by_class(uint8_t class_code, uint8_t subclass, uint8_t prog_if);

#endif
