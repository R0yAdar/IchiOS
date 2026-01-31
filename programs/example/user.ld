/* programs/user.ld */
ENTRY(_start)

SECTIONS
{
    /* Common base address for user programs */
    . = 0x40000;

    .text : {
        *(.text)
    }

    .rodata : {
        *(.rodata)
    }

    .data : {
        *(.data)
    }

    .bss : {
        *(.bss)
        *(COMMON)
    }

    /DISCARD/ : {
        *(.comment)
        *(.eh_frame)
    }
}