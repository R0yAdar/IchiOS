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

#define ARRAY_SIZE(arr) ((int)sizeof(arr) / (int)sizeof((arr)[0]))

int x;

void __early_zero_bss(void* bss_start, void* bss_end) {
    uint64_t* ptr = (uint64_t*)bss_start;
    uint64_t* end = (uint64_t*)bss_end;

    while (ptr < end) {
        *ptr++ = 0;
    }
}

void _start_kernel(multiboot_info* info) {
	extern __bss_start;
	extern __bss_end;

	// __early_zero_bss(&__bss_start, &__bss_end);

	extern bitmap;
	extern __kernel_origin;

	vga_clear_screen();


	const char loading_message[] = "Ichi kernel loading...";
	const char configured_pic_message[] = "Ichi kernel enabled PIC...";
	const char enabled_pit_message[] = "Ichi kernel enabled PIT...";

	println(loading_message);

	init_idt();

	systemCall(0, 0);

	volatile int a = 5;

	init_pic();
	
	println(configured_pic_message);

	init_pit();
	
	asm volatile ("sti" ::: "memory");


	println(enabled_pit_message);

	memory_region* regions = (memory_region*)info->m_mmap_addr;
	
	print("Memory LOW: "); printxln(info->m_memoryLo * 1024);
	print("Memory HIGH: "); printxln(info->m_memoryHi * 64 * 1024);

	print("Starting from ");
	printxln(info->m_mmap_addr);

	print("Length is ");
	printxln(info->m_mmap_length);	

	for(int i =0; i < info->m_mmap_length; ++i) {
		print("MEMORY REGION DETECTED FROM -> ");

		printx(regions->address);
		print("  SIZE:");
		printx(regions->size);
		print(" TYPE: ");
		printiln(regions->type);

		++regions;
	}

	pmm_context context;

	context.regions = (memory_region*)info->m_mmap_addr;
	context.regions_count = info->m_mmap_length;
	// Size of actual kernel + the offset of it in memory (to also preserve bootloader + stage2)
	context.kernel_ram_size = (&__bss_end - &__kernel_origin) + 0x8200;
	print("Kernel ram size: "); printxln(context.kernel_ram_size);

	pmm_init(&context);

	println("INIT PMM MEMORY MANAGER");

	print("PML4 is at: "); printxln(pmm_get_pdbr());

	print("bitmap: "); printxln(&bitmap);
	print("kernel origin: "); printxln(&__kernel_origin);
	print("kernel size on ram: "); printxln(&__bss_end - &__kernel_origin);
	
	void* phys_address = pmm_alloc();
	print("Phys address"); printxln(phys_address);

	init_vmem(info);
}