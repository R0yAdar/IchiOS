#include "ctype.h"

int toupper(int c) {
    if (c >= 'a' && c <= 'z') {
        return c - 0x20;
    }
    return c;
}

int tolower(int c) {
    if (c >= 'A' && c <= 'Z') {
        return c + 0x20;
    }
    return c;
}

int isspace(int c) {
    return (c == ' ' || c == '\t' || c == '\n' || c == '\r' || c == '\f' || c == '\v');
}