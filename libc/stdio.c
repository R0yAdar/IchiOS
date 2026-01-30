#include "stdio.h"
#include "stdarg.h"
#include "sysapi.h"

static char* _itoa(long value, char* str, int base) {
    char *rc, *ptr, *low;
    if (base < 2 || base > 36) return str;
    rc = ptr = str;
    if (value < 0 && base == 10) *ptr++ = '-';
    low = ptr;
    long v = (value < 0 && base == 10) ? -value : value;
    do {
        *ptr++ = "0123456789abcdefghijklmnopqrstuvwxyz"[v % base];
        v /= base;
    } while (v);
    *ptr-- = '\0';
    while (low < ptr) {
        char tmp = *low;
        *low++ = *ptr;
        *ptr-- = tmp;
    }
    return rc;
}

int vsnprintf(char* s, size_t n, const char* fmt, va_list ap) {
    size_t written = 0;
    while (*fmt && written < (n - 1)) {
        if (*fmt == '%') {
            fmt++; // move past '%'
            switch (*fmt) {
                case 's': {
                    char* str = va_arg(ap, char*);
                    if (!str) str = "<null>";
                    while (*str && written < (n - 1)) s[written++] = *str++;
                    break;
                }
                case 'd':
                case 'u':
                case 'i': {
                    char buf[32];
                    _itoa(va_arg(ap, int), buf, 10);
                    for (int i = 0; buf[i] && written < (n - 1); i++) s[written++] = buf[i];
                    break;
                }
                case 'x':
                case 'p': {
                    char buf[32];
                    _itoa(va_arg(ap, unsigned long), buf, 16);
                    for (int i = 0; buf[i] && written < (n - 1); i++) s[written++] = buf[i];
                    break;
                }
                case '%':
                    s[written++] = '%';
                    break;
                default:
                    // If we don't know the type, just print the char so we don't desync
                    s[written++] = *fmt;
                    break;
            }
        } else {
            s[written++] = *fmt;
        }
        fmt++;
    }
    s[written] = '\0';
    return (int)written;
}


int printf(const char* format, ...) {
    char buf[1024];
    va_list va;
    va_start(va, format);
    int ret = vsnprintf(buf, sizeof(buf), format, va);
    va_end(va);
    puts(buf);
    return ret;
}

int fprintf(FILE* stream, const char* format, ...) {
    char buf[1024];
    va_list va;
    va_start(va, format);
    int ret = vsnprintf(buf, sizeof(buf), format, va);
    va_end(va);

    if (stream == stderr) {
        puts("ERR1:");
    }

    puts(format);
    
    return ret;
}

int vfprintf(FILE* stream, const char* format, va_list vlist) {
    char buf[1024];
    int ret = vsnprintf(buf, sizeof(buf), format, vlist);

    if (stream == stderr) {
        puts("ERR2:");
    }

    puts(buf);
    return ret;
}

void snprintf(char *buffer, size_t len, char *fmt, ...) {
    va_list va;
    va_start(va, fmt);
    vsnprintf(buffer, len, fmt, va);
    va_end(va);
}

int sscanf(const char *str, const char *format, ...) {
    puts("Scanning");
    return 0;
}

FILE* fopen(const char* filename, const char* mode) {
    uint64_t fid;
    syscall_file_open(filename, &fid);
    printf("Opened file %d\n", fid);
    return (FILE*)fid;
}

int fclose(FILE* stream) {
    puts("Closing file");
    syscall_file_close((uint64_t)stream);
    return 0;
}

size_t fread(void* ptr, size_t size, size_t nmemb, FILE* stream) {
    size_t total = size * nmemb;
    return (size_t)syscall_file_read((uint64_t)stream, ptr, total);
}

size_t fwrite(const void* ptr, size_t size, size_t nmemb, FILE* stream) {
    puts("Writing file");
    return 0;
}

int fseek(FILE* stream, long offset, int whence) {
    return (int)syscall_file_seek((uint64_t)stream, offset, whence);
}

long ftell(FILE* stream) {
    puts("Telling file");
    return (long)syscall_file_tell((uint64_t)stream);
}

int feof(FILE* stream) {
    puts("Checking EOF");
    // Return 1 if current pos == file size
    return 0;
}

char buffer[1024] = {0};
char buffer_index = 0;

int putchar(int c) {
    buffer[buffer_index++] = c;

    if (buffer_index == 1023 || c == '\n') {
        syscall_puts(buffer);
        buffer_index = 0;
        for (size_t i = 0; i < 1024; i++)
        {
            buffer[i] = 0;
        }
    }
    
    return c;
}

int puts(const char* s) {
    while (*s) putchar(*s++);
    putchar('\n');
    return 0;
}

int remove(const char* filename) {
    puts("Removing");
    return 0;
}

int rename(const char* old, const char* new) {
    puts("Renaming");
    return 0;
}

int fflush(FILE* stream) {
    puts("Flushing stream");
    return 0;
}