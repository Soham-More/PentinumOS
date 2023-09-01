#pragma once

#include "includes.h"

#define PAGE_RW         0x02
#define PAGE_PRESENT    0x01

// identity maps vaddr to phyaddr
void initialisePages(uint64_t totalMemory);

// maps page at virtual address to page at physical address
void mapVirtualAddress(uint32_t vaddress, uint32_t paddress, uint32_t flags);

// maps n pages at vaddress to n pages at paddress
void mapVirtualPages(uint32_t vaddress, uint32_t paddress, uint32_t pages, uint32_t flags);
