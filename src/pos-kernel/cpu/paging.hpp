#pragma once

#include <includes.h>
#include "boot.hpp"

namespace cpu
{
    enum PageFlags : u32
    {
        PAGE_PRESENT = 0x01,
        PAGE_RW      = 0x02,
        PAGE_USER    = 0x04,

        PAGE_DISABLE_CACHING = 0x10,
        PAGE_WRITETHROUGH = 0x08,
    };

    void initializePaging(const PagingInfo& pagingInfo);

    // maps page at virtual address to page at physical address
    void mapVirtualAddress(u32 vaddress, u32 paddress, u32 flags);

    // maps n pages at vaddress to n pages at paddress
    void mapVirtualPages(u32 vaddress, u32 paddress, u32 pages, u32 flags);

    // convert a pointer to it's physical location
    ptr_t getPhysicalLocation(void* vaddress);

    // sets flags of page
    void setFlagsPage(u32 vaddress, u32 flags);
    // gets flags of page
    u32  getFlagsPage(u32 vaddress);

    // sets flags of multiple pages
    void setFlagsPages(u32 vaddress, u32 flags, size_t count);
}
