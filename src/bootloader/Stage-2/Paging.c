#include "Paging.h"

#include "Memory.h"
#include "memdefs.h"
#include "x86.h"
#include "IO.h"

uint32_t* pageDirectory = PAGE_DIRECTORY_PHYSICAL_ADDRESS;
uint32_t* pageTableArray = PAGE_TABLE_ARRAY_PHYSICAL_ADDRESS;

void initialisePages(uint64_t totalMemory)
{
    printf("Setting Up Identity Paging...  ");

    // maximum memory in 32-bit space
    for(uint32_t page_index = 0; page_index < (((uint64_t)UINT32_MAX + 1) / PAGE_SIZE); page_index += 1)
    {
        uint32_t pagePhysicalAddress = page_index * PAGE_SIZE;

        uint32_t indexPD = (pagePhysicalAddress) >> 22;
        uint32_t indexPT = (pagePhysicalAddress >> 12) & 0x03FF;

        uint32_t* pageTable = (uint32_t*)((char*)pageTableArray + (PAGE_SIZE * indexPD));

        uint32_t flags = PAGE_RW | PAGE_PRESENT;

        // set PD entry to PT physical address
        // PT physical address must be 4KiB aligned
        // mark as present
        pageDirectory[indexPD] = ((uint32_t)pageTable) | (flags & 0xFFF) | 0x01;

        // set PT entry to page address
        pageTable[indexPT] = pagePhysicalAddress | (flags & 0xFFF) | 0x01;

        //printf("PDE[%u]: %8x => PTE[%u] : %8x\n", indexPD, pageDirectory[indexPD], indexPT, pageTable[indexPT]);
    }
    
    x86_set_page_directory(pageDirectory);

    printf("ok\n");
}

// maps page at virtual address to page at physical address
void mapVirtualAddress(uint32_t vaddress, uint32_t paddress, uint32_t flags)
{
    // align adresses to 4KiB
    vaddress &= 0xFFFFF000;
    paddress &= 0xFFFFF000;

    uint32_t indexPD = (vaddress) >> 22;
    uint32_t indexPT = (vaddress >> 12) & 0x03FF;

    uint32_t* pageTable = (uint32_t*)((char*)pageTableArray + (PAGE_SIZE * indexPD));

    // set PD entry to PT physical address
    // PT physical address must be 4KiB aligned
    // mark as present
    pageDirectory[indexPD] = ((uint32_t)pageTable) | (flags & 0xFFF) | 0x01;

    // set PT entry to page address
    pageTable[indexPT] = paddress | (flags & 0xFFF) | 0x01;

    // flush TLB to load the new mapping
    x86_flushTLB();
}

// maps n pages at vaddress to n pages at paddress
void mapVirtualPages(uint32_t vaddress, uint32_t paddress, uint32_t pages, uint32_t flags)
{
    // align adresses to 4KiB
    vaddress &= 0xFFFFF000;
    paddress &= 0xFFFFF000;

    for(uint32_t page_index = 0; page_index < pages; page_index += 1)
    {
        uint32_t pageVirtualAddress = vaddress + page_index * PAGE_SIZE;
        uint32_t pagePhysicalAddress = paddress + page_index * PAGE_SIZE;

        uint32_t indexPD = (pageVirtualAddress) >> 22;
        uint32_t indexPT = (pageVirtualAddress >> 12) & 0x03FF;

        uint32_t* pageTable = (uint32_t*)((char*)pageTableArray + (PAGE_SIZE * indexPD));

        // set PD entry to PT physical address
        // PT physical address must be 4KiB aligned
        // mark as present
        pageDirectory[indexPD] = ((uint32_t)pageTable) | (flags & 0xFFF) | 0x01;

        // set PT entry to page address
        pageTable[indexPT] = pagePhysicalAddress | (flags & 0xFFF) | 0x01;

        //printf("PDE[%u]: %8x => PTE[%u] : %8x\n", indexPD, pageDirectory[indexPD], indexPT, pageTable[indexPT]);
    }

    // flush TLB to load the new mapping
    x86_flushTLB();
}

