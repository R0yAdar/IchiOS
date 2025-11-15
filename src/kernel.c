#include "core/interrupts/idt.h"
#include "core/interrupts/x86_pic.h"
#include "core/interrupts/x86_pit.h"
#include "ahci.h"
#include "stdint.h"
#include "vga.h"
#include "multiboot.h"
#include "str.h"
#include "print.h"
#include "pmm.h"
#include "err.h"
#include "vmm.h"
#include "core/gdt/gdt.h"
#include "assembly.h"
#include "keyboard.h"
#include "core/hal.h"
#include "pci.h"
#include "serial.h"
#include "ext2.h"

#define ARRAY_SIZE(arr) ((int)sizeof(arr) / (int)sizeof((arr)[0]))
#define KERNEL_STACK_SIZE 4
#define PAGE_SIZE 4096
#define KERNEL_PHYS_OFFSET 0x8200

extern uint64_t __bss_start;
extern uint64_t __bss_end;
extern uint64_t __kernel_origin;

void* _top_of_kernel_stack = NULL;
io_device device = {0};
tss _tss = {0};

void __early_zero_bss(void* bss_start, void* bss_end) {
    uint64_t* ptr = (uint64_t*)bss_start;
    uint64_t* end = (uint64_t*)bss_end;

    while (ptr < end) {
        *ptr++ = 0;
    }
}

void __early_init_pmm(multiboot_info* info) {
	pmm_context context;

	context.regions = (memory_region*)((uint64_t)info->m_mmap_addr);
	context.regions_count = info->m_mmap_length;
	// Size of actual kernel + the offset of it in memory (to also preserve bootloader + stage2)
	context.kernel_ram_size = ((uint64_t)&__bss_end - (uint64_t)&__kernel_origin) + KERNEL_PHYS_OFFSET;
	
	pmm_init(&context);
}

void _rest_of_start();

void init_kernel_stack() {
	void* stack = kpage_alloc(KERNEL_STACK_SIZE);
	void* stack_top = (void*)((char*)stack + KERNEL_STACK_SIZE * PAGE_SIZE);

	print("Stack allocated from: "); printx((uint64_t)stack_top); print(" -> "); printxln((uint64_t)stack);
	_top_of_kernel_stack = stack_top;

	switch_stack(stack_top, _rest_of_start);
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

void key_callback(uint8_t scancode, BOOL pressed) {
	char buffer[] = {0, 0};
	if (pressed) {
		uint32_t key = kybrd_key_to_ascii(scancode);
		
		if (key < 128) {			
			if(key == '0') {
				key = '1';
			}

			buffer[0] = key;

			print(buffer);
		}
	}
}

void mount_root_fs() {
	device.read = ahci_read;
	device.write = ahci_write;
	device.start_lba = 2048; // skip first 2048 sectors (1MB)
	device.end_lba = 2048 + 2048 * 10; // (10MB file system)

	ext2_context* context = ext2_init(&device);
	file_system fs = {0};
	fs.ops = &ext2_ops;
	fs.context = context;
	
	if (!mount(fs, "/")) {
		qemu_log("Failed to mount");
	} else {
		qemu_log("Mounted ahci-ext2 fs");
	}
}

void _start_kernel(multiboot_info* info) {
	serial_init();
	vga_clear_screen();
	println("Ichi kernel loading...");
	
	__early_zero_bss(&__bss_start, &__bss_end);
	__early_init_pmm(info);
	println("INIT PMM MEMORY MANAGER");

	init_idt();

	init_pic();
	
	println("Ichi kernel enabled PIC...");

	init_pit();
	
	println("Ichi kernel enabled PIT...");

	kybrd_init();

	kybrd_set_event_callback(key_callback);

	println("Ichi kernel enabled KEYBOARD...");

	init_gdt(0, 0);
	init_vmem();

	println("Ichi kernel setup VMM...");

	init_kernel_stack();
}

void _rest_of_start() {
	qemu_log("Ichi kernel reallocated stack...");
	_tss = create_tss_segment(_top_of_kernel_stack);
	init_gdt(&_tss, sizeof(tss));
	//syscall(0, 0);

	void* p1 = kmalloc(16);
	printxln(p1);

	
	void* p2 = kmalloc(8);
	kfree(p2);
	printxln(p2);

	void* p3 = kmalloc(8);

	print("Allocating umm: ");

	printxln(p3);
	size_t amount = allocate_umm(0, 4096 * 1);

	print("Allocated: "); printx(amount); println(" for user-space");	
	void* usermem = (void*)0;
	(*(uint64_t*)usermem) = 0xDEADBEEF;

	sti();

	qemu_log("Set hardware interrupts (sti)");

	ahci_init();

	mount_root_fs();

	file* f = fopen("/files/readme.txt", READ);

	if (!f) {
		qemu_log("Failed to open file");
	} else {
		qemu_log("Opened file");
	}

	void* data = kpage_alloc(1);

	qemu_log("Allocated page");

	fread(data, 1, 1024, f);

	qemu_log("Read file from main");

	fclose(f);

	qemu_log(data);
	
	while(1) { hlt(); } // if we return to bootloader - we'll double fault
	qemu_log("Out of loop ?_?");
}