#include "vmm.h"
#include "pmm.h"
#include "pte.h"
#include "cstring.h"
#include "assembly.h"
#include "serial.h"

/// VMM INTERNALS

typedef uint16_t ptable_index_t;

typedef enum
{
    PAGE_MAPPING_DEFAULT = 0,
    PAGE_MAPPING_USERSPACE = 1,
    PAGE_MAPPING_HUGE_PAGE = 2,
    PAGE_MAPPING_NON_CACHEABLE = 4,
} PAGE_MAPPING_OPTIONS;

typedef enum
{
    PTABLE_LVL_PML4 = 4,
    PTABLE_LVL_PDPT = 3,
    PTABLE_LVL_PDE = 2,
    PTABLE_LVL_PTE = 1
} PTABLE_LVL;

struct pagetable_context
{
    pte_t *root_table;
};

typedef struct
{
    BOOL is_remapped;
    uint16_t direct_mapping_size_gb;
    uint16_t kernel_size_pages;
} vmm_globals;

/// VMM GLOBALS

pagetable_context _km_vmm_ctx;

vmm_globals _km_vmm_globals = {
    .is_remapped = FALSE,
    .direct_mapping_size_gb = 8,
    .kernel_size_pages = 512};

/// VMM Utilities

ptable_index_t offset_to_index(void *address, PTABLE_LVL lvl)
{
    switch (lvl)
    {
    case PTABLE_LVL_PML4:
        return (((uint64_t)address >> (12 + 9 * 3)) & 0x1ff);
    case PTABLE_LVL_PDPT:
        return (((uint64_t)address >> (12 + 9 * 2)) & 0x1ff);
    case PTABLE_LVL_PDE:
        return (((uint64_t)address >> (12 + 9)) & 0x1ff);
    case PTABLE_LVL_PTE:
        return (((uint64_t)address >> 12) & 0x1ff);
    }

    return (ptable_index_t)-1;
}

void *vmm_get_vaddr(void *phys)
{
    return (void *)((uint64_t)phys | ((_km_vmm_globals.is_remapped) ? (VMM_RAM_DIRECT_MAPPING_OFFSET) : 0));
}

void *canonify(void *address)
{
    return ((uint64_t)address & (1ull << 47)) ? (void *)((0xffffull << 48) | (uint64_t)address) : (void *)((~(0xffffull << 48)) & (uint64_t)address);
}

/// PTE

pte_t *pte_get_index(pte_t *table, ptable_index_t index)
{
    if (index >= VMM_PTABLE_ENTRY_COUNT)
        return NULL;

    return (pte_t *)vmm_get_vaddr(table + index);
}

void pte_put(pte_t *table, ptable_index_t index, pte_t value)
{
    if (index >= VMM_PTABLE_ENTRY_COUNT)
        return;

    *((pte_t *)vmm_get_vaddr(table + index)) = value;
}

void pte_apply_options_to_entry(pte_t *entry, PAGE_MAPPING_OPTIONS options)
{
    if (options & PAGE_MAPPING_USERSPACE)
    {
        pte_mark_user_space(entry);
    }
    if (options & PAGE_MAPPING_HUGE_PAGE)
    {
        pte_mark_huge_page(entry);
    }
    if (options & PAGE_MAPPING_NON_CACHEABLE)
    {
        pte_mark_non_cacheable(entry);
    }
}

pte_t pte_create_entry(void *phys_address, PAGE_MAPPING_OPTIONS options)
{
    pte_t entry = DEFAULT_PTE;
    pte_apply_options_to_entry(&entry, options);
    pte_assign_address(&entry, phys_address);
    return entry;
}

/// PT

pte_t *pt_allocate_into(pte_t *table_index_p, PAGE_MAPPING_OPTIONS options)
{
    void *phys = pmm_alloc();
    if (!phys)
        return NULL;

    pte_t *ptable = (pte_t *)((vmm_get_vaddr(phys)));
    memset((void *)ptable, 0, PAGE_SIZE);

    (*table_index_p) = pte_create_entry(phys, options);

    return ptable;
}

pte_t *pt_get_or_allocate_into(pte_t *table_index_p, PAGE_MAPPING_OPTIONS options)
{
    if (!(*table_index_p))
    {
        return pt_allocate_into(table_index_p, options);
    }

    pte_t *pte = (pte_t *)vmm_get_vaddr(pte_get_address(table_index_p));
    pte_apply_options_to_entry(pte, options);

    return pte;
}

/// VMM

pte_t *vmm_init_page_entry(pagetable_context *ctx, void *vaddr, PTABLE_LVL level, PAGE_MAPPING_OPTIONS options)
{
    vaddr = VMM_ALIGN_4KB(vaddr);
    if (vaddr == NULL)
        return NULL;

    pte_t *pml4_entry = pte_get_index(ctx->root_table, offset_to_index(vaddr, PTABLE_LVL_PML4));
    pte_t *pdpt = pt_get_or_allocate_into(pml4_entry, options);
    if (!pdpt)
        return NULL;

    pte_t *pdpt_entry = pte_get_index(pdpt, offset_to_index(vaddr, PTABLE_LVL_PDPT));
    if (level == PTABLE_LVL_PDPT)
        return pdpt_entry;

    pte_t *pde = pt_get_or_allocate_into(pdpt_entry, options);
    if (!pde)
        return NULL;

    pte_t *pde_entry = pte_get_index(pde, offset_to_index(vaddr, PTABLE_LVL_PDE));
    if (level == PTABLE_LVL_PDE)
        return pde_entry;

    pte_t *pt = pt_get_or_allocate_into(pde_entry, options);
    if (!pt)
        return NULL;

    pte_t *pte = pte_get_index(pt, offset_to_index(vaddr, PTABLE_LVL_PTE));

    return pte;
}

void vmm_free_pte(void *vaddr, pte_t *pte)
{
    pmm_free(pte_get_address(pte));
    (*pte) = 0;
    flush_tlb((uint64_t)vaddr);
}

void vmm_free_page_entry(pagetable_context *ctx, void *vaddr)
{
    vaddr = VMM_ALIGN_4KB(vaddr);
    if (vaddr == NULL)
        return;

    pte_t *pml4_entry = pte_get_index(ctx->root_table, offset_to_index(vaddr, PTABLE_LVL_PML4));
    pte_t *pdpt = (pte_t *)vmm_get_vaddr(pte_get_address(pml4_entry));
    if (!pdpt)
        return;

    pte_t *pdpt_entry = pte_get_index(pdpt, offset_to_index(vaddr, PTABLE_LVL_PDPT));

    if (pte_is_huge_page(pdpt_entry))
    {
        vmm_free_pte(vaddr, pdpt_entry);
        return;
    }

    pte_t *pde = (pte_t *)vmm_get_vaddr(pte_get_address(pdpt_entry));
    if (!pde)
        return;

    pte_t *pde_entry = pte_get_index(pde, offset_to_index(vaddr, PTABLE_LVL_PDE));

    if (pte_is_huge_page(pde_entry))
    {
        vmm_free_pte(vaddr, pde_entry);
        return;
    }

    pte_t *pt = (pte_t *)vmm_get_vaddr(pte_get_address(pde_entry));
    ;
    if (!pt)
        return;

    pte_t *pte = pte_get_index(pt, offset_to_index(vaddr, PTABLE_LVL_PTE));
    vmm_free_pte(vaddr, pte);
}

ERROR_CODE vmm_map(pagetable_context *ctx, void *vaddr, void *paddr, PAGE_MAPPING_OPTIONS options)
{
    pte_t *entry = vmm_init_page_entry(ctx, vaddr, PTABLE_LVL_PTE, options);

    if (entry == NULL)
        return FAILED;

    (*entry) = pte_create_entry(paddr, options);

    flush_tlb((uint64_t)vaddr);

    return SUCCESS;
}

void *vmm_map_mmio_region(pagetable_context *ctx, void *phys_start, void *phys_end)
{
    phys_start = VMM_ALIGN_4KB(phys_start);
    phys_end = VMM_ALIGN_4KB(phys_end);
    void *vaddr = (void *)(VMM_RAM_MMIO_MAPPING_OFFSET | (uint64_t)phys_start);
    void *current_vaddr = vaddr;

    while ((uint64_t)phys_start < (uint64_t)phys_end)
    {
        pte_t *entry = vmm_init_page_entry(ctx, current_vaddr, PTABLE_LVL_PTE, PAGE_MAPPING_DEFAULT);

        if (entry == NULL)
            return NULL;

        (*entry) = pte_create_entry(phys_start, PAGE_MAPPING_NON_CACHEABLE);
        flush_tlb((uint64_t)current_vaddr);

        phys_start = (void *)((uint64_t)phys_start + PAGE_SIZE);
        current_vaddr = (void *)((uint64_t)current_vaddr + PAGE_SIZE);
    }

    return vaddr;
}

BOOL vmm_is_mmio(void *virt, void *phys)
{
    return (BOOL)(virt == ((void *)(VMM_RAM_MMIO_MAPPING_OFFSET | (uint64_t)phys)));
}

pagetable_context *vmm_get_global_context()
{
    return &_km_vmm_ctx;
}

/// VMM USERSPACE

pagetable_context *vmm_create_userspace_context()
{
    pagetable_context *ctx = (pagetable_context *)kmalloc(sizeof(pagetable_context));
    if (!ctx)
        return NULL;

    ctx->root_table = (pte_t *)pmm_alloc(PAGE_SIZE);

    for (uint16_t i = VMM_PTABLE_ENTRY_COUNT / 2; i < VMM_PTABLE_ENTRY_COUNT; i++)
    {
        *pte_get_index(ctx->root_table, i) = *pte_get_index(_km_vmm_ctx.root_table, i);
    }

    return ctx;
}

void *vmm_allocate_umm(pagetable_context *ctx, uint64_t vaddress, size_t len)
{
    qemu_logf("Allocating %d bytes at %x", len, vaddress);
    vaddress = (uint64_t)VMM_ALIGN_4KB(vaddress);
    len = (size_t)VMM_ALIGN_4KB(len);

    if (vaddress + len >= VMM_HIGHER_HALF_KERNEL_OFFSET)
    {
        return NULL;
    }

    for (uint64_t vcur = vaddress; vcur < vaddress + len; vcur += PAGE_SIZE)
    {
        void *phys = pmm_alloc();
        if (!phys)
        {
            for (uint64_t vlast = vcur - PAGE_SIZE; vcur >= vaddress; vlast -= PAGE_SIZE)
            {
                vmm_free_page_entry(ctx, (void *)vlast);
            }

            return NULL;
        }

        if (!vmm_map(ctx, (void *)vcur, phys, PAGE_MAPPING_USERSPACE))
        {
            for (uint64_t vlast = vcur; vcur >= vaddress; vlast -= PAGE_SIZE)
            {
                vmm_free_page_entry(ctx, (void *)vlast);
            }

            return NULL;
        }
    }

    return (void *)vaddress;
}

void vmm_destroy_userspace_context(pagetable_context *ctx)
{
    for (uint16_t l4 = 0; l4 < VMM_PTABLE_ENTRY_COUNT / 2; l4++)
    {
        pte_t *pdpt = pte_get_address(&ctx->root_table[l4]);
        if (!pdpt)
            continue;

        for (uint16_t l3 = 0; l3 < VMM_PTABLE_ENTRY_COUNT; l3++)
        {
            pte_t *pde = pte_get_address(&pdpt[l3]);
            if (!pde)
                continue;

            for (uint16_t l2 = 0; l2 < VMM_PTABLE_ENTRY_COUNT; l2++)
            {
                pte_t *pt = pte_get_address(&pde[l2]);
                if (!pt)
                    continue;

                for (uint16_t l1 = 0; l1 < VMM_PTABLE_ENTRY_COUNT; l1++)
                {
                    void *phys = pte_get_address(&pt[l1]);
                    if (!phys)
                        continue;
                    pmm_free(phys);
                }

                pmm_free(pt);
            }

            pmm_free(pde);
        }

        pmm_free(pdpt);
    }

    pmm_free(ctx->root_table);
}

void vmm_apply_pagetable(pagetable_context *ctx)
{
    write_cr3((uint64_t)ctx->root_table);
}

/// INITIALIZATION

void vmm_direct_map_gigabytes(pagetable_context *ctx, uint16_t count)
{
    const uint64_t GIGABYTE = 1024 * 1024 * 1024;

    uint8_t *vstart_addr = (uint8_t *)VMM_RAM_DIRECT_MAPPING_OFFSET;

    for (uint16_t i = 0; i < count; i++, vstart_addr += GIGABYTE)
    {
        pte_t *pdpt_entry = vmm_init_page_entry(ctx, vstart_addr, PTABLE_LVL_PDPT, PAGE_MAPPING_DEFAULT);
        (*pdpt_entry) = pte_create_entry((void *)(GIGABYTE * i), PAGE_MAPPING_HUGE_PAGE);
    }
}

void vmm_direct_map_kernel(pagetable_context *ctx)
{
    uint8_t *current_vaddr = (uint8_t *)VMM_HIGHER_HALF_KERNEL_OFFSET;
    uint8_t *current_paddr = (uint8_t *)0x0;

    for (uint16_t i = 0; i < _km_vmm_globals.kernel_size_pages; i++)
    {
        if (!vmm_map(ctx, (void *)current_vaddr, (void *)current_paddr, PAGE_MAPPING_DEFAULT))
        {
            qemu_log("Failed to map kernel");
            return;
        }

        current_paddr += PAGE_SIZE;
        current_vaddr += PAGE_SIZE;
    }
}

void vmm_setup_higher_half_pml4(pte_t *pml4)
{
    for (uint16_t i = VMM_PTABLE_ENTRY_COUNT / 2; i < VMM_PTABLE_ENTRY_COUNT; i++)
    {
        if (!pt_get_or_allocate_into(pte_get_index(pml4, i), PAGE_MAPPING_DEFAULT))
        {
            qemu_logf("Failed to allocate higher half pml4 entry %d", i);
            return;
        }
    }
}

ERROR_CODE vmm_init()
{
    _km_vmm_ctx.root_table = (pte_t *)pmm_alloc();
    if (!_km_vmm_ctx.root_table)
        return FAILED;

    memset((void *)vmm_get_vaddr(_km_vmm_ctx.root_table), 0, PAGE_SIZE);

    vmm_direct_map_gigabytes(&_km_vmm_ctx, _km_vmm_globals.direct_mapping_size_gb);
    vmm_direct_map_kernel(&_km_vmm_ctx);

    pmm_load_root_ptable(_km_vmm_ctx.root_table);

    _km_vmm_globals.is_remapped = TRUE;

    vmm_setup_higher_half_pml4(_km_vmm_ctx.root_table);

    return SUCCESS;
}
