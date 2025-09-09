#include "paging.hpp"

#include <arch/x86.h>

namespace cpu
{
    PagingInfo g_pagingInfo;

    void initializePaging(const PagingInfo& pagingInfo)
    {
        g_pagingInfo = pagingInfo;
    }

    // maps page at virtual address to page at physical address
    void mapVirtualAddress(u32 vaddress, u32 paddress, u32 flags)
    {
        // align adresses to 4KiB
        vaddress &= 0xFFFFF000;
        paddress &= 0xFFFFF000;

        u32 indexPD = (vaddress) >> 22;
        u32 indexPT = (vaddress >> 12) & 0x03FF;

        u32* pageTable = (u32*)((char*)g_pagingInfo.pageTableArray + (PAGE_SIZE * indexPD));

        // set PD entry to PT physical address
        // PT physical address must be 4KiB aligned
        // mark as present
        g_pagingInfo.pageDirectory[indexPD] = ((u32)pageTable) | (flags & 0xFFF) | 0x01;

        // set PT entry to page address
        pageTable[indexPT] = paddress | (flags & 0xFFF) | 0x01;

        // flush TLB to load the new mapping
        x86_flushTLB();
    }

    // maps n pages at vaddress to n pages at paddress
    void mapVirtualPages(u32 vaddress, u32 paddress, u32 pages, u32 flags)
    {
        // align adresses to 4KiB
        vaddress &= 0xFFFFF000;
        paddress &= 0xFFFFF000;

        for(u32 page_index = 0; page_index < pages; page_index += 1)
        {
            u32 pageVirtualAddress = vaddress + page_index * PAGE_SIZE;
            u32 pagePhysicalAddress = paddress + page_index * PAGE_SIZE;

            u32 indexPD = (pageVirtualAddress) >> 22;
            u32 indexPT = (pageVirtualAddress >> 12) & 0x03FF;

            u32* pageTable = (u32*)((char*)g_pagingInfo.pageTableArray + (PAGE_SIZE * indexPD));

            // set PD entry to PT physical address
            // PT physical address must be 4KiB aligned
            // mark as present
            g_pagingInfo.pageDirectory[indexPD] = ((u32)pageTable) | (flags & 0xFFF) | 0x01;

            // set PT entry to page address
            pageTable[indexPT] = pagePhysicalAddress | (flags & 0xFFF) | 0x01;
        }

        // flush TLB to load the new mapping
        x86_flushTLB();
    }

    // convert a pointer to it's physical location
    ptr_t getPhysicalLocation(void* vaddress)
    {
        ptr_t vaddr = ptr_cast(vaddress);

        // align adresses to 4KiB
        ptr_t vaddrPage = vaddr & 0xFFFFF000;

        ptr_t indexPD = (vaddrPage) >> 22;
        ptr_t indexPT = (vaddrPage >> 12) & 0x03FF;

        ptr_t* pageTable = (ptr_t*)((char*)g_pagingInfo.pageTableArray + (PAGE_SIZE * indexPD));

        // get PT entry
        ptr_t paddrPage = pageTable[indexPT] & 0xFFFFF000;

        return paddrPage + (vaddr & 0xFFF);
    }

    // sets flags of page
    void setFlagsPage(u32 vaddress, u32 flags)
    {
        // align adresses to 4KiB
        vaddress &= 0xFFFFF000;

        u32 indexPD = (vaddress) >> 22;
        u32 indexPT = (vaddress >> 12) & 0x03FF;

        u32* pageTable = (u32*)((char*)g_pagingInfo.pageTableArray + (PAGE_SIZE * indexPD));

        // get address stored in page table
        u32 pageTableEntryAddr = pageTable[indexPT] & ~(0xFFF);

        // set PT Entry flags
        pageTable[indexPT] = pageTableEntryAddr | (flags & 0xFFF);

        // flush TLB to load the new mapping
        x86_flushTLB();
    }

    // gets flags of page
    u32 getFlagsPage(u32 vaddress)
    {
        // align adresses to 4KiB
        vaddress &= 0xFFFFF000;

        u32 indexPD = (vaddress) >> 22;
        u32 indexPT = (vaddress >> 12) & 0x03FF;

        u32* pageTable = (u32*)((char*)g_pagingInfo.pageTableArray + (PAGE_SIZE * indexPD));

        return pageTable[indexPT] & (0xFFF);
    }

    // sets flags of pages
    void setFlagsPages(u32 vaddress, u32 flags, size_t count)
    {
        // align adresses to 4KiB
        vaddress &= 0xFFFFF000;

        for(u32 page_index = 0; page_index < count; page_index += 1)
        {
            u32 pageVirtualAddress = vaddress + page_index * PAGE_SIZE;

            u32 indexPD = (pageVirtualAddress) >> 22;
            u32 indexPT = (pageVirtualAddress >> 12) & 0x03FF;

            u32* pageTable = (u32*)((char*)g_pagingInfo.pageTableArray + (PAGE_SIZE * indexPD));

            // get address stored in page table
            u32 pageTableEntryAddr = pageTable[indexPT] & ~(0xFFF);

            // set PT flags
            pageTable[indexPT] = pageTableEntryAddr | (flags & 0xFFF) | 0x01;
        }

        // flush TLB to load the new mapping
        x86_flushTLB();
    }
}