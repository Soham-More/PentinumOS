#pragma once

#include <includes.h>

namespace std
{
    void initHeap();

    void* malloc(size_t size);
    void* mallocAligned(size_t size, size_t align);

    void free(void* ptr);
}
