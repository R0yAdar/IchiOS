global gdtReloadSegments
gdtReloadSegments:
   push 0x08
   lea rax, [rel .reloadCS]
   push rax
   retfq
.reloadCS:
   mov ax, 0x10 ; 0x10 = data segment
   mov ds, ax
   mov es, ax
   mov fs, ax
   mov gs, ax
   mov ss, ax
   ret