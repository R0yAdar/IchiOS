    align 16
gdt64_start:
    ;; 8-byte null descriptor (index 0).
    dq 0x0
	
gdt64_code_segment:
    dw 0xffff
    dw 0x0000
    db 0x00
    db 10011010b
    db 10101111b
    db 0x00

gdt64_data_segment:
    dw 0xffff
    dw 0x0000
    db 0x00
    db 10010010b
    db 10101111b
    db 0x00

gdt64_userspace_code_segment:
	dw 0xffff
	dw 0x0
	db 0x0
	db 11111010b
	db 10101111b
	db 0x0

gdt64_userspace_data_segment:
	dw 0xffff
	dw 0x0
	db 0x0
	db 11111010b
	db 10101111b
	db 0x0

gdt64_end:

gdt64_pseudo_descriptor:
    dw gdt64_end - gdt64_start - 1
    dq gdt64_start

CODE_SEG64 equ gdt64_code_segment - gdt64_start
DATA_SEG64 equ gdt64_data_segment - gdt64_start