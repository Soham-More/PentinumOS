#include "memory.h"
#include <includes.h>
#include <std/Bitmap/Bitmap.hpp>
#include <std/Utils/math.h>
#include <i686/Exception.h>
#include <std/IO/stdio.h>

// TODO:
// add checks for Out of bound allocations
// Use a linked list allocator?
// 

static std::Bitmap pagesAllocated;
static uint64_t maxPageCount;

void memcpy(const void* src, void* dst, size_t size)
{
    for(uint32_t i = 0; i < size; i++)
    {
        ((char*)dst)[i] = ((char*)src)[i];
    }
}
void memset(void* src, uint8_t value, size_t size)
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

e820_MemoryMap memoryMap;

uint32_t allocatorStatus;
ptr_t lastKernelAddress;

void mem_mark_free(uint32_t begin, uint32_t size)
{
    uint32_t beginPage = DivRoundUp(begin, PAGE_SIZE);
    uint32_t pageSize = DivRoundDown(begin + size, PAGE_SIZE) - beginPage;

    pagesAllocated.setBits(beginPage, pageSize, false);
}
void mem_mark_reserved(uint64_t begin, uint64_t size)
{
    uint64_t beginPage = DivRoundDown(begin, PAGE_SIZE);
    uint64_t pageSize = DivRoundUp(begin + size, PAGE_SIZE) - beginPage;

    pagesAllocated.setBits(beginPage, pageSize, true);
}

void mem_init(KernelInfo& kernelInfo)
{
    memoryMap.entryCount = *(size_t*)kernelInfo.e820_mmap;
    memoryMap.entries = (e820_MemoryMapEntry*)((size_t*)kernelInfo.e820_mmap + 1);
    
    KernelMap& kernelMap = *(KernelMap*)kernelInfo.kernelMap;
    uint64_t lastAddress = 0;

    for(size_t i = 0; i < memoryMap.entryCount; i++)
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
    pagesAllocated = std::Bitmap(BITMAP_MEMORY_ADDR, DivRoundDown(UINT32_MAX, PAGE_SIZE), true);

    maxPageCount = DivRoundDown(lastAddress, PAGE_SIZE);

    for(size_t i = 0; i < memoryMap.entryCount; i++)
    {
        e820_MemoryMapEntry& entry = memoryMap.entries[i];

        if(entry.regionType == MMAP_TYPE_FREE)
        {
            mem_mark_free(entry.baseAddress, entry.regionSize);
        }
    }

    lastKernelAddress = 0;
    for(size_t i = 0; i < kernelMap.entryCount; i++)
    {
        mem_mark_reserved(kernelMap.entries[i].sectionBegin, kernelMap.entries[i].sectionSize);

        if(lastKernelAddress < kernelMap.entries[i].sectionBegin + kernelMap.entries[i].sectionSize)
        {
            lastKernelAddress = kernelMap.entries[i].sectionBegin + kernelMap.entries[i].sectionSize;
        }
    }

    mem_mark_reserved(ptr_cast(BITMAP_MEMORY_ADDR), DivRoundUp(DivRoundDown(UINT32_MAX, PAGE_SIZE), 8));

    // mark stack as reserved
    mem_mark_reserved(0, ptr_cast(&lastAddress) + 4096);

    // copy all contents of kernelInfo
    kernelInfo.e820_mmap = alloc_cp((void*)kernelInfo.e820_mmap, sizeof(uint32_t) + memoryMap.entryCount * sizeof(e820_MemoryMapEntry));
    kernelInfo.kernelMap = alloc_cp((void*)kernelInfo.kernelMap, sizeof(uint32_t) + kernelMap.entryCount * sizeof(KernelMapEntry));

    kernelInfo.pagingInfo = (PagingInfo*)alloc_cp((void*)kernelInfo.pagingInfo, sizeof(PagingInfo));

    // no error
    clearAllocatorStatus();
}

void prettyPrintMemory()
{
    bool isInUse = pagesAllocated.get(0);
    ptr_t beginPage = 0;

    printf("Allocated blocks: \n");
    
    for(size_t i = 1; i < pagesAllocated.size(); i++)
    {
        if(isInUse && !pagesAllocated.get(i))
        {
            printf("\t%p - %p: %p\n", beginPage * PAGE_SIZE, i * PAGE_SIZE, (i - beginPage) * PAGE_SIZE);
        }

        if(!isInUse && pagesAllocated.get(i))
        {
            beginPage = i;
        }

        isInUse = pagesAllocated.get(i);
    }
}

void* alloc_page()
{
    uint32_t freePage = pagesAllocated.find_false();

    // no memory available
    if(freePage == std::Bitmap::npos)
    {
        allocatorStatus |= ALLOC_NO_FREE_SPACE;

        return nullptr;
    }

    pagesAllocated.set(freePage, true);

    return reinterpret_cast<void*>(freePage * PAGE_SIZE);
}
void* alloc_pages(size_t count)
{
    size_t freePage = pagesAllocated.find_false_bits(count, true);

    // no memory available
    if(freePage == std::Bitmap::npos)
    {
        if(pagesAllocated.find_false() == std::Bitmap::npos) allocatorStatus |= ALLOC_NO_FREE_SPACE;
        else allocatorStatus |= ALLOC_REQ_SIZE_NAVAIL;

        return nullptr;
    }

    // release cached value
    pagesAllocated.find_false();

    pagesAllocated.setBits(freePage, count, true);

    return reinterpret_cast<void*>(freePage * PAGE_SIZE);
}
void* realloc_pages(void* pointer, size_t prev_count, size_t req_count)
{
    uint32_t loc = reinterpret_cast<uint32_t>(pointer) / PAGE_SIZE;

    for(size_t i = prev_count; i < req_count; i++)
    {
        // if allocated, then this array can't be extended
        if(pagesAllocated.get(i + loc))
        {
            void* newLoc = alloc_pages(req_count);

            memcpy(pointer, newLoc, prev_count * PAGE_SIZE);
            free_pages(pointer, prev_count);
            
            return newLoc;
        }
    }

    // if here then, memory can be extended
    pagesAllocated.setBits(loc, req_count, true);
    return pointer;
}

void* alloc_pages_kernel(size_t pages)
{
    ptr_t page = (lastKernelAddress & ~0xFFF) + 0x100000;

    mem_mark_reserved(page, pages);

    return (void*)page;
}

void* alloc_pages_aligned(size_t align, size_t count)
{
    for(size_t pageAddr = align; pageAddr < maxPageCount; pageAddr += align)
    {
        bool found = true;
        for(size_t i = 0; i < count; i++)
        {
            // if it is used up, then skip this alignement
            if(pagesAllocated.get(pageAddr + i))
            {
                found = false;
                break;
            }
        }
        if(found)
        {
            pagesAllocated.setBits(pageAddr, count, true);
            return reinterpret_cast<void*>(pageAddr * PAGE_SIZE);
        }
    }

    // did not find the memory with the alignment requirement
    return nullptr;
}

void* alloc_cp(const void* data, size_t size)
{
    size_t pageCount = DivRoundUp(size, PAGE_SIZE);

    void* loc = alloc_pages(pageCount);
    if(loc == nullptr) return nullptr;

    memcpy(data, loc, size);

    return loc;
}

void* alloc_io_space_pages(size_t align, size_t count)
{
    // start seaching before last entry
    // most likely it is 4G limit
    for(size_t pageAddr = memoryMap.entries[memoryMap.entryCount - 1].baseAddress - align; pageAddr > 0; pageAddr -= align)
    {
        bool found = true;
        for(size_t i = 0; i < count; i++)
        {
            // if it is used up, then skip this alignement
            if(pagesAllocated.get(pageAddr + i))
            {
                found = false;
                break;
            }
        }
        if(found)
        {
            pagesAllocated.setBits(pageAddr, count, true);
            return reinterpret_cast<void*>(pageAddr * PAGE_SIZE);
        }
    }

    // did not find the memory with the alignment requirement
    return nullptr;
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

void free_pages(void* mem, size_t count)
{
    size_t loc = reinterpret_cast<size_t>(mem);

    // if not at a page boundary
    // it may not be a allocated page
    // do not free
    // throw an exception once it is set up
    // also thow an exception if mem is nullptr
    if(loc & 0xFFF > 0 || mem == nullptr)
    {
        x86_raise(0);
    }

    loc /= PAGE_SIZE;

    // free pages
    pagesAllocated.setBits(loc, count, false);
}

uint32_t getAllocatorStatus()
{
    return allocatorStatus;
}
void clearAllocatorStatus()
{
    allocatorStatus = 0;
}
