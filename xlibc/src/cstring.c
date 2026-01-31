#include "cstring.h"

int memcmp(const uint8_t* left, const uint8_t* right, size_t len) {
    for (size_t i = 0; i < len; i++) {
        if (left[i] != right[i]) {
            return left[i] - right[i];
        }
    }
    
    return 0;
}

void* memcpy(void* dest, void* src, uint64_t count) {
    for(uint64_t i = 0; i < count; ++i) {
        *((char*)(dest) + i) = *((char*)(src) + i);
    }

    return dest;
}

void* memset(void* dest, uint8_t value, uint64_t count) {
    for(uint64_t i = 0; i < count; ++i) {
        *((char*)(dest) + i) = value;
    }

    return dest;
}

int number_as_string(uint64_t value, char* buffer, int base) {
    uint8_t pos = 0;
    uint8_t len;

    while (value > 0)
    {
        char c = '0' + value % base;
        if (c > '9') c += 'A' - '0' - 10;
        buffer[pos++] = c;
        value /= base;
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
                uint64_t number = va_arg(arg, int64_t);
                s += number_as_string(number, s, 10);

                ++format;
            }
            else if (format[0] == 'x')
            {
                uint64_t number = va_arg(arg, uint64_t);
                s += number_as_string(number, s, 16);

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