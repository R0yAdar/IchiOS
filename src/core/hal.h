#ifndef HAL_H
#define HAL_H

#include "stdint.h"

typedef uint64_t gdt_desc;

#define SEG_DESCTYPE(x) ((x) << 0x04)      // Descriptor type (0 for system, 1 for code/data)
#define SEG_PRES(x) ((x) << 0x07)          // Present
#define SEG_SAVL(x) ((x) << 0x0C)          // Available for system use
#define SEG_LONG(x) ((x) << 0x0D)          // Long mode
#define SEG_SIZE(x) ((x) << 0x0E)          // Size (0 for 16-bit, 1 for 32)
#define SEG_GRAN(x) ((x) << 0x0F)          // Granularity (0 for 1B - 1MB, 1 for 4KB - 4GB)
#define SEG_PRIV(x) (((x) & 0x03) << 0x05) // Set privilege level (0 - 3)

#define SEG_DATA_RD 0x00        // Read-Only
#define SEG_DATA_RDA 0x01       // Read-Only, accessed
#define SEG_DATA_RDWR 0x02      // Read/Write
#define SEG_DATA_RDWRA 0x03     // Read/Write, accessed
#define SEG_DATA_RDEXPD 0x04    // Read-Only, expand-down
#define SEG_DATA_RDEXPDA 0x05   // Read-Only, expand-down, accessed
#define SEG_DATA_RDWREXPD 0x06  // Read/Write, expand-down
#define SEG_DATA_RDWREXPDA 0x07 // Read/Write, expand-down, accessed
#define SEG_CODE_EX 0x08        // Execute-Only
#define SEG_CODE_EXA 0x09       // Execute-Only, accessed
#define SEG_CODE_EXRD 0x0A      // Execute/Read
#define SEG_CODE_EXRDA 0x0B     // Execute/Read, accessed
#define SEG_CODE_EXC 0x0C       // Execute-Only, conforming
#define SEG_CODE_EXCA 0x0D      // Execute-Only, conforming, accessed
#define SEG_CODE_EXRDC 0x0E     // Execute/Read, conforming
#define SEG_CODE_EXRDCA 0x0F    // Execute/Read, conforming, accessed

#define GDT_CODE_PL0 SEG_DESCTYPE(1) | SEG_PRES(1) | SEG_SAVL(0) | \
                         SEG_LONG(1) | SEG_SIZE(0) | SEG_GRAN(1) | \
                         SEG_PRIV(0) | SEG_CODE_EXRD

#define GDT_DATA_PL0 SEG_DESCTYPE(1) | SEG_PRES(1) | SEG_SAVL(0) | \
                         SEG_LONG(1) | SEG_SIZE(0) | SEG_GRAN(1) | \
                         SEG_PRIV(0) | SEG_DATA_RDWR

#define GDT_CODE_PL3 SEG_DESCTYPE(1) | SEG_PRES(1) | SEG_SAVL(0) | \
                         SEG_LONG(1) | SEG_SIZE(0) | SEG_GRAN(1) | \
                         SEG_PRIV(3) | SEG_CODE_EXRD

#define GDT_DATA_PL3 SEG_DESCTYPE(1) | SEG_PRES(1) | SEG_SAVL(0) | \
                         SEG_LONG(1) | SEG_SIZE(0) | SEG_GRAN(1) | \
                         SEG_PRIV(3) | SEG_DATA_RDWR

#define GDT_NULL_OFFSET 0
#define GDT_CODE_PL0_OFFSET (1 * sizeof(gdt_desc))
#define GDT_DATA_PL0_OFFSET (2 * sizeof(gdt_desc))
#define GDT_CODE_PL3_OFFSET (3 * sizeof(gdt_desc))
#define GDT_DATA_PL3_OFFSET (4 * sizeof(gdt_desc))
#define GDT_TSS_OFFSET (5 * sizeof(gdt_desc))
                         
#pragma pack(push, 1)

typedef struct
{
    uint32_t reserved0;
    uint64_t rsp0;
    uint64_t rsp1;
    uint64_t rsp2;
    uint64_t reserved1;
    uint64_t ist1;
    uint64_t ist2;
    uint64_t ist3;
    uint64_t ist4;
    uint64_t ist5;
    uint64_t ist6;
    uint64_t ist7;
    uint64_t reserved2;
    uint16_t reserved3;
    uint16_t iopb_offset;
} tss;

typedef struct
{
    uint16_t limit_low;
    uint16_t base_low;
    uint8_t base_mid;

    uint8_t type : 4;
    uint8_t s : 1;
    uint8_t dpl : 2;
    uint8_t p : 1;

    uint8_t limit_high : 4;
    uint8_t avl : 1;
    uint8_t l : 1;
    uint8_t db : 1;
    uint8_t g : 1;

    uint8_t base_high;

    uint32_t base_upper;
    uint32_t reserved;
} gdt_tss_descriptor;

typedef struct
{
    uint16_t offset_low16;
    uint16_t segment_selector;

    uint8_t ist;
    uint8_t flags;

    uint16_t offset_upper16;
    uint32_t offset_upper32;
    uint32_t reserved;
} idt_descriptor;

typedef struct
{
    gdt_desc null;
    gdt_desc code_pl0;
    gdt_desc data_pl0;
    gdt_desc code_pl3;
    gdt_desc data_pl3;
    gdt_tss_descriptor tss;
} gdt_table;

typedef struct
{
    uint16_t limit;
    uint64_t base;
} gdt_ptr;

#pragma pack(pop)

idt_descriptor idt_create_descriptor(void (*handler)());

idt_descriptor idt_create_userland_descriptor(void (*handler)());

gdt_desc gdt_create_descriptor(uint32_t base, uint32_t limit, uint16_t flag);

gdt_tss_descriptor gdt_create_tss_descriptor(uint64_t tss_address, uint16_t tss_size);

tss create_tss_segment(void *kernel_stack_top);

#endif