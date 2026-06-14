#pragma once

#include <includes.h>
#include <handoff/handoff.h>
#include "../defs.h"

#define X86_PAGE_PRESENT 0x01
#define X86_PAGE_RW      0x02
#define X86_PAGE_USER    0x04

#define X86_PAGE_DISABLE_CACHING 0x10
#define X86_PAGE_WRITETHROUGH 0x08

typedef struct x86_mmu_map_t {
    page_allocator_t page_allocator;
    u32* directory;
    u32* tables;
} x86_mmu_map_t;

// makes an empty pagetable with maps
x86_mmu_map_t x86_construct_pagetable(page_allocator_t page_allocator);
x86_mmu_map_t x86_from_handoff(PagingInfo* pagingInfo);

// maps n pages at vaddress to n pages at paddress
err_t x86_map_pages(x86_mmu_map_t* map, u32 vaddress, u32 paddress, u32 pages, u32 flags);
err_t x86_set_flags_pages(x86_mmu_map_t* map, u32 vaddress, u32 pages, u32 flags);

u32 x86_get_flags(x86_mmu_map_t* map, ptr_t vaddress);
u32 x86_get_map  (x86_mmu_map_t* map, ptr_t vaddress);

// get the current pagetable pointer
u32 x86_get_mmu_ctx();

// loads a map
void x86_load_mmu_map(x86_mmu_map_t* map);
// flushes the TLB cache
void x86_refresh_mmu_map();
