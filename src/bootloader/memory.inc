struc multiboot_info
	.flags				resd	1
	.memoryLo			resd	1
	.memoryHi			resd	1
	.bootDevice			resd	1
	.cmdLine			resd	1
	.mods_count			resd	1
	.mods_addr			resd	1
	.syms0				resd	1
	.syms1				resd	1
	.syms2				resd	1
	.mmap_length		resd	1
	.mmap_addr			resd	1
	.drives_length		resd	1
	.drives_addr		resd	1
	.config_table		resd	1
	.bootloader_name	resd	1
	.apm_table			resd	1
	.vbe_control_info	resd	1
	.vbe_mode_info		resd	1
	.vbe_mode			resw	1
	.vbe_interface_seg	resw	1
	.vbe_interface_off	resw	1
	.vbe_interface_len	resw	1
endstruc

;--------------------------------------------
;	Memory map entry structure
;--------------------------------------------

struc	MemoryMapEntry
	.baseAddress		resq	1
	.length				resq	1
	.type				resd	1
	.acpi_null			resd	1
endstruc

;---------------------------------------------
;	Get memory map from bios
;	/in es:di->destination buffer for entries
;	/in es:si->destination for count of enteries (dword)
;---------------------------------------------

BiosGetMemoryMap:
	pushad
	xor		ebx, ebx
	mov dword [es:si], 0

	mov		edx, 'PAMS'								; 'SMAP'
	mov		eax, 0xe820
	mov		ecx, 24									; memory map entry struct is 24 bytes
	int		0x15									; get first entry
	jc		.error	
	cmp		eax, 'PAMS'								; bios returns SMAP in eax
	jne		.error
	test	ebx, ebx								; if ebx=0 then list is one entry long; bail out
	je		.error
	jmp		.start
.next_entry:
	mov		edx, 'PAMS'								; some bios's trash this register
	mov		ecx, 24									; entry is 24 bytes
	mov		eax, 0xe820
	int		0x15									; get next entry
.start:
	jcxz	.skip_entry								; if actual returned bytes is 0, skip entry
.notext:
	mov		ecx, [es:di + MemoryMapEntry.length]	; get length (low dword)
	test	ecx, ecx								; if length is 0 skip it
	jne		short .good_entry
	mov		ecx, [es:di + MemoryMapEntry.length + 4]; get length (upper dword)
	jecxz	.skip_entry								; if length is 0 skip it
.good_entry:
	mov eax, [es:si]
	inc eax
	mov [es:si], eax

	add		di, 24									; point di to next entry in buffer
.skip_entry:
	cmp		ebx, 0									; if ebx return is 0, list is done
	jne		.next_entry								; get next entry
	jmp		.done
.error:
	stc
.done:
	popad
	ret

;---------------------------------------------
;	Get memory size for >64M configuations
;	ret\ ax=KB between 1MB and 16MB
;	ret\ bx=number of 64K blocks above 16MB
;	ret\ bx=0 and ax= -1 on error
;---------------------------------------------

BiosGetMemorySize64MB:
	push	ecx
	push	edx
	xor		ecx, ecx
	xor		edx, edx
	mov		ax, 0xe801
	int		0x15	
	jc		.error
	cmp		ah, 0x86		;unsupported function
	je		.error
	cmp		ah, 0x80		;invalid command
	je		.error
	jcxz	.use_ax			;bios may have stored it in ax,bx or cx,dx. test if cx is 0
	mov		ax, cx			;its not, so it should contain mem size; store it
	mov		bx, dx

.use_ax:
	pop		edx				;mem size is in ax and bx already, return it
	pop		ecx
	ret

.error:
	mov		ax, -1
	mov		bx, 0
	pop		edx
	pop		ecx
	ret
