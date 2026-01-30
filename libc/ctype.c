#include "ctype.h"

int toupper(int c) {
    return c - 0x20;
}

int tolower(int c) {
    return c + 0x20;
}

int isspace(int c) {
    return c == ' ';
}