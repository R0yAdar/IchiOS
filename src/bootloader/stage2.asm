global stage2_start
section .stage2

[bits 16]

%include "src/bootloader/gdt32.inc"
%include "src/bootloader/gdt64.inc"
%include "src/bootloader/memory.inc"
%include "src/bootloader/vbe.inc"

boot_info:
istruc multiboot_info
	at multiboot_info.flags,			dd 0
	at multiboot_info.memoryLo,			dd 0
	at multiboot_info.memoryHi,			dd 0
	at multiboot_info.bootDevice,		dd 0
	at multiboot_info.cmdLine,			dd 0
	at multiboot_info.mods_count,		dd 0
	at multiboot_info.mods_addr,		dd 0
	at multiboot_info.syms0,			dd 0
	at multiboot_info.syms1,			dd 0
	at multiboot_info.syms2,			dd 0
	at multiboot_info.mmap_length,		dd 0
	at multiboot_info.mmap_addr,		dd 0
	at multiboot_info.drives_length,	dd 0
	at multiboot_info.drives_addr,		dd 0
	at multiboot_info.config_table,		dd 0
	at multiboot_info.bootloader_name,	dd 0
	at multiboot_info.apm_table,		dd 0
	at multiboot_info.vbe_control_info,	dd 0
	at multiboot_info.vbe_mode_info,	dd 0
	at multiboot_info.vbe_interface_seg,dw 0
	at multiboot_info.vbe_interface_off,dw 0
	at multiboot_info.vbe_interface_len,dw 0
iend

stage2_start:
mov bx, stage2_msg
call print_string

;; load gdt
cli ; disable hardware interrupts 
lgdt [gdt32_pseudo_descriptor]

xor		eax, eax
xor		ebx, ebx

call	BiosGetMemorySize64MB

mov	dword [multiboot_info.memoryHi], ebx
mov	dword [multiboot_info.memoryLo], eax

mov		eax, 0x0
mov		es, ax

mov		di, MMAP_ADDRESS
mov 	si, multiboot_info.mmap_length
call	BiosGetMemoryMap

mov dword [multiboot_info.mmap_addr], MMAP_ADDRESS

mov dword [multiboot_info.vbe_mode_info], vbe_mode_info

call VbeInit

; set protected mode

mov eax, cr0
or eax, 1
mov cr0, eax

jmp CODE_SEG32:start_prot_mode


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

[bits 32]

start_prot_mode:

mov esp, PROT32_STACK_TOP

mov ax, DATA_SEG32
mov ds, ax
mov ss, ax
mov es, ax
mov fs, ax
mov gs, ax

mov bx, stage2_protected_msg
call print_string32

mov ebx, START_OF_PAGE_TABLES
call build_page_table
mov cr3, ebx

; enable physical address extension
mov eax, cr4
or eax, 1 << 5
mov cr4, eax


;; The EFER (Extended Feature Enable Register) MSR (Model-Specific Register) contains fields
;; related to IA-32e mode operation. Bit 8 if this MSR is the LME (long mode enable) flag
;; that enables IA-32e operation.
mov ecx, 0xc0000080
rdmsr
or eax, 1 << 8
wrmsr

; enable paging
mov eax, cr0
or eax, 1 << 31
mov cr0, eax

mov ebx, comp_mode_msg

call print_string32

call enable_a20

mov ebx, a20_msg
call print_string32

lgdt [gdt64_pseudo_descriptor]

mov ebx, long_mode_msg
call print_string32

jmp CODE_SEG64:start_long_mode

end:
	hlt
	jmp end	

	; wait for input buffer to be clear

enable_a20:
	pusha

        call    wait_input
        mov     al,0xAD
        out     0x64,al		; disable keyboard
        call    wait_input

        mov     al,0xD0
        out     0x64,al		; tell controller to read output port
        call    wait_output

        in      al,0x60
        push    eax		; get output port data and store it
        call    wait_input

        mov     al,0xD1
        out     0x64,al		; tell controller to write output port
        call    wait_input

        pop     eax
        or      al,2		; set bit 1 (enable a20)
        out     0x60,al		; write out data back to the output port

        call    wait_input
        mov     al,0xAE		; enable keyboard
        out     0x64,al

        call    wait_input
	popa
        ret


wait_input:
        in      al,0x64
        test    al,2
        jnz     wait_input
        ret

	; wait for output buffer to be clear

wait_output:
        in      al,0x64
        test    al,1
        jz      wait_output
        ret

print_string32:
	pusha
	VGA_BUF equ 0xb8000
	WB_COLOR equ 0xf
	
	mov edx, VGA_BUF
	
print_string32_loop:
	; every VGA CELL has one byte for character and one for color

	cmp byte [ebx], 0
	je print_string32_end
	
	mov al, [ebx]
	mov ah, WB_COLOR
	mov [edx], ax
	
	inc ebx
	add edx, 2
	
	jmp print_string32_loop
	
print_string32_end:
	popa
	ret

build_page_table:
	pusha
	
	PAGE64_PAGE_SIZE equ 0x1000
	PAGE64_TAB_SIZE equ 0x1000
	PAGE64_TAB_ENT_NUM equ 512
	PAGE64_ENTRY_SIZE equ 8
	
	
    ;; Initialize all four tables to 0. If the present flag is cleared, all other bits in any
    ;; entry are ignored. So by filling all entries with zeros, they are all "not present".
    ;; Each repetition zeros four bytes at once. That's why a number of repetitions equal to
    ;; the size of a single page table is enough to zero all four tables.
	
	mov ecx, PAGE64_TAB_SIZE ; ecx stores the number of repetitions
    mov edi, ebx             ; edi stores the base address
    xor eax, eax             ; eax stores the value
    rep stosd
	
	mov edi, ebx ; start of table
	; hackish lea
	lea eax, [edi + (PAGE64_TAB_SIZE | 11b)] ; set r/w & present flags
	
	mov dword [edi], eax
	mov dword [edi + 0x1FF * PAGE64_ENTRY_SIZE], eax
	
	add edi, PAGE64_TAB_SIZE
	add eax, PAGE64_TAB_SIZE ; we put the next table right after
	mov dword [edi], eax 
	mov dword [edi + 0x1FE * PAGE64_ENTRY_SIZE], eax
	
	add edi, PAGE64_TAB_SIZE
	add eax, PAGE64_TAB_SIZE ; we put the next table right after
	mov dword [edi], eax 
	
	; the lowest table
	add edi, PAGE64_TAB_SIZE
	mov ebx, 11b
	mov ecx, PAGE64_TAB_ENT_NUM
	
	set_page_table_entry:
		mov dword [edi], ebx
		add ebx, PAGE64_PAGE_SIZE
		
		add edi, PAGE64_ENTRY_SIZE
		loop set_page_table_entry
	popa
	ret


[bits 64]
start_long_mode:
	mov rsp, stack_top
	and rsp, 0xFFFFFFFFFFFFFFF0  ; align to 16 bytes

	mov rdi, MMAP_ADDRESS

	; resolved when linking with kernel.c
	extern _start_kernel
	mov rdi, [boot_info]

	mov rax, _start_kernel
	call rax
end64:
	hlt
	jmp end64


START_OF_PAGE_TABLES equ 0x1000 ; goes until 0x5000 (0x1000 per tables)
MMAP_ADDRESS equ 0x5000
PROT32_STACK_TOP equ 0x6000

stage2_msg: db "Hello from stage 2", 13, 10, 0
stage2_protected_msg: db "Hello from protected mode!", 13, 10, 0
a20_msg: db "Enabled A20 line!", 13, 10, 0
long_mode_msg: db "Ichi jumps to long mode", 13, 10, 0
comp_mode_msg: db "Ichi entered 64 bit compatability mode", 0

section .bss
align 16
stack_bottom: resb 0x4000    ; 16KB
stack_top: