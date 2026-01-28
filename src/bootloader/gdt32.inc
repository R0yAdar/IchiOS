align 8

gdt32_start:
	; null descriptor
	dq 0x0
	
gdt32_code_segment:
	dw 0xffff
	dw 0x0
	db 0x0
	db 10011010b
	db 11000111b
	db 0x0
	
gdt32_data_segment:
	dw 0xffff
	dw 0x0
	db 0x0
	db 10010010b
	db 11000111b
	db 0x00
	
gdt32_end:

gdt32_pseudo_descriptor:
	dw gdt32_end - gdt32_start - 1
	dd gdt32_start
	
CODE_SEG32 equ gdt32_code_segment - gdt32_start
DATA_SEG32 equ gdt32_data_segment - gdt32_start