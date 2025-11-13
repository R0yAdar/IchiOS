MEMORY 
{
	boot_sector (rwx) : ORIGIN = 0x7c00, LENGTH = 512
	stage2 (rwx) : ORIGIN = 0x7e00, LENGTH = 1024
	kernel (rwx) : ORIGIN = 0xffffffff80008200, LENGTH = 0x120000
}

ENTRY(__start)
SECTIONS
{
	.boot_sector : { *(.boot_sector); } > boot_sector
	
	.boot_sign (0x7c00 + 510) :
	{
		BYTE(0x55)
		BYTE(0xaa)
	} > boot_sector
	
	.stage2 :
	{
		*(.stage2)
		. = ORIGIN(stage2) + LENGTH(stage2) - 1;
		BYTE(0)
	} > stage2

	. = ORIGIN(kernel);
	__kernel_origin = .;

	.text : { *(.text); } > kernel
	.data : { *(.data); } > kernel
    .rodata : { *(.rodata); } > kernel
    .bss ALIGN(16) :
    {
		__bss_start = .;
        *(.bss)
        *(COMMON)
		__bss_end = .;
    } > kernel	
}