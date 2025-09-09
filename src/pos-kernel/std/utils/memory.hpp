#pragma once
#include <includes.h>

namespace std
{
    inline void* memcpy(const void* src, void* dst, size_t size)
    {
        const char* s = static_cast<const char*>(src);
        char* d = static_cast<char*>(dst);
        for (size_t i = 0; i < size; i++)
        {
            d[i] = s[i];
        }
        return dst;
    }
    inline void* memset(void* src, u8 value, size_t size)
    {
        u8* s = static_cast<u8*>(src);
        for (size_t i = 0; i < size; i++)
        {
            s[i] = value;
        }
        return src;
    }
}
