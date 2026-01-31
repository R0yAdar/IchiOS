#include "core/intrp/idt.h"
#include "core/intrp/pic.h"
#include "core/intrp/pit.h"
#include "core/user/elf.h"
#include "core/user/process.h"
#include "core/user/scheduler.h"
#include "core/user/syscall.h"
#include "ahci.h"
#include "stdint.h"
#include "vga.h"
#include "multiboot.h"
#include "str.h"
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
#define KERNEL_STACK_SIZE 10
#define PAGE_SIZE 4096
#define KERNEL_PHYS_OFFSET 0x8200

extern uint64_t __bss_start;
extern uint64_t __bss_end;
extern uint64_t __kernel_origin;

void clean_start();

void *_top_of_kernel_stack = NULL;
io_device device = {0};
tss _tss = {0};
vbe_mode_info_structure vbe_mode_info;

void __early_zero_bss(void *bss_start, void *bss_end)
{
	uint64_t *ptr = (uint64_t *)bss_start;
	uint64_t *end = (uint64_t *)bss_end;

	while (ptr < end)
	{
		*ptr++ = 0;
	}
}

void __early_init_pmm(multiboot_info *info)
{
	pmm_context context;

	context.regions = (memory_region *)((uint64_t)info->m_mmap_addr);
	context.regions_count = info->m_mmap_length;
	// Size of actual kernel + the offset of it in memory (to also preserve bootloader + stage2)
	context.kernel_ram_size = ((uint64_t)&__bss_end - (uint64_t)&__kernel_origin) + KERNEL_PHYS_OFFSET;

	pmm_init(&context);
}

void __early_init_framebuffer(multiboot_info *info)
{
	vbe_mode_info_structure *mode_info = (vbe_mode_info_structure *)(uint64_t)info->m_vbe_mode_info;
	vbe_mode_info = *mode_info;
	qemu_logf("Saved framebuffer (Screen=%dx%d, bpp=%d, MemoryModel=%d)", vbe_mode_info.width, vbe_mode_info.height, vbe_mode_info.bpp, vbe_mode_info.memory_model);
}

void __switch_kernel_stack()
{
	void *stack = kpage_alloc(KERNEL_STACK_SIZE);
	void *stack_top = (void *)((char *)stack + KERNEL_STACK_SIZE * PAGE_SIZE);

	_top_of_kernel_stack = stack_top;
	
	qemu_logf("New stack top: %x", stack_top);
	switch_stack(stack_top, clean_start);
}

void mount_root_fs()
{
	device.read = ahci_read;
	device.write = ahci_write;
	device.start_lba = 2048;		   // skip first 2048 sectors (1MB)
	device.end_lba = 2048 + 2048 * 10; // (10MB file system)

	ext2_context *context = ext2_init(&device);
	file_system fs = {0};
	fs.ops = &ext2_ops;
	fs.context = context;

	if (!mount(fs, "/"))
	{
		qemu_log("Failed to mount");
	}
	else
	{
		qemu_log("Mounted ahci-ext2 fs");
	}
}

void _start_kernel(multiboot_info *info)
{
	serial_init();
	vga_clear_screen();
	qemu_log("Ichi kernel loading...");

	__early_zero_bss(&__bss_start, &__bss_end);
	__early_init_pmm(info);
	__early_init_framebuffer(info);

	qemu_log("INIT PMM MEMORY MANAGER");

	idt_init();

	pic_init();

	qemu_log("Ichi kernel enabled PIC...");

	kybrd_init();

	qemu_log("Ichi kernel enabled KEYBOARD...");

	gdt_init(0, 0);

	qemu_log("Ichi kernel setup GDT...");

	vmm_init();

	pit_init();

	qemu_log("Ichi kernel enabled PIT...");

	qemu_log("Ichi kernel setup VMM...");

	__switch_kernel_stack();
}

void clean_start()
{
	_tss = create_tss_segment(_top_of_kernel_stack);
	gdt_init(&_tss, sizeof(tss));
	sti();

	qemu_log("Set hardware interrupts (sti)");

	ahci_init();

	qemu_log("Initializing AHCI...");

	mount_root_fs();

	qemu_log("Mounting root fs...");

	file *f = fopen("/files/readme.txt", READ);

	if (!f)
	{
		qemu_log("Failed to open file");
	}
	else
	{
		qemu_log("Opened file");
	}

	void *data = kpage_alloc(1);

	qemu_log("Allocated page");

	size_t read_len = fread(data, 1, 1024, f);

	qemu_logf("Read file from main: %d bytes", read_len);

	qemu_log(data);

	fclose(f);

	framebuffer *fb = framebuffer_init(
		vmm_get_vaddr((void *)(uint64_t)vbe_mode_info.framebuffer),
		vbe_mode_info.width,
		vbe_mode_info.height,
		vbe_mode_info.pitch,
		vbe_mode_info.bpp,
		vbe_mode_info.memory_model);

	if (!fb)
	{
		qemu_log("failed to get framebuffer");
	}
	else
	{
		framebuffer_clear(fb);
	}

	file *exe = fopen("/files/doom.elf", READ);

	if (!exe) {
		qemu_log("Failed to open exe file");
	}
	else {
		scheduler_init();
		process_ctx* p1 = process_create();
		process_init_idle(p1);
		scheduler_add_process(p1);

		process_ctx* p = process_create();
		process_exec(p, exe);
		scheduler_add_process(p);
	}
	qemu_log("Transferring control to scheduler");

	syscall_init(fb);
	scheduler_transfer_ctrl();
	
	qemu_log("BYE BYE");

	while (1)
	{
		hlt();
	} // if we return to bootloader - we'll double fault
	qemu_log("Out of loop ?_?");
}