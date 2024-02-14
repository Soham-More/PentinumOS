#include "string.h"

void copyWideStringToString(void* src, void* dst, size_t srcByteLength)
{
    uint16_t* in = (uint16_t*)src;
    uint8_t* out = (uint8_t*)dst;

    for(int i = 0; i < (srcByteLength / 2); i++)
    {
        out[i] = in[i];
    }
    out[srcByteLength / 2] = 0;
}