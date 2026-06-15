#pragma once

#include <includes.h>
#include <handoff/handoff.h>
#include "priv.h"

typedef struct heap_allocator_t heap_allocator_t;
typedef struct page_alloc_info_t {
    void* memory;
    usize count;
    err_t error;
    u32   mapping;
} page_alloc_info_t;

// heap allocator

// use a global allocator to make an heap_allocator object
heap_allocator_t* initialize_heap(heap_allocator_t* global_allocator, u8* pool, usize pool_size_bytes);
// use pre-existing pool of memory to make an heap_allocator object
heap_allocator_t* construct_heap(void* memory, u8* pool, usize pool_size_bytes);
// function to get the size of heap_allocator_t
usize get_heap_allocator_bytesize();

void* malloc_aligned(heap_allocator_t* heap_allocator, u32 req_size, u32 align);
void* malloc(heap_allocator_t* heap_allocator, u32 req_size);

void* realloc(heap_allocator_t* heap_allocator, void* prev_ptr, u32 new_req_size);

void* copy2heap(heap_allocator_t* heap_allocator, void* obj, usize bytelength);
#define COPY_TO_HEAP(alloca, obj) copy2heap(alloca, (void*)&(obj), sizeof(obj))

void log_heap_status(heap_allocator_t* heap_allocator);

void free(heap_allocator_t* heap_allocator, void* ptr);

// a buddy allocator for allocating multiple pages
err_t initialize_buddy_allocator(heap_allocator_t* heap_allocator, KernelInfo* kInfo);

// allocate page_cnt pages(may allocate more)
const page_alloc_info_t allocate_pages(usize page_cnt);

void log_page_allocator_status();

// free allocated pages
// will mark the pages as free and ummaped if not on ram
err_t free_pages(const page_alloc_info_t* page_info);

