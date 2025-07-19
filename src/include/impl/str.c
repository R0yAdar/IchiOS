#include "str.h"

const char* int_to_str(long long value){
    static char buffer[20];
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