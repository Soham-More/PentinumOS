#pragma once

#include <includes.h>

typedef struct heap_allocator_t heap_allocator_t;

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

