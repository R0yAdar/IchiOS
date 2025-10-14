[bits 16]

section .boot_sector

global __start

__start:
	mov [drive_number], dl
	
	mov bx, startup_msg
	call print_string
	
	mov si, disk_address_packet
	call load_disk_sectors

end:
	hlt
	jmp end
	
load_disk_sectors:
	; kernel_end is a symbol that marks the end of the kernel
	;extern kernel_end
	;mov eax, kernel_end

	; the bootsector is loaded at 0x7c00
	; the stage2+kernel are right after - starting from 0x7e00
	;sub eax, 0x7e00
	;shr eax, 9
	mov eax, 100
	mov [dap_sectors_num], ax
	mov [sectors_load_count], ax

	mov ah, 0x42 ; "extended read"
	mov dl, [drive_number]
	int 0x13

	mov bx, read_sectors_msg1
	call print_string

	mov ax, [dap_sectors_num]
	call print_int

	mov bx, read_sectors_msg2
	call print_string

	mov ax, [sectors_load_count]
	call print_int

	mov bx, read_sectors_msg3
	call print_string
	
	jc handle_disk_error

ignore_disk_error:
	mov bx, press_any_key_msg
	call print_string

	; wait for key	
	mov ah, 0x00
	int 0x16

	extern stage2_start
	jmp 0:stage2_start

handle_disk_error:
	mov ax, [sectors_load_count]
	cmp word [dap_sectors_num], ax
	jle ignore_disk_error
	
	mov bx, error_reading_disk_msg
	call print_string

; prints int16 from ax
print_int:
	pusha
	mov bx, int_buffer + 4

print_int_loop:
	cmp bx, int_buffer - 1
	je print_int_end
	xor dx, dx
	push bx
	
	mov bx, 10
	div bx
	
	pop bx
	add dl, '0'
	mov [bx], dl
	dec bx
	jmp print_int_loop

print_int_end:
	mov bx, int_buffer
	call print_string
	popa
	ret


; prints string from bx
print_string:
	pusha
	mov ah, 0x0e
	
	print_string_loop:
	cmp byte [bx], 0
	
	je print_string_end
	
	mov al, [bx]
	int 0x10
	
	inc bx
	jmp print_string_loop
	
print_string_end:
	popa
	ret

; data segment
align 4
disk_address_packet:
	db 0x10 ; size of struct
	db 0 ; unused
dap_sectors_num:
	dw 0 ; number of sectors to read
	dd 0x7c00 + 512 ; right after our bootsector
	dq 1 ; sector to start at (skip the bootsector)

int_buffer: db '0', '0', '0', '0', '0', 0
sectors_load_count dw 0
drive_number: db 0
read_sectors_msg1: db "Read ", 0
read_sectors_msg2: db " out of ", 0
read_sectors_msg3: db " requested sectors.", 13, 10,0
startup_msg: db "Starting up - Ichi...", 13, 10, 0
press_any_key_msg: db "Press any key to continue...", 13, 10, 0
error_reading_disk_msg: db "Error: failed to read disk with 0x13/ah=0x42", 13, 10, 0

times 446 - ($ - $$) db 0   ; pad up to partition table

db 0x80              ; Boot flag: 0x80 = bootable
db 0x00, 0x02, 0x00  ; dummy
db 0x0B              ; fat32 type
db 0xFF, 0xFF, 0xFF  ; dummy
dd 2048              ; start lba
dd 20480             ; size in sectors ~ 10MB