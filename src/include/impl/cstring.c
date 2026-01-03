#include "cstring.h"

// can remove i to optimize
void* memset(void* dest, uint8_t value, uint64_t count) {
    for(uint64_t i = 0; i < count; ++i) {
        *((char*)(dest) + i) = value;
    }

    return dest;
}

int int_to_text(unsigned long long value, char* buffer){
    uint8_t pos = 0;
    uint8_t len;

    while (value > 0)
    {
        buffer[pos++] = '0' + value % 10;
        value /= 10;
    }

    if (pos == 0){
        buffer[pos++] = '0';
    }

    len = --pos;
    
    while (pos > len / 2) {
        char temp = buffer[len - pos];
        buffer[len - pos] = buffer[pos];
        buffer[pos] = temp;
        --pos;
    }

    return len + 1;
}

// https://wiki.osdev.org/User:A22347/Printf
int vsprintf(char* s, const char* format, va_list arg) {
    while (format[0] != '\0')
    {
        if (format[0] == '%')
        {
            format++;

            if (format[0] == '%')
            {
                s[0] = '%';
                ++s;
                ++format;
            }
            else if (format[0] == 'c')
            {
                s[0] = (char)va_arg(arg, int);
                ++s;
                ++format;
            }
            else if (format[0] == 's')
            {
                char* str = va_arg(arg, char*);
                while (*str != '\0')
                {
                    *s = *str;
                    ++s;
                    ++str;
                }
                ++format;
            }
            else if (format[0] == 'd')
            {
                int number = va_arg(arg, int);
                s += int_to_text(number, s);

                ++format;
            }
        }
        else
        {
            *s = *format;
            ++s;
            ++format;
        }
    }

    *s = '\0';

    return 0;
}