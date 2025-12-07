#include "print.h"
#include "vga.h"
#include "str.h"

long long row = 0;
long long position = 0;
char color = 0x09;

void print(const char* text) {
    vga_text_input input = {position, row, text, color};
    vga_put(&input);
    position += strlen(text);
}

void printi(unsigned long long number) {
    const char* number_as_text = int_to_str(number);
    vga_text_input input = {position, row, number_as_text, color};
    vga_put(&input);
    position += strlen(number_as_text);
}

void printx(unsigned long long number) {
    const char* number_as_text = int_to_hex(number);
    vga_text_input input = {position, row, number_as_text, color};
    vga_put(&input);
    position += strlen(number_as_text);
}

void println(const char* text) {
    print(text);
    row = (row + 1) % VGA_ROWS_NUM;
    position = 0;
}

void printiln(unsigned long long number) {
    printi(number);
    row = (row + 1) % VGA_ROWS_NUM;
    position = 0;
}

void printxln(unsigned long long number) {
    printx(number);
    row = (row + 1) % VGA_ROWS_NUM;
    position = 0;
}