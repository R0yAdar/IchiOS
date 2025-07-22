#include "core/interrupts/idt.h"
#include "core/interrupts/x86_pic.h"
#include "core/interrupts/x86_pit.h"
#include "stdint.h"
#include "vga.h"
#include "multiboot.h"
#include "str.h"
#include "print.h"

#define ARRAY_SIZE(arr) ((int)sizeof(arr) / (int)sizeof((arr)[0]))


void _start_kernel(multiboot_info* info) {
	vga_clear_screen();

	const char loading_message[] = "Ichi kernel loading...";
	const char configured_pic_message[] = "Ichi kernel enabled PIC...";
	const char enabled_pit_message[] = "Ichi kernel enabled PIT...";

	vga_text_input input  = {0, 0, loading_message, 0x09};
	vga_put(&input);

	input.text = configured_pic_message;
	++input.y;
	vga_put(&input);

	init_idt();

	systemCall(0, 0);

	volatile int a = 5;

	// int b = a / 0;

	init_pic();
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

	print("Memory LOW: "); printiln(info->m_memoryLo);
	print("Memory HIGH: "); printiln(info->m_memoryHi);


	print("Starting from ");
	printiln(info->m_mmap_addr);

	print("Length is ");
	printiln(info->m_mmap_length);

	/*
	for(uint32_t start = info->m_mmap_addr; 
		start < info->m_mmap_addr + info->m_mmap_length * sizeof(memory_region);
		++start){
			*((char*)start) = 0;
		}
	*/
	

	for(int i =0; i < info->m_mmap_length; ++i) {
		print("MEMORY REGION DETECTED FROM -> ");

		printi(regions->address);
		print("  SIZE:");
		printi(regions->size);
		print(" TYPE: ");
		printiln(regions->type);

		++regions;
	}
}