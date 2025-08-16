#include "core/interrupts/idt.h"
#include "core/interrupts/x86_pic.h"
#include "core/interrupts/x86_pit.h"
#include "stdint.h"
#include "vga.h"
#include "multiboot.h"
#include "str.h"
#include "print.h"
#include "pmm.h"
#include "err.h"
#include "core/mem/vmem.h"
#include "core/gdt/gdt.h"
#include "assembly.h"

#define ARRAY_SIZE(arr) ((int)sizeof(arr) / (int)sizeof((arr)[0]))

extern __bss_start;
extern __bss_end;
extern __kernel_origin;

void __early_zero_bss(void* bss_start, void* bss_end) {
    uint64_t* ptr = (uint64_t*)bss_start;
    uint64_t* end = (uint64_t*)bss_end;

    while (ptr < end) {
        *ptr++ = 0;
    }
}

void __early_init_pmm(multiboot_info* info) {
	pmm_context context;

	context.regions = (memory_region*)info->m_mmap_addr;
	context.regions_count = info->m_mmap_length;
	// Size of actual kernel + the offset of it in memory (to also preserve bootloader + stage2)
	context.kernel_ram_size = (&__bss_end - &__kernel_origin) + 0x8200;
	pmm_init(&context);
}

void __early_init_stack() {
	int stack_size = 4; // pages
	void* stack = pmm_alloc_blocks(stack_size);
	void* stack_top = (void*)((char*)stack + stack_size * 4096);

	print("Stack allocated from: "); printx(stack_top); print(" -> "); printxln(stack);

	set_rsp(stack_top);
}

void print_memory(multiboot_info* info) {
	memory_region* regions = (memory_region*)info->m_mmap_addr;
		
	print("Memory LOW: "); printxln(info->m_memoryLo * 1024);
	print("Memory HIGH: "); printxln(info->m_memoryHi * 64 * 1024);

	for(int i =0; i < info->m_mmap_length; ++i) {
		print("MEMORY REGION DETECTED FROM -> ");

		printx(regions->address);
		print("  SIZE:");
		printx(regions->size);
		print(" TYPE: ");
		printiln(regions->type);

		++regions;
	}

	print("kernel origin: "); printxln(&__kernel_origin);
	print("kernel size on ram: "); printxln(&__bss_end - &__kernel_origin);
}

void _start_kernel(multiboot_info* info) {
	vga_clear_screen();
	
	__early_zero_bss(&__bss_start, &__bss_end);
	__early_init_pmm(info);
	__early_init_stack();

	const char loading_message[] = "Ichi kernel loading...";
	const char configured_pic_message[] = "Ichi kernel enabled PIC...";
	const char enabled_pit_message[] = "Ichi kernel enabled PIT...";

	println(loading_message);

	println("INIT PMM MEMORY MANAGER");

	init_idt();

	syscall(0, 0);

	init_pic();
	
	println(configured_pic_message);

	init_pit();
	
	println(enabled_pit_message);

	init_gdt();

	print_memory(info);

	init_vmem();

	sti();
	
	while(1) { hlt(); } // if we return to bootloader - we'll double fault
	println("Out of loop");
}