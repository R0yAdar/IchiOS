#ifndef STDIO_H
#define STDIO_H

#include "stdint.h"
#include "stdarg.h"

typedef uint64_t FILE;

int printf(const char *format, ...);

int fprintf(FILE *stream, const char *format, ...);

int sprintf(char *str, const char *format, ...);

void snprintf(char *buffer, size_t len, char *fmt, ...);

int vsnprintf(char *s, size_t n, const char *fmt, va_list ap);

int vfprintf(FILE *stream, const char *format, va_list vlist);

int sscanf(const char *str, const char *format, ...);

#define stdin (FILE *)0
#define stdout (FILE *)1
#define stderr (FILE *)2
#define SEEK_SET 0
#define SEEK_CUR 1
#define SEEK_END 2

FILE *fopen(const char *filename, const char *mode);
int fclose(FILE *stream);
size_t fread(void *ptr, size_t size, size_t nmemb, FILE *stream);
int fseek(FILE *stream, long offset, int whence);
long ftell(FILE *stream);
int feof(FILE *stream);
int fflush(FILE *stream);

int putchar(int c);

int puts(const char *s);

int remove(const char *filename);

int rename(const char *old, const char *new);

#endif
