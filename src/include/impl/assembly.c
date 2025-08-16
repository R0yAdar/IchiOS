#include "assembly.h"

uint64_t read_cr3() {
    uint64_t value;

    asm volatile (
        "mov %%cr3, %0"
        : "=r" (value)
    );
    
    return value;
}

void write_cr3(uint64_t value) {
    asm volatile (
        "mov %0, %%cr3" 
        : : "r"(value) 
        : "memory");
}

void sti() {
    	asm volatile ("sti" ::: "memory");
}

void hlt() {
    asm volatile ("hlt" ::: "memory");
}

void set_rsp(void* stack_top) {
    	asm volatile (
        "mov %0, %%rsp" 
        : : "r"(stack_top) 
        : "memory");
}

void load_gdtr(void* gdtr) {
        asm volatile (
        "lgdt (%0)"
        :
        : "r"(gdtr)
        : "memory"
    );
}

char port_inb(char port){
    char result;
    asm volatile ("in %%dx, %%al" : "=a" (result) : "d" (port));
    return result;
}


void port_outb(char port, char data){
    asm volatile ("out %%al, %%dx" : : "a" (data), "d" (port));
}

void load_idtr(idtr idtr) {
    asm volatile (
        "lidt %0"
        :
        : "m"(idtr)
    );
}

void interrupt80(uint64_t arg1, void* arg2)
{
    asm volatile( "int $0x80" :: "a"(arg1), "c"(arg2) : "memory" );
}
