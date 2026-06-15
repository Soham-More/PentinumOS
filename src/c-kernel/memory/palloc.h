// struct to allocate and manage pages
// provides different page allocators based on the buddy allocator and the heap allocator
#pragma once

#include <includes.h>
#include <memory/memory.h>

typedef struct page_alloc_ctx_t page_alloc_ctx_t;

// construct a page allocator context using the heap allocator
page_alloc_ctx_t* construct_page_alloc_ctx(heap_allocator_t* heap_allocator);

// allocate pages using the page allocator context
void* palloc_allocate(page_alloc_ctx_t* ctx, usize page_cnt);

// destroy the page allocator context and free all allocated pages
err_t destroy_page_alloc_ctx(page_alloc_ctx_t* ctx);


