[bits 64]
align 16

%macro pushall 0
    push rax
    push rcx
    push rdx
    push rsi
    push rdi
    push r8
    push r9
    push r10
    push r11
%endmacro

%macro popall 0
    pop r11
    pop r10
    pop r9
    pop r8
    pop rdi
    pop rsi
    pop rdx
    pop rcx
    pop rax
%endmacro

%macro define_isr_handler 2
    global %1
    %1:
    cli

    pushall

    cld

    mov rdi, rax
    mov rsi, rcx

    extern %2
    call %2

    popall
    sti

    iretq
%endmacro

%macro define_irq_handler 2
    global %1
    %1:
    cli

    pushall

    cld

    extern %2
    call %2

    ; Signal EOI (End of IRQ)
    mov al, 0x20
    out 0x20, al

    popall
    sti

    iretq
%endmacro

%macro define_exception_handler 3
    global %1
    %1:
    cli

    mov rax, %3

    pushall

    cld

    mov rdi, rax
    mov rsi, rcx

    extern %2
    call %2

    popall
    sti

    iretq
%endmacro

; ISRs for Exceptions
define_exception_handler isr0_handler,  general_exception_handler,  0
define_exception_handler isr1_handler,  general_exception_handler,  1
define_exception_handler isr2_handler,  general_exception_handler,  2
define_exception_handler isr3_handler,  general_exception_handler,  3
define_exception_handler isr4_handler,  general_exception_handler,  4
define_exception_handler isr5_handler,  general_exception_handler,  5
define_exception_handler isr6_handler,  general_exception_handler,  6
define_exception_handler isr7_handler,  general_exception_handler,  7
define_exception_handler isr8_handler,  general_exception_handler,  8
define_exception_handler isr9_handler,  general_exception_handler,  9
define_exception_handler isr10_handler, general_exception_handler, 10
define_exception_handler isr11_handler, general_exception_handler, 11
define_exception_handler isr12_handler, general_exception_handler, 12
define_exception_handler isr13_handler, general_exception_handler, 13
define_exception_handler isr14_handler, general_exception_handler, 14
define_exception_handler isr15_handler, general_exception_handler, 15
define_exception_handler isr16_handler, general_exception_handler, 16
define_exception_handler isr17_handler, general_exception_handler, 17
define_exception_handler isr18_handler, general_exception_handler, 18
define_exception_handler isr19_handler, general_exception_handler, 19
define_exception_handler isr20_handler, general_exception_handler, 20
define_exception_handler isr21_handler, general_exception_handler, 21
define_exception_handler isr22_handler, general_exception_handler, 22
define_exception_handler isr23_handler, general_exception_handler, 23
define_exception_handler isr24_handler, general_exception_handler, 24
define_exception_handler isr25_handler, general_exception_handler, 25
define_exception_handler isr26_handler, general_exception_handler, 26
define_exception_handler isr27_handler, general_exception_handler, 27
define_exception_handler isr28_handler, general_exception_handler, 28
define_exception_handler isr29_handler, general_exception_handler, 29
define_exception_handler isr30_handler, general_exception_handler, 30
define_exception_handler isr31_handler, general_exception_handler, 31


; External Devices ISRs (PIC)
define_irq_handler isr32_handler, pit_irq_handler
define_irq_handler isr33_handler, kybrd_irq_handler

; Custom ISRs
define_isr_handler isr80_handler, syscall_handler

; Opaque

define_isr_handler isr_opaque_handler, opaque_handler
