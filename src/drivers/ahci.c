#include "pci.h"
#include "ahci.h"
#include "../../core/mem/vmm.h"
#include "print.h"
#include "types.h"

pci_device _device;
sata_device _sata0 = {0};

// Start command engine
void start_cmd(hba_port_t *port)
{
	// Wait until CR (bit15) is cleared
	while (port->cmd & HBA_PxCMD_CR)
		;

	// Set FRE (bit4) and ST (bit0)
	port->cmd |= HBA_PxCMD_FRE;
	port->cmd |= HBA_PxCMD_ST; 
}

// Stop command engine
void stop_cmd(hba_port_t *port)
{
	// Clear ST (bit0)
	port->cmd &= ~HBA_PxCMD_ST;

	// Clear FRE (bit4)
	port->cmd &= ~HBA_PxCMD_FRE;

	// Wait until FR (bit14), CR (bit15) are cleared
	while(1)
	{
		if (port->cmd & HBA_PxCMD_FR)
			continue;
		if (port->cmd & HBA_PxCMD_CR)
			continue;
		break;
	}

}

#define FIS_BUFFER_SIZE 256
#define CMD_LIST_BUFFER_SIZE 0x1000
#define CMD_TABLE_ENTRY_SIZE 32
#define CMD_TABLE_ENTRY_COUNT 8


void port_rebase(hba_port_t *port)
{
	void* paddr;
	void* vaddr = kpage_alloc_dma(3, &paddr);
	
	void* pclb = paddr;
	void* vclb = vaddr;

	void* pfb = (void*)((uint64_t)paddr + CMD_LIST_BUFFER_SIZE);
	void* vfb = (void*)((uint64_t)vaddr + CMD_LIST_BUFFER_SIZE);

	void* pctba_top = (void*)((uint64_t)paddr + CMD_LIST_BUFFER_SIZE + FIS_BUFFER_SIZE);
	void* vctba_top = (void*)((uint64_t)vaddr + CMD_LIST_BUFFER_SIZE + FIS_BUFFER_SIZE);

	stop_cmd(port);

	port->clb = (uint32_t)pclb;
	port->clbu = (uint32_t)((uint64_t)pclb >> 32);

	port->fb = (uint32_t)pfb;
	port->fbu = (uint32_t)((uint64_t)pfb >> 32);

	hba_cmd_header_t* cmdheader = (hba_cmd_header_t*)(vclb);
	
	for (int i=0; i < AHCI_COMMAND_SLOTS_AMOUNT; i++)
	{
		uint64_t address = (uint64_t)pctba_top + (CMD_TABLE_ENTRY_COUNT * CMD_TABLE_ENTRY_SIZE) * i;
		cmdheader[i].prdtl = CMD_TABLE_ENTRY_COUNT;
		cmdheader[i].ctba = (uint32_t)address;
		cmdheader[i].ctbau = (uint32_t)(address >> 32);
	}

	_sata0.clb = vclb;
	_sata0.fb = vfb;
	_sata0.ctba_start = vctba_top;
	_sata0.port = *port;

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
	println("Registring SATA device");
	port_rebase(port);
}

void probe_port(hba_mem_t *abar)
{
	// Search disk in implemented ports
	uint32_t pi = abar->pi;
	int i = 0;
	while (i<32)
	{
		if (pi & 1)
		{
			int dt = check_type(&abar->ports[i]);
			if (dt == AHCI_DEV_SATA)
			{
				println("SATA drive found");
                register_drive(&abar->ports[i]);
			}
			else if (dt == AHCI_DEV_SATAPI)
			{
				println("SATAPI drive found");
			}
			else if (dt == AHCI_DEV_SEMB)
			{
				println("SEMB drive found");
			}
			else if (dt == AHCI_DEV_PM)
			{
				println("PM drive found");
			}
			else
			{
				//println("No drive found");
			}
		}

		pi >>= 1;
		i ++;
	}
}

// Check device type
int check_type(hba_port_t *port)
{
	uint32_t ssts = port->ssts;

	uint8_t ipm = (ssts >> 8) & 0x0F;
	uint8_t det = ssts & 0x0F;

	if (det != HBA_PORT_DET_PRESENT)	// Check drive status
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

#define SECTOR_SIZE 512
#define MAX_PRDT_BYTE_COUNT (4 * 1024 * 1024) // 4MB per PRDT entry

BOOL ahci_read(void* lba_ptr, uint32_t sector_count, void* phys_buffer)
{
    if (sector_count == 0) return FALSE;

    volatile hba_port_t *port = &_sata0.port;

    // 1) find a free slot
    int slot = find_cmdslot(port);
    if (slot == -1) {
        println("AHCI: no free command slot");
        return FALSE;
    }

    // 2) pointers to header and command table (virtual)
    volatile hba_cmd_header_t *cmdheader = (hba_cmd_header_t*)_sata0.clb;
    volatile hba_cmd_tbl_t *cmdtbl = (hba_cmd_tbl_t*)((uint64_t)_sata0.ctba_start + (slot * (CMD_TABLE_ENTRY_SIZE * CMD_TABLE_ENTRY_COUNT)));

    // Clear command table memory (64+16+48+PRDT area)
    // 256 bytes total per table
    for (int i = 0; i < 256/4; ++i) {
        ((uint32_t*)cmdtbl)[i] = 0;
    }

    // 3) fill command header
    // cfl = command FIS length in DWORDS (5 dwords = 20 bytes)
    cmdheader[slot].cfl = (uint8_t)(sizeof(fis_reg_h2d_t) / 4); // 20/4 = 5
    cmdheader[slot].w   = 0;    // 0 = device to host (read)
    cmdheader[slot].prdtl = 0;  // we'll set actual PRDT entries count below
    cmdheader[slot].prdbc = 0;  // cleared

    // ensure PRDTL will fit; we'll update it after building PRDT
    // ctba/ctbau were set during port_rebase; no need to set here

    // 4) prepare CFIS (host -> device command)
    volatile fis_reg_h2d_t *cfis = (fis_reg_h2d_t*)(_sata0.fb);
    // Zeroing already done by clearing cmdtbl above; set fields:
    cfis->fis_type = FIS_TYPE_REG_H2D;
    cfis->c = 1; // Command
    cfis->pmport = 0;
    cfis->command = ATA_CMD_READ_DMA_EX; // 0xC8

    // LBA: allow 48-bit LBA
    uint64_t lba = (uint64_t) (uint64_t)lba_ptr;
    cfis->lba0 = (uint8_t)(lba & 0xFF);
    cfis->lba1 = (uint8_t)((lba >> 8) & 0xFF);
    cfis->lba2 = (uint8_t)((lba >> 16) & 0xFF);
    cfis->device = 1 << 6; // set LBA bit (bit6)
    cfis->lba3 = (uint8_t)((lba >> 24) & 0xFF);
    cfis->lba4 = (uint8_t)((lba >> 32) & 0xFF);
    cfis->lba5 = (uint8_t)((lba >> 40) & 0xFF);

    // Sector count (16-bit)
    cfis->countl = (uint8_t)(sector_count & 0xFF);
    cfis->counth = (uint8_t)((sector_count >> 8) & 0xFF);

    // 5) build PRDT entries to point the controller to phys_buffer
    uint64_t bytes_remaining = (uint64_t)sector_count * SECTOR_SIZE;
    uint64_t buffer_pa = (uint64_t)phys_buffer;

    int prdt_index = 0;
    while (bytes_remaining > 0 && prdt_index < CMD_TABLE_ENTRY_COUNT) {
        uint64_t bytes_this = bytes_remaining;
        if (bytes_this > MAX_PRDT_BYTE_COUNT) bytes_this = MAX_PRDT_BYTE_COUNT;

        // Fill PRDT entry
        cmdtbl->prdt_entry[prdt_index].dba = (uint32_t)(buffer_pa & 0xFFFFFFFF);
        cmdtbl->prdt_entry[prdt_index].dbau = (uint32_t)(buffer_pa >> 32);
        cmdtbl->prdt_entry[prdt_index].rsv0 = 0;

        // dbc is byte count - 1 in bits 0..21 of that DW (we have 22 bits)
        uint32_t dbc_field = (uint32_t)(bytes_this - 1) & 0x3FFFFF;
        // set interrupt on completion for last PRDT entry (i=1)
        cmdtbl->prdt_entry[prdt_index].dbc = dbc_field;
        cmdtbl->prdt_entry[prdt_index].i = 0; // clear for now

        buffer_pa += bytes_this;
        bytes_remaining -= bytes_this;
        prdt_index++;
    }

    if (bytes_remaining != 0) {
        // Not enough PRDT entries available
        println("AHCI: transfer too large for PRDT entries");
        return FALSE;
    }

    // Mark interrupt-on-completion on last PRDT
    if (prdt_index > 0) {
        cmdtbl->prdt_entry[prdt_index - 1].i = 1;
    }

    // Update header with PRDT count
    cmdheader[slot].prdtl = prdt_index;

    // 6) Clear port interrupt status
    port->is = (uint32_t)-1;

    // 7) Issue command by setting CI bit for the slot
    port->ci = (1u << slot);

    // 8) Wait for completion
    // Wait until slot cleared in CI
    while (1) {
        // Check for taskfile error from port interrupt status
        if (port->is & HBA_PxIS_TFES) {
            println("AHCI: taskfile error (TFES)");
            // Clear error status
            port->is = HBA_PxIS_TFES;
            return FALSE;
        }

        if ((port->ci & (1u << slot)) == 0)
            break;
    }

    // 9) Check TFD (task file) for errors or busy
    // Ensure device is not busy and has cleared DRQ
    while ( (port->tfd & (ATA_DEV_BUSY | ATA_DEV_DRQ)) ) {
        // optional: add timeout here to avoid infinite loop
        if (port->is & HBA_PxIS_TFES) {
            println("AHCI: device error after completion");
            port->is = HBA_PxIS_TFES;
            return FALSE;
        }
    }

    // 10) Success — buffer now contains read data
    return TRUE;
}


// Find a free command list slot
int find_cmdslot(hba_port_t *port)
{
	// If not set in SACT and CI, the slot is free
	uint32_t slots = (port->sact | port->ci);
	for (int i=0; i< 32; i++)
	{
		if ((slots&1) == 0)
			return i;
		slots >>= 1;
	}
	return -1;
}

BOOL ahci_init() {
    print("Size of hba: ");printi(sizeof(hba_mem_t)); print("  "); printiln(sizeof(hba_port_t));
    pci_device partial = pci_get_by_class(MASS_STORAGE, PCI_SATA_SUBCLASS, PCI_AHCI_PROGIF);
    _device = pci_get_device(partial.vendor_id, partial.device_id);
	print("IRQ line is"); printiln(_device.irq);
	
	pci_enable_mmio(&_device);
	pci_enable_mastering(&_device);
	pci_enable_interrupts(&_device);

	if (_device.device_id == 0) return FALSE;
    uint32_t mmio_base_address = _device.bar[5] & (~0xF);

    volatile hba_mem_t* hba = (volatile hba_mem_t*)map_mmio_region((void*)mmio_base_address, (void*)((uint64_t)mmio_base_address + sizeof(hba_mem_t)));
    
    set_ahci_mode(hba);

	if (is_64bit(hba)) {
		println("ACHI supports 64bit!");
	}

    probe_port(hba);

	//read(0, 0, )
}