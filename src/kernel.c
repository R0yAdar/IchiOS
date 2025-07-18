#include "core/graphics/vga.h"
#include "core/interrupts/idt.h"
#include "core/interrupts/x86_pic.h"
#include "core/interrupts/x86_pit.h"
#include "core/stdint.h"

#define ARRAY_SIZE(arr) ((int)sizeof(arr) / (int)sizeof((arr)[0]))


typedef struct multiboot_info_t {

	uint32_t	m_flags;
	uint32_t	m_memoryLo;
	uint32_t	m_memoryHi;
	uint32_t	m_bootDevice;
	uint32_t	m_cmdLine;
	uint32_t	m_modsCount;
	uint32_t	m_modsAddr;
	uint32_t	m_syms0;
	uint32_t	m_syms1;
	uint32_t	m_syms2;
	uint32_t	m_mmap_length;
	uint32_t	m_mmap_addr;
	uint32_t	m_drives_length;
	uint32_t	m_drives_addr;
	uint32_t	m_config_table;
	uint32_t	m_bootloader_name;
	uint32_t	m_apm_table;
	uint32_t	m_vbe_control_info;
	uint32_t	m_vbe_mode_info;
	uint16_t	m_vbe_mode;
	uint32_t	m_vbe_interface_addr;
	uint16_t	m_vbe_interface_len;
} multiboot_info;


/*
0xA0000 - 0xBFFFF Video Memory used for graphics modes
0xB0000 - 0xB7777 Monochrome Text mode
0xB8000 - 0xBFFFF Color text mode and CGA compatible graphics modes
*/

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

}