#include "core/interrupts/idt.h"
#include "core/interrupts/pic.h"
#include "core/user/elf.h"
#include "core/interrupts/pit.h"
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
#include "framebuffer.h"
#include "cstring.h"

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
vbe_mode_info_structure vbe_mode_info;

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

void __early_init_framebuffer(multiboot_info* info) {
	qemu_log("Saving framebuffer info...");
	vbe_mode_info_structure* mode_info = (vbe_mode_info_structure*)(uint64_t)info->m_vbe_mode_info;
	qemu_log_int(mode_info->framebuffer);
	vbe_mode_info = *mode_info;
}

void _rest_of_start();

void init_kernel_stack() {
	void* stack = kpage_alloc(KERNEL_STACK_SIZE);
	void* stack_top = (void*)((char*)stack + KERNEL_STACK_SIZE * PAGE_SIZE);

	print("Stack allocated from: "); printx((uint64_t)stack_top); print(" -> "); printxln((uint64_t)stack);
	_top_of_kernel_stack = stack_top;

	switch_stack(stack_top, _rest_of_start);
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

void jump_to_userland(uint64_t stack_addr, uint64_t code_addr)
{
    asm volatile(" \
        push $0x23 \n\
        push %0 \n\
        push $0x202 \n\
        push $0x1B \n\
        push %1 \n\
        iretq \n\
        " :: "r"(stack_addr), "r"(code_addr));
}

void _start_kernel(multiboot_info* info) {
	serial_init();
	vga_clear_screen();
	println("Ichi kernel loading...");
	
	__early_zero_bss(&__bss_start, &__bss_end);
	__early_init_pmm(info);
	__early_init_framebuffer(info);
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

	size_t read_len = fread(data, 1, 1024, f);

	qemu_logf("Read file from main: %d bytes", read_len);

	qemu_log(data);

	fclose(f);

	qemu_logf("Screen %dx%d", vbe_mode_info.height, vbe_mode_info.width);

	qemu_logf("Logging bpp: %d", vbe_mode_info.bpp);

	qemu_logf("Memory model: %d", vbe_mode_info.memory_model);


	framebuffer* fb = framebuffer_init(
		vphys_address((void*)(uint64_t)vbe_mode_info.framebuffer), 
		vbe_mode_info.width, 
		vbe_mode_info.height, 
		vbe_mode_info.pitch, 
		vbe_mode_info.bpp, 
		vbe_mode_info.memory_model
	);
	
	if (!fb) {
		qemu_log("failed to get framebuffer");
	} else {
		framebuffer_clear(fb);

		framebuffer_draw_char8x8(fb, 0, 0, 'H', 0xF800, 2);
		framebuffer_draw_char8x8(fb, 16, 0, 'E', 0x07C0, 2);
		framebuffer_draw_char8x8(fb, 32, 0, 'L', 0x001F, 2);
		framebuffer_draw_char8x8(fb, 48, 0, 'L', 0xF800, 2);
		framebuffer_draw_char8x8(fb, 64, 0, 'O', 0x07C0, 2);
		framebuffer_draw_char8x8(fb, 80, 0, ' ', 0x001F, 2);
		framebuffer_draw_char8x8(fb, 96, 0, 'W', 0xF800, 2);
		framebuffer_draw_char8x8(fb, 112, 0, 'O', 0x07C0, 2);
		framebuffer_draw_char8x8(fb, 128, 0, 'R', 0x001F, 2);
		framebuffer_draw_char8x8(fb, 144, 0, 'L', 0xF800, 2);
		framebuffer_draw_char8x8(fb, 160, 0, 'D', 0x07C0, 2);
		framebuffer_draw_char8x8(fb, 176, 0, '!', 0x001F, 2);
		
		qemu_log("tried to draw something");
	}

	syscall(0, 0);

	size_t amount = allocate_umm(4096, 4096 * 1);

	qemu_logf("Allocated: %d for user-space", amount);

	file* exe = fopen("/files/example.elf", READ);

	if (!exe) qemu_log("Failed to open exe file");
	else elf_load(exe);	

	/*
	volatile BOOL test = FALSE;

	if (test) {	
		loop_start:
		while(1) { 
    		asm volatile( "int $0x80" :: "a"(0), "c"(0) : "memory" );
			asm volatile( "int $0x80" :: "a"(1), "c"(0) : "memory" );
		} // sample code
	}

	void* usermem = (void*)4096;
	for (size_t i = 0; i < 100; i++)
	{
		*((uint8_t*)usermem + i) = *((uint8_t*)&&loop_start + i);
	}

	qemu_log("Jumping to usermode");
	
	jump_to_userland(7000, 4096);

	*/

	while(1) { hlt(); } // if we return to bootloader - we'll double fault
	qemu_log("Out of loop ?_?");
}