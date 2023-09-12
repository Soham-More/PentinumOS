#include "Paging.hpp"

#include <i686/x86.h>

namespace sys
{
    PagingInfo g_pagingInfo;

    void initialisePaging(const PagingInfo& pagingInfo)
    {
        g_pagingInfo = pagingInfo;
    }

    // maps page at virtual address to page at physical address
    void mapVirtualAddress(uint32_t vaddress, uint32_t paddress, uint32_t flags)
    {
        // align adresses to 4KiB
        vaddress &= 0xFFFFF000;
        paddress &= 0xFFFFF000;

        uint32_t indexPD = (vaddress) >> 22;
        uint32_t indexPT = (vaddress >> 12) & 0x03FF;

        uint32_t* pageTable = (uint32_t*)((char*)g_pagingInfo.pageTableArray + (PAGE_SIZE * indexPD));

        // set PD entry to PT physical address
        // PT physical address must be 4KiB aligned
        // mark as present
        g_pagingInfo.pageDirectory[indexPD] = ((uint32_t)pageTable) | (flags & 0xFFF) | 0x01;

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

            uint32_t* pageTable = (uint32_t*)((char*)g_pagingInfo.pageTableArray + (PAGE_SIZE * indexPD));

            // set PD entry to PT physical address
            // PT physical address must be 4KiB aligned
            // mark as present
            g_pagingInfo.pageDirectory[indexPD] = ((uint32_t)pageTable) | (flags & 0xFFF) | 0x01;

            // set PT entry to page address
            pageTable[indexPT] = pagePhysicalAddress | (flags & 0xFFF) | 0x01;
        }

        // flush TLB to load the new mapping
        x86_flushTLB();
    }

    // sets flags of page
    void setFlagsPage(uint32_t vaddress, uint32_t flags)
    {
        // align adresses to 4KiB
        vaddress &= 0xFFFFF000;

        uint32_t indexPD = (vaddress) >> 22;
        uint32_t indexPT = (vaddress >> 12) & 0x03FF;

        uint32_t* pageTable = (uint32_t*)((char*)g_pagingInfo.pageTableArray + (PAGE_SIZE * indexPD));

        // get address stored in page table
        uint32_t pageTableEntryAddr = pageTable[indexPT] & ~(0xFFF);

        // set PT Entry flags
        pageTable[indexPT] = pageTableEntryAddr | (flags & 0xFFF) | 0x01;

        // flush TLB to load the new mapping
        x86_flushTLB();
    }

    // sets flags of pages
    void setFlagsPages(uint32_t vaddress, uint32_t flags, size_t count)
    {
        // align adresses to 4KiB
        vaddress &= 0xFFFFF000;

        for(uint32_t page_index = 0; page_index < count; page_index += 1)
        {
            uint32_t pageVirtualAddress = vaddress + page_index * PAGE_SIZE;

            uint32_t indexPD = (pageVirtualAddress) >> 22;
            uint32_t indexPT = (pageVirtualAddress >> 12) & 0x03FF;

            uint32_t* pageTable = (uint32_t*)((char*)g_pagingInfo.pageTableArray + (PAGE_SIZE * indexPD));

            // get address stored in page table
            uint32_t pageTableEntryAddr = pageTable[indexPT] & ~(0xFFF);

            // set PT flags
            pageTable[indexPT] = pageTableEntryAddr | (flags & 0xFFF) | 0x01;
        }

        // flush TLB to load the new mapping
        x86_flushTLB();
    }
}