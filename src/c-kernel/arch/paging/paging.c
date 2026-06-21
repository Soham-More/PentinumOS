#include "paging.h"

#include "../x86.h"
#include <utils/cstdlib.h>

#define X86_PD_FLAGS(map, pd_index) (map->directory[pd_index] & 0xFFF)
#define X86_PD_TABLE(map, pd_index) (u32*)(map->directory[pd_index] & 0xFFFFF000)

void initialize_pagetable(x86_mmu_map_t* map, u32* table, u32 pd_index) {
    for(u32 pt_index = 0; pt_index < X86_PAGETABLE_SIZE; pt_index++) {
        table[pt_index] = ((u32)nullptr) & ~X86_PAGE_PRESENT;
    }
    map->directory[pd_index] = ((u32)table) | (X86_PD_FLAGS(map, pd_index) & 0xFFF) | X86_PAGE_PRESENT;
}
x86_mmu_map_t x86_construct_pagetable(void* page) {
    x86_mmu_map_t map;
    map.directory = (u32*)page;

    for(u32 pd_index = 0; pd_index < X86_PAGETABLE_SIZE; pd_index++) {
        u32 flags = X86_PAGE_RW | X86_PAGE_PRESENT;

        map.directory[pd_index] = (((uint32_t)nullptr) | (flags & 0xFFF)) & ~X86_PAGE_PRESENT;
    }

    return map;
}
x86_mmu_map_t x86_from_handoff(PagingInfo* pagingInfo) {
    x86_mmu_map_t map;
    map.directory = pagingInfo->pageDirectory;
    return map;
}

// returns the number of pages that need to be allocated to map the given range
usize x86_map_pages_get_page_count(x86_mmu_map_t* map, u32 vaddress, u32 pages) {
    // align adresses to 4KiB
    vaddress &= 0xFFFFF000;

    u32  table_alloc_count = 0;
    u32  table_alloc_idx = 0;
    u32* tables = nullptr;

    for(u32 page_index = 0; page_index < pages; page_index += 1) {
        u32 pageVirtualAddress = vaddress + page_index * X86_PAGE_SIZE;

        u32 indexPD = (pageVirtualAddress) >> 22;
        u32 indexPT = (pageVirtualAddress >> 12) & 0x03FF;

        // if page table is not present - then schedule to allocate one!
        if(map->directory[indexPD] & X86_PAGE_PRESENT) continue;

        table_alloc_count++;
    }
    return table_alloc_count;
}
// maps n pages at vaddress to n pages at paddress
err_t x86_map_pages(x86_mmu_map_t* map, u32 vaddress, u32 paddress, u32 pages, u32 flags, void* alloc_pages, usize alloc_page_count) {
    // align adresses to 4KiB
    vaddress &= 0xFFFFF000;
    paddress &= 0xFFFFF000;

    u32  table_alloc_idx = 0;
    u32* tables = nullptr;

    tables = (u32*) alloc_pages;

    usize req_page_count = x86_map_pages_get_page_count(map, vaddress, pages);
    if(req_page_count > alloc_page_count) return ENOMEM;

    for(u32 page_index = 0; page_index < pages; page_index += 1) {
        u32 pageVirtualAddress = vaddress + page_index * X86_PAGE_SIZE;
        u32 pagePhysicalAddress = paddress + page_index * X86_PAGE_SIZE;

        u32 indexPD = (pageVirtualAddress) >> 22;
        u32 indexPT = (pageVirtualAddress >> 12) & 0x03FF;

        // if page table is not present - then allocate one!
        if(map->directory[indexPD] & X86_PAGE_PRESENT == 0) {
            initialize_pagetable(map, tables + (table_alloc_idx * X86_PAGETABLE_SIZE), indexPD);
            table_alloc_idx++;
        }

        u32* pageTable = (u32*)X86_PD_TABLE(map, indexPD);

        // set PD entry to PT physical address
        // PT physical address must be 4KiB aligned
        // mark as present
        map->directory[indexPD] = ((u32)pageTable) | ((X86_PD_FLAGS(map, indexPD) | flags) & 0xFFF) | X86_PAGE_PRESENT;

        // set PT entry to page address
        pageTable[indexPT] = pagePhysicalAddress | (flags & 0xFFF) | X86_PAGE_PRESENT;
    }

    return ESUCCESS;
}
// sets flags of pages
err_t x86_set_flags_pages(x86_mmu_map_t* map, u32 vaddress, u32 pages, u32 flags) {
    // align adresses to 4KiB
    vaddress &= 0xFFFFF000;

    for(u32 page_index = 0; page_index < pages; page_index += 1) {
        u32 pageVirtualAddress = vaddress + page_index * X86_PAGE_SIZE;

        u32 indexPD = (pageVirtualAddress) >> 22;
        u32 indexPT = (pageVirtualAddress >> 12) & 0x03FF;

        ptr_t* pageTable = X86_PD_TABLE(map, indexPD);
        if(pageTable == nullptr || X86_PD_FLAGS(map, indexPD) & X86_PAGE_PRESENT == 0) return ENOPAGE;

        // get address stored in page table
        u32 pageTableEntryAddr = pageTable[indexPT] & ~(0xFFF);

        // set PT flags
        pageTable[indexPT] = pageTableEntryAddr | (flags & 0xFFF) | 0x01;

        // set PD flags
        map->directory[indexPD] = ((u32)pageTable) | ((X86_PD_FLAGS(map, indexPD) | flags) & 0xFFF) | X86_PAGE_PRESENT;
    }

    return ESUCCESS;
}

// convert a pointer to it's physical location
u32 x86_get_map(x86_mmu_map_t* map, ptr_t vaddress)
{
    ptr_t vaddr = (u32)(vaddress);

    // align adresses to 4KiB
    ptr_t vaddrPage = vaddr & 0xFFFFF000;

    ptr_t indexPD = (vaddrPage) >> 22;
    ptr_t indexPT = (vaddrPage >> 12) & 0x03FF;

    // get PT
    ptr_t* pageTable = X86_PD_TABLE(map, indexPD);
    if(pageTable == nullptr || X86_PD_FLAGS(map, indexPD) & X86_PAGE_PRESENT == 0) return ENOPAGE;

    // get PT entry
    ptr_t paddrPage = pageTable[indexPT] & 0xFFFFF000;

    return paddrPage + (vaddr & 0xFFF);
}
// gets flags of page
u32 x86_get_flags(x86_mmu_map_t* map, ptr_t vaddress)
{
    // align adresses to 4KiB
    vaddress &= 0xFFFFF000;

    u32 indexPD = (vaddress) >> 22;
    u32 indexPT = (vaddress >> 12) & 0x03FF;

    // get PT
    ptr_t* pageTable = X86_PD_TABLE(map, indexPD);
    if(pageTable == nullptr || X86_PD_FLAGS(map, indexPD) & X86_PAGE_PRESENT == 0) return ENOPAGE;

    return pageTable[indexPT] & (0xFFF);
}

u32 x86_get_mmu_ctx() {
    return x86_get_cr3_register();
}

u32 x86_get_ctx_map(const x86_mmu_map_t *map) {
    return (u32)map->directory;
}

void x86_load_mmu_map(x86_mmu_map_t* map) {
    x86_set_page_directory((void*)map->directory);
}
