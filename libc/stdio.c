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
    while (*fmt && written < n - 1) {
        if (*fmt == '%') {
            fmt++;
            if (*fmt == 's') {
                char* str = va_arg(ap, char*);
                while (*str && written < n - 1) s[written++] = *str++;
            } else if (*fmt == 'd') {
                char buf[32];
                _itoa(va_arg(ap, int), buf, 10);
                for (int i = 0; buf[i] && written < n - 1; i++) s[written++] = buf[i];
            } else if (*fmt == 'x') {
                char buf[32];
                _itoa(va_arg(ap, unsigned int), buf, 16);
                for (int i = 0; buf[i] && written < n - 1; i++) s[written++] = buf[i];
            } else if (*fmt == '%') {
                s[written++] = '%';
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
        puts("ERR:");
    }

    puts(buf);
    return ret;
}

int vfprintf(FILE* stream, const char* format, va_list vlist) {
    char buf[1024];
    int ret = vsnprintf(buf, sizeof(buf), format, vlist);

    if (stream == stderr) {
        puts("ERR:");
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
    return 0;
}

FILE* fopen(const char* filename, const char* mode) {
    uint64_t fid;
    syscall_file_open(filename, &fid);
    return (FILE*)fid;
}

int fclose(FILE* stream) {
    syscall_file_close((uint64_t)stream);
    return 0;
}

size_t fread(void* ptr, size_t size, size_t nmemb, FILE* stream) {
    size_t total = size * nmemb;
    return (size_t)syscall_file_read((uint64_t)stream, ptr, total);
}

size_t fwrite(const void* ptr, size_t size, size_t nmemb, FILE* stream) {
    return 0;
}

int fseek(FILE* stream, long offset, int whence) {
    return (int)syscall_file_seek((uint64_t)stream, offset, whence);
}

long ftell(FILE* stream) {
    return (long)syscall_file_tell((uint64_t)stream);
}

int feof(FILE* stream) {
    // Return 1 if current pos == file size
    return 0;
}

int putchar(int c) {
    char b = (char)c;
    syscall_putc(b);
    return c;
}

int puts(const char* s) {
    while (*s) putchar(*s++);
    putchar('\n');
    return 0;
}

int remove(const char* filename) {
    return 0;
}

int rename(const char* old, const char* new) {
    return 0;
}

int fflush(FILE* stream) {
    return 0;
}