#include "pci.h"
#include "assembly.h"

#define PCI_COMMAND_OFFSET 0x04

uint32_t pci_create_address(uint8_t bus, uint8_t device, uint8_t function, uint8_t reg_offset)
{
    return (((uint32_t)1) << 31) | ((uint32_t)bus << 16) | (((uint32_t)device & 0x1F) << 11) | (((uint32_t)function & 0x7) << 8) | ((uint32_t)reg_offset & 0xFC);
}

uint32_t pci_read(uint8_t bus, uint8_t device, uint8_t function, uint8_t offset)
{
    uint32_t address = pci_create_address(bus, device, function, offset);
    port_outl(PCI_CONFIG_ADDR_PORT, address);
    return port_inl(PCI_CONFIG_DATA_PORT);
}

void pci_write(uint8_t bus, uint8_t device, uint8_t function, uint8_t offset, uint32_t value)
{
    uint32_t address = pci_create_address(bus, device, function, offset);
    port_outl(PCI_CONFIG_ADDR_PORT, address);
    port_outl(PCI_CONFIG_DATA_PORT, value);
}

pci_device pci_populate_device(uint8_t bus, uint8_t dev, uint8_t func)
{
    uint32_t id = pci_read(bus, dev, func, 0x00);

    uint16_t read_vendor = id & 0xFFFF;
    uint16_t read_device = (id >> 16) & 0xFFFF;

    pci_device pci_info = {0};

    pci_info.bus = bus;
    pci_info.device = dev;
    pci_info.function = func;
    pci_info.vendor_id = read_vendor;
    pci_info.device_id = read_device;

    uint32_t class_info = pci_read(bus, dev, func, 0x08);
    pci_info.class_code = (class_info >> 24) & 0xFF;
    pci_info.sub_class = (class_info >> 16) & 0xFF;
    pci_info.prog_if = (class_info >> 8) & 0xFF;

    uint32_t header = pci_read(bus, dev, func, 0x0C);
    pci_info.header_type = (header >> 16) & 0xFF;

    for (int i = 0; i < 6; i++)
    {
        pci_info.bar[i] = pci_read(bus, dev, func, 0x10 + i * 4);
    }

    uint32_t irq_info = pci_read(bus, dev, func, 0x3C);
    pci_info.irq = irq_info & 0xFF;

    return pci_info;
}

pci_device pci_get_device(uint16_t vendor, uint16_t device_id)
{
    for (uint16_t bus = 0; bus < 256; bus++)
    {
        for (uint8_t dev = 0; dev < 32; dev++)
        {
            for (uint8_t func = 0; func < 8; func++)
            {

                uint32_t id = pci_read(bus, dev, func, 0x00);
                uint16_t read_vendor = id & 0xFFFF;
                if (read_vendor == 0xFFFF)
                    continue;

                uint16_t read_device = (id >> 16) & 0xFFFF;
                if (read_vendor == vendor && read_device == device_id)
                {
                    return pci_populate_device(bus, dev, func);
                }
            }
        }
    }

    return (pci_device){.vendor_id = 0xFFFF};
}

pci_device pci_get_by_class(uint8_t class_code, uint8_t subclass, uint8_t prog_if)
{
    for (uint32_t bus = 0; bus < 256; bus++)
    {
        for (uint8_t dev = 0; dev < 32; dev++)
        {
            for (uint8_t func = 0; func < 8; func++)
            {
                uint32_t id = pci_read(bus, dev, func, 0x00);
                if ((id & 0xFFFF) == 0xFFFF)
                    continue;

                uint32_t class_info = pci_read(bus, dev, func, 0x08);
                uint8_t cls = (class_info >> 24) & 0xFF;
                uint8_t sub = (class_info >> 16) & 0xFF;
                uint8_t prog = (class_info >> 8) & 0xFF;

                if (cls == class_code && sub == subclass && prog == prog_if)
                {
                    return pci_populate_device(bus, dev, func);
                }
            }
        }
    }

    return (pci_device){.vendor_id = 0xFFFF};
}

void pci_enable_mmio(pci_device *desc)
{
    uint32_t cmd = pci_read(desc->bus, desc->device, desc->function, PCI_COMMAND_OFFSET);
    cmd |= PCI_COMMAND_MEMORY;
    pci_write(desc->bus, desc->device, desc->function, PCI_COMMAND_OFFSET, cmd);
}

void pci_enable_mastering(pci_device *desc)
{
    uint32_t cmd = pci_read(desc->bus, desc->device, desc->function, PCI_COMMAND_OFFSET);
    cmd |= PCI_COMMAND_MASTER;
    pci_write(desc->bus, desc->device, desc->function, PCI_COMMAND_OFFSET, cmd);
}

void pci_enable_interrupts(pci_device *desc)
{
    uint32_t cmd = pci_read(desc->bus, desc->device, desc->function, PCI_COMMAND_OFFSET);
    cmd &= ~PCI_COMMAND_INT_DISABLE;
    pci_write(desc->bus, desc->device, desc->function, PCI_COMMAND_OFFSET, cmd);
}
