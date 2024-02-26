#include "string.h"
#include <std/IO/stdio.h>

void copyWideStringToString(const void* src, void* dst, size_t srcByteLength)
{
    const uint16_t* in = (const uint16_t*)src;
    uint8_t* out = (uint8_t*)dst;

    for(int i = 0; i < (srcByteLength / 2); i++)
    {
        out[i] = in[i];
    }
    out[srcByteLength / 2] = 0;
}

bool strcmp(const char* a, const char* b, uint32_t length)
{
    for(uint32_t i = 0; i < length; i++)
    {
        if(a[i] != b[i])
        {
            return false;
        }
    }

    return true;
}

char toUpper(char c)
{
    if(c >= 'a' && c <= 'z')
    {
        return c + 'A' - 'a';
    }

    return c;
}

