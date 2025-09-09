#include "string.h"

size_t strlen(const char *a)
{
    size_t size = 0;
    while(a[size] != 0) size++;
    return size;
}

bool strcmp(const char *a, const char *b, u32 length)
{
    for(u32 i = 0; i < length; i++)
    {
        if(a[i] != b[i])
        {
            return false;
        }
    }

    return true;
}

