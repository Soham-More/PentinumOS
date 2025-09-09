#pragma once

#include <includes.h>

namespace std
{
    void initialize_heap();

    void* malloc_aligned(u32 req_size, u32 align);
    void* malloc(u32 req_size);

    void* realloc(void* prev_ptr, u32 new_req_size);

    void log_heap_status();

    void free(void* ptr);
}

#ifndef _novscode
#include <cstddef>
#endif

// global new operator
inline void* operator new(unsigned long size) noexcept {
    return std::malloc(size);
}

// global delete operator
inline void operator delete(void* ptr) noexcept {
    std::free(ptr);
}

inline void operator delete(void* ptr, size_t size) noexcept {
    std::free(ptr);
}

// For arrays
inline void* operator new[](unsigned long size) noexcept {
    return std::malloc(size);
}

inline void operator delete[](void* ptr) noexcept {
    std::free(ptr);
}

inline void operator delete[](void* ptr, unsigned long size) noexcept {
    std::free(ptr);
}

// placement new and deletes
inline void* operator new(unsigned long, void* ptr) noexcept { return ptr; }
inline void operator delete(void*, void*) noexcept { }
