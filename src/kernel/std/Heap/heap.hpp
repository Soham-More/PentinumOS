#pragma once

#include <includes.h>

namespace std
{
    void initHeap();

    void* malloc(uint32_t size);
    void* mallocAligned(uint32_t size, uint8_t alignBits);

    void free(void* ptr);
}
