// struct to allocate and manage pages
// provides different page allocators based on the buddy allocator and the heap allocator
#pragma once

#include <includes.h>
#include <utils/heap.h>
#include <arch/paging/paging.h>

typedef struct page_alloc_info_t {
    void* memory;
    usize count;
    struct page_alloc_info_t* next;
} page_alloc_info_t;

typedef struct page_mgr_ctx_t {
    heap_allocator_t* heap_allocator;
    page_alloc_info_t* alloc_pages_head;
    page_alloc_info_t* alloc_pages_tail;
    x86_mmu_map_t ptable;
    usize page_count;
    usize ram_page_count;
} page_mgr_ctx_t;

// construct a page allocator context using the heap allocator
page_mgr_ctx_t construct_page_mgr_ctx(heap_allocator_t* heap_allocator);

// allocate pages & map them to a virtual address
err_t pmgr_alloc_pages(page_mgr_ctx_t* ctx, ptr_t vaddress, usize page_cnt, u32 flags);

// allocate unmapped pages & map them to a virtual address
err_t pmgr_alloc_unmapped_pages(page_mgr_ctx_t* ctx, ptr_t vaddress, usize page_cnt, u32 map_flags, u32 page_flags);

// destroy the page allocator context and free all allocated pages
err_t destroy_page_mgr_ctx(page_mgr_ctx_t* ctx);
