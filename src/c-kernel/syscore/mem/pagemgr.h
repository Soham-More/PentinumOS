// struct to allocate and manage pages
// provides different page allocators based on the buddy allocator and the heap allocator
#pragma once

#include <includes.h>
#include <utils/heap.h>
#include <arch/paging/paging.h>

#include "priv.h"
#include "buddy.h"

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

// allocate page_cnt pages(may allocate more)
page_alloc_info_t* allocate_pages(page_mgr_ctx_t* ctx, usize page_cnt);
// allocate page_cnt pages(may allocate more) on memory not on ram
// if status is PNODE_BLK_MAPPED - then mark allocated pages as BLK_MAPPED
// if status is PNODE_IO_MAPPED - then mark allocated pages as IO_MAPPED
page_alloc_info_t* allocate_mapped_pages(page_mgr_ctx_t* ctx, u16 status, usize page_cnt);

// free allocated pages
err_t free_pages(const page_alloc_info_t* page_info);

// construct a page allocator context using the heap allocator and a pre-existing page table
page_mgr_ctx_t construct_page_mgr_ctx(heap_allocator_t* heap_allocator, x86_mmu_map_t ptable);

// allocate pages & map them to a virtual address
err_t pmgr_alloc_pages(page_mgr_ctx_t* ctx, ptr_t vaddress, usize page_cnt, u32 flags);

// allocate unmapped pages & map them to a virtual address
err_t pmgr_alloc_unmapped_pages(page_mgr_ctx_t* ctx, ptr_t vaddress, usize page_cnt, u32 map_flags, u32 page_flags);

// destroy the page allocator context and free all allocated pages
err_t destroy_page_mgr_ctx(page_mgr_ctx_t* ctx);

