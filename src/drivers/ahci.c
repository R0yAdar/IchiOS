#include "pci.h"
#include "ahci.h"
#include "vmm.h"
#include "types.h"
#include "cstring.h"
#include "serial.h"

#define FIS_BUFFER_SIZE 256
#define CMD_LIST_BUFFER_SIZE 0x1000
#define CMD_TABLE_ENTRY_SIZE 32
#define CMD_TABLE_ENTRY_COUNT 8
#define COMMAND_TABLE_SIZE (CMD_TABLE_ENTRY_SIZE * (CMD_TABLE_ENTRY_COUNT - 1) + sizeof(hba_cmd_tbl_t) + 16) // (alignment)
#define SECTOR_SIZE 512
#define MAX_PRDT_BYTE_COUNT (4 * 1024 * 1024) // 4MB per PRDT entry

pci_device _device;
sata_device _sata0 = {0};

void start_cmd(hba_port_t *port)
{
	while (port->cmd & HBA_PxCMD_CR);

	port->cmd |= HBA_PxCMD_FRE;
	port->cmd |= HBA_PxCMD_ST; 
}

void stop_cmd(hba_port_t *port)
{
	port->cmd &= ~HBA_PxCMD_ST;
	port->cmd &= ~HBA_PxCMD_FRE;

	while(1)
	{
		if (port->cmd & HBA_PxCMD_FR)
			continue;
		if (port->cmd & HBA_PxCMD_CR)
			continue;
		break;
	}

}

void port_rebase(hba_port_t *port)
{
	void* paddr;
	void* vaddr = kpage_alloc_dma(4, &paddr);
	
	void* pclb = paddr;
	void* vclb = vaddr;

	void* pfb = (void*)((uint64_t)paddr + CMD_LIST_BUFFER_SIZE);
	void* vfb = (void*)((uint64_t)vaddr + CMD_LIST_BUFFER_SIZE);

	void* pctba_top = (void*)((uint64_t)paddr + CMD_LIST_BUFFER_SIZE + FIS_BUFFER_SIZE);
	void* vctba_top = (void*)((uint64_t)vaddr + CMD_LIST_BUFFER_SIZE + FIS_BUFFER_SIZE);

	stop_cmd(port);

	port->clb = (uint32_t)((uint64_t)pclb & 0xFFFFFFFF);
	port->clbu = (uint32_t)((uint64_t)pclb >> 32);

	port->fb = (uint32_t)((uint64_t)pfb & 0xFFFFFFFF);
	port->fbu = (uint32_t)((uint64_t)pfb >> 32);

	hba_cmd_header_t* cmdheader = (hba_cmd_header_t*)(vclb);
	
	for (int i=0; i < AHCI_COMMAND_SLOTS_AMOUNT; i++)
	{
		uint64_t address = (uint64_t)pctba_top + (COMMAND_TABLE_SIZE) * i;
		cmdheader[i].prdtl = CMD_TABLE_ENTRY_COUNT;
		cmdheader[i].ctba = (uint32_t)address;
		cmdheader[i].ctbau = (uint32_t)(address >> 32);
	}

	_sata0.clb = vclb;
	_sata0.fb = vfb;
	_sata0.ctba_start = vctba_top;
	_sata0.port = port;

	start_cmd(port);
}

void set_ahci_mode(volatile hba_mem_t* hba) {
    hba->ghc |= (1U << 31);
}

BOOL is_64bit(volatile hba_mem_t* hba) {
    uint64_t cap = hba->cap;
	return (cap & (1 << 31)) > 0 ? TRUE : FALSE;
}

void register_drive(hba_port_t* port) {
	qemu_log("Registring SATA device");
	port_rebase(port);
}

int check_type(hba_port_t *port)
{
	uint32_t ssts = port->ssts;

	uint8_t ipm = (ssts >> 8) & 0x0F;
	uint8_t det = ssts & 0x0F;

	if (det != HBA_PORT_DET_PRESENT)
		return AHCI_DEV_NULL;
	if (ipm != HBA_PORT_IPM_ACTIVE)
		return AHCI_DEV_NULL;

	switch (port->sig)
	{
	case SATA_SIG_ATAPI:
		return AHCI_DEV_SATAPI;
	case SATA_SIG_SEMB:
		return AHCI_DEV_SEMB;
	case SATA_SIG_PM:
		return AHCI_DEV_PM;
	default:
		return AHCI_DEV_SATA;
	}
}

void probe_port(hba_mem_t *abar)
{
	uint32_t pi = abar->pi;
	int i = 0;

	while (i < 32)
	{
		if (pi & 1)
		{
			int dt = check_type(&abar->ports[i]);
			if (dt == AHCI_DEV_SATA)
			{
				qemu_log("SATA drive found... registering...");
                register_drive(&abar->ports[i]);
			}
			else if (dt == AHCI_DEV_SATAPI)
			{
				qemu_log("SATAPI drive found");
			}
			else if (dt == AHCI_DEV_SEMB)
			{
				qemu_log("SEMB drive found");
			}
			else if (dt == AHCI_DEV_PM)
			{
				qemu_log("PM drive found");
			}
		}

		pi >>= 1;
		i ++;
	}
}

int find_cmdslot(hba_port_t *port)
{
	uint32_t slots = (port->sact | port->ci);
	for (int i=0; i< 32; i++)
	{
		if ((slots&1) == 0)
			return i;
		slots >>= 1;
	}
	return -1;
}

BOOL ahci_action(uint64_t lba, uint32_t sector_count, void* phys_buffer, uint8_t command)
{
    if (sector_count == 0) return FALSE;

    volatile hba_port_t *port = _sata0.port;

    int slot = find_cmdslot(port);

    if (slot == -1) {
        qemu_log("AHCI: no free command slot");
        return FALSE;
    }

    volatile hba_cmd_header_t *cmdheader = (hba_cmd_header_t*)(_sata0.clb);

	cmdheader[slot].cfl = (uint8_t)(sizeof(fis_reg_h2d_t) / DWORD_SIZE);
    
	if (command == ATA_CMD_WRITE_DMA_EX) {
		cmdheader[slot].w = 1;
	} 
	else
	{
		cmdheader[slot].w = 0;
	}

	volatile hba_cmd_tbl_t *cmdtbl = (hba_cmd_tbl_t*)((uint64_t)_sata0.ctba_start + (slot * COMMAND_TABLE_SIZE));

	memset((void*)cmdtbl, 0, COMMAND_TABLE_SIZE);

	uint64_t buffer_pa = (uint64_t)phys_buffer;

	uint64_t sectors_per_prdt_entry = sector_count / (CMD_TABLE_ENTRY_COUNT - 1);
	uint64_t remaining_sectors = sector_count % (CMD_TABLE_ENTRY_COUNT - 1);

	int prdt_index = 0;

	for (; prdt_index < CMD_TABLE_ENTRY_COUNT - 1 && sectors_per_prdt_entry > 0; prdt_index++)
	{
		cmdtbl->prdt_entry[prdt_index].dba = (uint32_t)(buffer_pa & 0xFFFFFFFF);
        cmdtbl->prdt_entry[prdt_index].dbau = (uint32_t)(buffer_pa >> 32);
		cmdtbl->prdt_entry[prdt_index].dbc = sectors_per_prdt_entry * SECTOR_SIZE - 1;
		cmdtbl->prdt_entry[prdt_index].i = 1;
		buffer_pa += sectors_per_prdt_entry * SECTOR_SIZE;
    }
	if (remaining_sectors > 0) {
		cmdtbl->prdt_entry[prdt_index].dba = (uint32_t)(buffer_pa & 0xFFFFFFFF);
		cmdtbl->prdt_entry[prdt_index].dbau = (uint32_t)(buffer_pa >> 32);
		cmdtbl->prdt_entry[prdt_index].dbc = remaining_sectors * SECTOR_SIZE - 1;
		cmdtbl->prdt_entry[prdt_index].i = 1;
		++prdt_index;
	}

	cmdheader->prdtl = prdt_index;

    volatile fis_reg_h2d_t *cfis = (fis_reg_h2d_t*)(&cmdtbl->cfis);

    cfis->fis_type = FIS_TYPE_REG_H2D;
    cfis->c = 1;

    cfis->command = command;

    cfis->lba0 = (uint8_t)(lba & 0xFF);
    cfis->lba1 = (uint8_t)((lba >> 8) & 0xFF);
    cfis->lba2 = (uint8_t)((lba >> 16) & 0xFF);
    cfis->device = 1 << 6; // lba bit
    cfis->lba3 = (uint8_t)((lba >> 24) & 0xFF);
    cfis->lba4 = (uint8_t)((lba >> 32) & 0xFF);
    cfis->lba5 = (uint8_t)((lba >> 40) & 0xFF);

    cfis->countl = (uint8_t)(sector_count & 0xFF);
    cfis->counth = (uint8_t)((sector_count >> 8) & 0xFF);

    port->is = (uint32_t)-1;

    port->ci = (1u << slot);

    while (1) {
        if (port->is & HBA_PxIS_TFES) {
            qemu_log("AHCI: taskfile error (TFES)!");
            port->is = HBA_PxIS_TFES;
            return FALSE;
        }

        if ((port->ci & (1u << slot)) == 0)
            break;
    }

    if (port->is & HBA_PxIS_TFES)
	{
		qemu_log("AHCI: taskfile error (TFES)");
		return FALSE;
	}
	
    return TRUE;
}

BOOL ahci_read(uint64_t lba, uint32_t sector_count, void* phys_buffer)
{
    return ahci_action(lba, sector_count, phys_buffer, ATA_CMD_READ_DMA_EX);
}

BOOL ahci_write(uint64_t lba, uint32_t sector_count, void* phys_buffer)
{
    return ahci_action(lba, sector_count, phys_buffer, ATA_CMD_WRITE_DMA_EX);
}

BOOL ahci_init() {
    pci_device partial = pci_get_by_class(MASS_STORAGE, PCI_SATA_SUBCLASS, PCI_AHCI_PROGIF);
    _device = pci_get_device(partial.vendor_id, partial.device_id);
	
	pci_enable_mmio(&_device);
	pci_enable_mastering(&_device);
	pci_enable_interrupts(&_device);

	if (_device.device_id == 0) return FALSE;
    uint32_t mmio_base_address = _device.bar[5] & (~0xF);

    volatile hba_mem_t* hba = (volatile hba_mem_t*)map_mmio_region(
		(void*)((uint64_t)mmio_base_address), 
		(void*)((uint64_t)mmio_base_address + sizeof(hba_mem_t))
	);
    
    set_ahci_mode(hba);

	if (!is_64bit(hba)) {
		qemu_log("ACHI doesn't support 64bit!");
		return FALSE;
	}

    probe_port(hba);

	return TRUE;
}