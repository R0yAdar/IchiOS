[bits 64]
global processUserlandTrampoline

processUserlandTrampoline:
    mov rax, rdi

    mov bx, 0x23
    mov ds, bx
    mov es, bx
    mov fs, bx
    mov gs, bx

    push qword [rax + 152] 
    push qword [rax + 144] 
    push qword [rax + 136] 
    push qword [rax + 128] 
    push qword [rax + 120] 

    mov r15, [rax + 112]
    mov r14, [rax + 104]
    mov r13, [rax + 96]
    mov r12, [rax + 88]
    mov rbx, [rax + 80]
    mov rbp, [rax + 72]
    mov rcx, [rax + 56]
    mov rdx, [rax + 48]
    mov rsi, [rax + 40]
    mov r8,  [rax + 24]
    mov r9,  [rax + 16]
    mov r10, [rax + 8]
    mov r11, [rax + 0]

    mov rdi, [rax + 32] 
    mov rax, [rax + 64] 

    iretq