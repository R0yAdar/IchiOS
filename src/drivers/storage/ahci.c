#include "pci.h"
#include "storage/ahci.h"
#include "../../core/mem/vmm.h"
#include "print.h"
#include "types.h"

pci_device _device;

#pragma pack(push, 1)

typedef struct {
    uint32_t cap;
    uint32_t ghc;
    uint32_t is;
    uint32_t pi;
    uint32_t vs;
    uint32_t ccc_ctl;
    uint32_t ccc_ports;
    uint32_t em_loc;
    uint32_t em_ctl;
    uint32_t cap2;
    uint32_t bohc;
} hba_mem_t;

#pragma pack(pop)


BOOL ahci_init() {
    pci_device partial = pci_get_by_class(0x01, 0x06, 0x01);
    _device = pci_get_device(partial.vendor_id, partial.device_id);


	if (_device.device_id == 0) return FALSE;

    uint32_t mmio_base_address = _device.bar[5] & (~0xF);

    volatile hba_mem_t* hba = (volatile hba_mem_t*)map_non_cacheable_page((void*)mmio_base_address);
    print("Address: ");
    printxln((void*)hba);
    println("WTF:");

    hba->ghc |= (1 << 31);
    uint32_t ports_implemented = hba->pi;

    printxln(ports_implemented);
}