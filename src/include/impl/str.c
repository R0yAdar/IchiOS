#include "str.h"

int strlen(const char* text) {
    int i = 0;

    while(text[i++] != 0){}

    return i - 1;
}

const char* int_to_str(unsigned long long value){
    static char buffer[320];
    char pos = 0;
    char len;

    while (value > 0)
    {
        buffer[pos++] = '0' + value % 10;
        value /= 10;
    }

    if (pos == 0){
        buffer[pos++] = '0';
    }

    buffer[pos] = 0;

    len = --pos;
    
    while (pos > len / 2) {
        char temp = buffer[len - pos];
        buffer[len - pos] = buffer[pos];
        buffer[pos] = temp;
        --pos;
    }

    return buffer;
}

const char* int_to_hex(unsigned long long value){ 
    static char buffer[20];
    buffer[0] = '0';
    buffer[1] = 'x';

    char* number = buffer + 2;

    char pos = 0;
    char len;

    while (value > 0)
    {
        char digit = value % 16;
        if (digit <= 9) {
            number[pos++] = '0' + digit;
        }
        else {
            number[pos++] = 'A' + (digit - 10);
        }

        value /= 16;
    }

    if (pos == 0){
        number[pos++] = '0';
    }

    number[pos] = 0;

    len = --pos;
    
    while (pos > len / 2) {
        char temp = number[len - pos];
        number[len - pos] = number[pos];
        number[pos] = temp;
        --pos;
    }

    return buffer;   
}