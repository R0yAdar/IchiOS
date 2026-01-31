#include "str.h"

int strlen(const char *text)
{
    int i = 0;

    while (text[i++] != 0)
    {
    }

    return i - 1;
}

int strcmp(const char *string1, const char *string2)
{
    size_t index = 0;

    while (string1[index] == string2[index])
    {
        if (string1[index] == '\0')
            return 0;
        ++index;
    }

    return string1[index] - string2[index];
}

const char *strstr(const char *str1, const char *str2)
{
    size_t match_index = 0;

    while (str1[match_index] != '\0')
    {
        size_t i = 0;

        while (str1[match_index + i] == str2[i])
        {
            ++i;
            if (str2[i] == '\0')
                return str1 + match_index;
        }

        ++match_index;
    }

    return NULL;
}

const char *strchr(const char *str, char character)
{
    while (*str != character && *str != '\0')
        ++str;
    return *str == '\0' ? NULL : str;
}

int strncmp(const char *str1, const char *str2, size_t num)
{
    size_t i = 0;

    while (i < num && str1[i] == str2[i] && str1[i] != '\0')
        i++;

    if (i == num)
        return 0;

    return (unsigned char)str1[i] - (unsigned char)str2[i];
}

char *strcpy(char *destination, const char *source)
{
    size_t i = 0;
    while (source[i] != '\0')
    {
        destination[i] = source[i];
        ++i;
    }
    destination[i] = '\0';
    return destination;
}