#include "memory.h"
#include <includes.h>
#include <std/Bitmap.hpp>
#include <std/math.h>
#include <i686/Exception.h>

#define PAGE_SIZE 4096

static std::Bitmap pagesAllocated;

void memcpy(void* src, void* dst, uint32_t size)
{
    for(uint32_t i = 0; i < size; i++)
    {
        ((char*)dst)[i] = ((char*)src)[i];
    }
}
void memset(void* src, uint8_t value, uint32_t size)
{
	for(uint32_t i = 0; i < size; i++)
    {
        ((char*)src)[i] = value;
    }
}

typedef struct
{
    uint32_t sectionBegin;
    uint32_t sectionSize;
}_packed KernelMapEntry;
typedef struct
{
    uint32_t entryCount;
    KernelMapEntry* entries;
}_packed KernelMap;
typedef struct
{
	uint64_t baseAddress;
	uint64_t regionSize;
	uint32_t regionType;
	uint32_t ACPI3_eattrib_bf;
} _packed e820_MemoryMapEntry;
typedef struct
{
    uint32_t entryCount;
    e820_MemoryMapEntry* entries;
}_packed e820_MemoryMap;

// store in unused memory
#define BITMAP_MEMORY_ADDR (uint8_t*)0x30000

#define MMAP_TYPE_USED_CRITICAL     0
#define MMAP_TYPE_FREE              1
#define MMAP_TYPE_RESERVED          2
#define MMAP_TYPE_ACPI_RECLAIMABLE  3
#define MMAP_TYPE_ACPI_NVS          4
#define MMAP_TYPE_BAD               5

#define ACPI_ATTRIB_IGNORE          0x01
#define ACPI_ATTRIB_NON_VOLATILE    0x02

void mem_mark_free(uint32_t begin, uint32_t size)
{
    uint32_t beginPage = DivRoundUp(begin, PAGE_SIZE);
    uint32_t pageSize = DivRoundDown(begin + size, PAGE_SIZE) - beginPage;

    pagesAllocated.setBits(beginPage, pageSize, false);
}
void mem_mark_reserved(uint32_t begin, uint32_t size)
{
    uint32_t beginPage = DivRoundDown(begin, PAGE_SIZE);
    uint32_t pageSize = DivRoundUp(begin + size, PAGE_SIZE) - beginPage;

    pagesAllocated.setBits(beginPage, pageSize, true);
}

void mem_init(void* e820_mmap, void* _kernelMap)
{
    e820_MemoryMap memoryMap;

    memoryMap.entryCount = *(uint32_t*)e820_mmap;

    memoryMap.entries = (e820_MemoryMapEntry*)((uint32_t*)e820_mmap + 1);
    
    KernelMap& kernelMap = *(KernelMap*)_kernelMap;

    uint64_t lastAddress = 0;

    for(uint32_t i = 0; i < memoryMap.entryCount; i++)
    {
        if(memoryMap.entries[i].baseAddress + memoryMap.entries[i].regionSize > lastAddress)
        {
            lastAddress = memoryMap.entries[i].baseAddress + memoryMap.entries[i].regionSize;
        }
    }

    // mark all as reserved
    // then free only locations that e820_mmap says is free
    // then reserve all kernel related pages
    // then reserve all bitmap related pages
    // then reserve first page
    pagesAllocated = std::Bitmap(BITMAP_MEMORY_ADDR, DivRoundDown(lastAddress, PAGE_SIZE), true);

    for(uint32_t i = 0; i < memoryMap.entryCount; i++)
    {
        e820_MemoryMapEntry& entry = memoryMap.entries[i];

        if(entry.regionType == MMAP_TYPE_FREE)
        {
            mem_mark_free(entry.baseAddress, entry.regionSize);
        }
    }

    for(uint32_t i = 0; i < kernelMap.entryCount; i++)
    {
        mem_mark_reserved(kernelMap.entries[i].sectionBegin, kernelMap.entries[i].sectionSize);
    }

    mem_mark_reserved((uint32_t)BITMAP_MEMORY_ADDR, lastAddress);

    // reserve first page
    pagesAllocated.set(0, true);
}

void* alloc_page()
{
    uint32_t freePage = pagesAllocated.find_false();

    // no memory available
    if(freePage == std::Bitmap::npos)
    {
        return nullptr;
    }

    pagesAllocated.set(freePage, true);

    return reinterpret_cast<void*>(freePage * PAGE_SIZE);
}
void* alloc_pages(uint32_t count)
{
    uint32_t freePage = pagesAllocated.find_false_bits(count);

    // no memory available
    if(freePage == std::Bitmap::npos)
    {
        return nullptr;
    }

    for(uint32_t i = 0; i < count; i++)
    {
        pagesAllocated.set(freePage + i, true);
    }

    return reinterpret_cast<void*>(freePage * PAGE_SIZE);
}

void free_page(void* mem)
{
    uint32_t loc = reinterpret_cast<uint32_t>(mem);

    // if not at a page boundary
    // it may not be a allocated page
    // do not free
    // throw an exception once it is set up
    if(loc & 0xFFF)
    {
        x86_raise(0);
    }

    loc /= PAGE_SIZE;

    // free page
    pagesAllocated.set(loc, false);
}

void free_pages(void* mem, uint32_t count)
{
    uint32_t loc = reinterpret_cast<uint32_t>(mem);

    // if not at a page boundary
    // it may not be a allocated page
    // do not free
    // throw an exception once it is set up
    if(loc & 0xFFF)
    {
        x86_raise(0);
    }

    loc /= PAGE_SIZE;

    // free pages
    pagesAllocated.setBits(loc, count, false);
}
