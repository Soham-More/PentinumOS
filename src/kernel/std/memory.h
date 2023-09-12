#pragma once
#include <includes.h>
#include <System/Boot.h>

void memcpy(const void* src, void* dst, size_t size);
void memset(void* src, uint8_t value, size_t size);

void mem_init(KernelInfo& kernelInfo);

void* alloc_page();
void* alloc_pages(size_t count);
// allocate more pages, preferably without changing the pointer
void* realloc_pages(void* pointer, size_t prev, size_t count);

void* alloc_pages_aligned(size_t align, size_t count);

void* alloc_io_space_pages(size_t align, size_t count);

// allocate a new location and copy data there
void* alloc_cp(const void* data, size_t size);

void free_page(void* mem);
void free_pages(void* mem, size_t count);
