#pragma once
#include <includes.h>

void memcpy(void* src, void* dst, uint32_t size);
void memset(void* src, uint8_t value, uint32_t size);

void mem_init(void* e820_mmap, void* _kernelMap);

void* alloc_page();
void* alloc_pages(uint32_t count);

void free_page(void* mem);
void free_pages(void* mem, uint32_t count);
