#pragma once

#include <std/basic.hpp>

#include "boot.hpp"

#define ALLOC_ERROR 0x1
#define ALLOC_NO_FREE_SPACE 0x2
#define ALLOC_REQ_SIZE_NAVAIL 0x4
#define ALLOC_REQ_IMPOSSIBLE 0x8

namespace cpu
{
    void initialize_memory(KernelInfo& kernel_info);

    void logMemoryInfo();

    void* alloc_page();
    void* alloc_pages(size_t count);
    void* realloc_pages(void* pointer, size_t prev_count, size_t req_count);

    void* alloc_extend(void* pointer, size_t prev_count, size_t req_count);

    // allocate a new location and copy data there
    void* alloc_cp(const void* data, size_t size);

    // allocate I/O memory pages
    void* alloc_io_pages(size_t align, size_t count);

    void free_page(void* mem);
    void free_pages(void* mem, size_t count);

    u32 getAllocatorStatus();
    void clearAllocatorStatus();
}

