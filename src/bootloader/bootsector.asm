[bits 16]

section .boot_sector

global __start

__start:
	mov [drive_number], dl
	
	mov bx, startup_message
	call print_string
	
	mov si, disk_address_packet
	call read_disk_sectors
	
	
end:
	hlt
	jmp end
	
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


read_disk_sectors:
	mov ah, 0x42 ; "extended read"
	mov dl, [drive_number]
	int 0x13
	
	jc handle_disk_error

ignore_disk_error:
	;SECOND_STAGE_ADDRESS equ 0x7c00 + 512
	extern stage2_start
	jmp 0:stage2_start

handle_disk_error:
	cmp word [dap_sectors_num], SECTORS_LOAD_COUNT
	jle ignore_disk_error
	
	mov bx, error_reading_disk_msg
	call print_string
	

	
	
	align 4
disk_address_packet:
	db 0x10 ; size of struct
	db 0 ; unused
dap_sectors_num:
	dw SECTORS_LOAD_COUNT ; number of sectors to read
	dd 0x7c00 + 512 ; right after our bootsector
	dq 1 ; sector to start at (skip the bootsector)


drive_number: db 0
startup_message: db "Starting up - Ichi...", 0
error_reading_disk_msg: db "Error: failed to read disk with 0x13/ah=0x42", 13, 10, 0


SECTORS_LOAD_COUNT equ 128