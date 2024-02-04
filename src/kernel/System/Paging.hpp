#pragma once

#include <includes.h>
#include "Boot.h"

namespace sys
{
    enum PageFlags
    {
        PAGE_PRESENT = 0x01,
        PAGE_RW      = 0x02,
        PAGE_USER    = 0x04,

        PAGE_DISABLE_CACHING = 0x10,
        PAGE_WRITETHROUGH = 0x08,
    };

    void initialisePaging(const PagingInfo& pagingInfo);

    // maps page at virtual address to page at physical address
    void mapVirtualAddress(uint32_t vaddress, uint32_t paddress, uint32_t flags);

    // maps n pages at vaddress to n pages at paddress
    void mapVirtualPages(uint32_t vaddress, uint32_t paddress, uint32_t pages, uint32_t flags);

    // convert a pointer to it's physical location
    ptr_t getPhysicalLocation(void* vaddress);

    // sets flags of page
    void setFlagsPage(uint32_t vaddress, uint32_t flags);

    // sets flags of multiple pages
    void setFlagsPages(uint32_t vaddress, uint32_t flags, size_t count);
}
