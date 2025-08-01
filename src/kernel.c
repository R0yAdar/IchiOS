#include "core/interrupts/idt.h"
#include "core/interrupts/x86_pic.h"
#include "core/interrupts/x86_pit.h"
#include "stdint.h"
#include "vga.h"
#include "multiboot.h"
#include "str.h"
#include "print.h"
#include "pmm.h"

#define ARRAY_SIZE(arr) ((int)sizeof(arr) / (int)sizeof((arr)[0]))


void _start_kernel(multiboot_info* info) {
	vga_clear_screen();


	const char loading_message[] = "Ichi kernel loading...";
	const char configured_pic_message[] = "Ichi kernel enabled PIC...";
	const char enabled_pit_message[] = "Ichi kernel enabled PIT...";

	vga_text_input input  = {0, 0, loading_message, 0x09};
	vga_put(&input);

	printxln(&loading_message);

	init_idt();

	systemCall(0, 0);

	volatile int a = 5;

	// int b = a / 0;



	init_pic();
	input.text = configured_pic_message;
	++input.y;
	vga_put(&input);
	init_pit();
	
	asm volatile ("sti" ::: "memory");

	input.text = enabled_pit_message;
	++input.y;
	vga_put(&input);

	++input.y;

	input.text = int_to_str(12406378);
	++input.y;
	vga_put(&input);

	input.text = int_to_str(124506378);
	++input.y;
	vga_put(&input);

	input.text = "brah.";
	++input.y;
	vga_put(&input);

	input.text = int_to_str(info->m_mmap_addr);
	++input.y;
	vga_put(&input);

	input.text = "Memory region detected";
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

	pmm_init(&context);

	println("INIT PMM MEMORY MANAGER");

	void* memory_block = pmm_alloc();
	print("GOT: "); printxln(memory_block);
	// println("FREED");
	// pmm_free(memory_block);
	memory_block = pmm_alloc();
	print("GOT: "); printxln(memory_block);
}