#include "memory.hpp"

#include <c/math.h>
#include <io/io.h>

#include <arch/x86.h>

extern u8 __page_allocator_start[];

#define MMAP_TYPE_USED_CRITICAL     0
#define MMAP_TYPE_FREE              1
#define MMAP_TYPE_RESERVED          2
#define MMAP_TYPE_ACPI_RECLAIMABLE  3
#define MMAP_TYPE_ACPI_NVS          4
#define MMAP_TYPE_BAD               5

#define ACPI_ATTRIB_IGNORE          0x01
#define ACPI_ATTRIB_NON_VOLATILE    0x02

namespace cpu
{
    namespace
    {
        static u32 allocator_status = 0;

        static std::Bitmap page_status;
        static uint64_t page_count;
        static uint64_t kernel_end;

        typedef struct
        {
            u32 sectionBegin;
            u32 sectionSize;
        }_packed KernelMapEntry;
        typedef struct
        {
            u32 entryCount;
            KernelMapEntry* entries;
        }_packed KernelMap;
        typedef struct
        {
            uint64_t baseAddress;
            uint64_t regionSize;
            u32 regionType;
            u32 ACPI3_eattrib_bf;
        } _packed e820_MemoryMapEntry;
        typedef struct
        {
            u32 entryCount;
            e820_MemoryMapEntry* entries;
        }_packed e820_MemoryMap;

        e820_MemoryMap memoryMap;

        void mem_mark_free(u32 begin, u32 size)
        {
            u32 page_begin = DivRoundUp(begin, PAGE_SIZE);
            u32 page_cnt = DivRoundDown(begin + size, PAGE_SIZE) - page_begin;

            page_status.setBits(page_begin, page_cnt, false);
        }
        void mem_mark_reserved(uint64_t begin, uint64_t size)
        {
            uint64_t page_begin = DivRoundDown(begin, PAGE_SIZE);
            uint64_t page_cnt = DivRoundUp(begin + size, PAGE_SIZE) - page_begin;

            page_status.setBits(page_begin, page_cnt, true);
        }
    }

    void initialize_memory(KernelInfo& kernel_info)
    {
        memoryMap.entryCount = *(size_t*)kernel_info.e820_mmap;
        memoryMap.entries = (e820_MemoryMapEntry*)((size_t*)kernel_info.e820_mmap + 1);

        KernelMap& kernelMap = *(KernelMap*)kernel_info.kernelMap;
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
        page_status = std::Bitmap(__page_allocator_start, DivRoundDown(UINT32_MAX, PAGE_SIZE), true);

        page_count = DivRoundDown(lastAddress, PAGE_SIZE);

        for(size_t i = 0; i < memoryMap.entryCount; i++)
        {
            e820_MemoryMapEntry& entry = memoryMap.entries[i];

            if(entry.regionType == MMAP_TYPE_FREE)
            {
                mem_mark_free(entry.baseAddress, entry.regionSize);
            }
        }

        kernel_end = 0;
        for(size_t i = 0; i < kernelMap.entryCount; i++)
        {
            mem_mark_reserved(kernelMap.entries[i].sectionBegin, kernelMap.entries[i].sectionSize);

            if(kernel_end < kernelMap.entries[i].sectionBegin + kernelMap.entries[i].sectionSize)
            {
                kernel_end = kernelMap.entries[i].sectionBegin + kernelMap.entries[i].sectionSize;
            }
        }

        //mem_mark_reserved(ptr_cast(BITMAP_MEMORY_ADDR), DivRoundUp(DivRoundDown(UINT32_MAX, PAGE_SIZE), 8));

        // mark stack as reserved
        mem_mark_reserved(0, ptr_cast(&lastAddress) + 4096);

        // copy all contents of kernelInfo
        kernel_info.e820_mmap = alloc_cp((void*)kernel_info.e820_mmap, sizeof(u32) + memoryMap.entryCount * sizeof(e820_MemoryMapEntry));
        kernel_info.kernelMap = alloc_cp((void*)kernel_info.kernelMap, sizeof(u32) + kernelMap.entryCount * sizeof(KernelMapEntry));

        kernel_info.pagingInfo = (PagingInfo*)alloc_cp((void*)kernel_info.pagingInfo, sizeof(PagingInfo));
    }

    void* alloc_page()
    {
        u32 freePage = page_status.find_false();

        // no memory available
        if(freePage == std::Bitmap::npos)
        {
            allocator_status |= ALLOC_NO_FREE_SPACE;

            return nullptr;
        }

        page_status.set(freePage, true);

        return reinterpret_cast<void*>(freePage * PAGE_SIZE);
    }
    void logMemoryInfo()
    {
        bool isInUse = page_status.get(0);
        ptr_t beginPage = 0;

        printf("Allocated blocks: \n");
        
        for(size_t i = 1; i < page_status.size(); i++)
        {
            if(isInUse && !page_status.get(i))
            {
                printf("\t%p - %p: %p\n", beginPage * PAGE_SIZE, i * PAGE_SIZE, (i - beginPage) * PAGE_SIZE);
            }

            if(!isInUse && page_status.get(i))
            {
                beginPage = i;
            }

            isInUse = page_status.get(i);
        }
    }
    void *alloc_pages(size_t count)
    {
        size_t freePage = page_status.find_false_bits(count, true);

        // no memory available
        if(freePage == std::Bitmap::npos)
        {
            if(page_status.find_false() == std::Bitmap::npos) allocator_status |= ALLOC_NO_FREE_SPACE;
            else allocator_status |= ALLOC_REQ_SIZE_NAVAIL;

            return nullptr;
        }

        // release cached value
        page_status.find_false();

        page_status.setBits(freePage, count, true);

        return reinterpret_cast<void*>(freePage * PAGE_SIZE);
    }
    void* realloc_pages(void* pointer, size_t prev_count, size_t req_count)
    {
        u32 loc = reinterpret_cast<u32>(pointer) / PAGE_SIZE;

        for(size_t i = prev_count; i < req_count; i++)
        {
            // if allocated, then this array can't be extended
            if(page_status.get(i + loc))
            {
                void* newLoc = alloc_pages(req_count);

                std::memcpy(pointer, newLoc, prev_count * PAGE_SIZE);
                free_pages(pointer, prev_count);
                
                return newLoc;
            }
        }

        // if here then, memory can be extended
        page_status.setBits(loc, req_count, true);
        return pointer;
    }

    void *alloc_extend(void *pointer, size_t prev_count, size_t req_count)
    {
        u32 loc = reinterpret_cast<u32>(pointer) / PAGE_SIZE;

        for(size_t i = prev_count; i < req_count; i++)
        {
            // if allocated, then this array can't be extended
            if(page_status.get(i + loc))
            {
                allocator_status |= ALLOC_REQ_IMPOSSIBLE;
                return nullptr; // can't extend, return nullptr
            }
        }

        // if here then, memory can be extended
        page_status.setBits(loc, req_count, true);
        return pointer;
    }

    void* alloc_cp(const void* data, size_t size)
    {
        size_t pageCount = DivRoundUp(size, PAGE_SIZE);

        void* loc = alloc_pages(pageCount);
        if(loc == nullptr) return nullptr;

        std::memcpy(data, loc, size);

        return loc;
    }

    void* alloc_io_pages(size_t align, size_t count)
    {
        // start seaching before last entry
        // most likely it is 4G limit
        for(size_t pageAddr = memoryMap.entries[memoryMap.entryCount - 1].baseAddress - align; pageAddr > 0; pageAddr -= align)
        {
            bool found = true;
            for(size_t i = 0; i < count; i++)
            {
                // if it is used up, then skip this alignement
                if(page_status.get(pageAddr + i))
                {
                    found = false;
                    break;
                }
            }
            if(found)
            {
                page_status.setBits(pageAddr, count, true);
                return reinterpret_cast<void*>(pageAddr * PAGE_SIZE);
            }
        }

        // did not find the memory with the alignment requirement
        return nullptr;
    }

    void free_page(void* mem)
    {
        u32 loc = reinterpret_cast<u32>(mem);

        // if not at a page boundary
        // it may not be a allocated page
        // do not free
        // throw an exception once it is set up
        if(loc & 0xFFF)
        {
            log_error("Attempting to free a page that is not page-aligned");
            x86_raise(0);
        }

        loc /= PAGE_SIZE;

        // free page
        page_status.set(loc, false);
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
            log_error("Attempting to free a page that is not page-aligned or a nullptr");
            x86_raise(0);
        }

        loc /= PAGE_SIZE;

        // free pages
        page_status.setBits(loc, count, false);
    }
}