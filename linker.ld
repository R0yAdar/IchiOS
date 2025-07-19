MEMORY 
{
	boot_sector (rwx) : ORIGIN = 0x7c00, LENGTH = 512
	stage2 (rwx) : ORIGIN = 0x7e00, LENGTH = 1024
	kernel (rwx) : ORIGIN = 0x8200, LENGTH = 0x10000
	kernel_end (rwx): ORIGIN = 0x18200, LENGTH = 1
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
	.stage2 : { *(.stage2); } > stage2
	.text : { *(.text); } > kernel
	.data : { *(.data); } > kernel
    .rodata : { *(.rodata); } > kernel
    .bss :
    {
        *(.bss)
        *(COMMON)
    } > kernel
	.kernel_end : { *(.kernel_end); } > kernel_end
}