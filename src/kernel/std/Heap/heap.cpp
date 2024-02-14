#include "heap.hpp"

#include <std/memory.h>

#define HEAP_BIN_SIZE 0x10
#define MIN_NODE_SIZE 0x10

namespace std
{
    struct free_node
    {
        free_node* next;
        free_node* prev;
        size_t size;
    }_packed;
    struct alloc_node
    {
        alloc_node* next;
        alloc_node* prev;
        size_t size;
    }_packed;

    uint8_t* heap_bin_top;
    size_t heap_bin_head;
    size_t heap_bin_size;

    alloc_node* prevAllocNode;
    free_node* free_bin;

    ptr_t getAlignPadding(ptr_t value, ptr_t align)
    {
        return (align - (value & (align - 1))) & (align - 1);
    }

    void add_free_node(ptr_t address, size_t size = 0x0)
    {
        free_node* node = (free_node*)address;

        // if free bin exists, then add this node to end of linked list
        if(free_bin)
        {
            free_bin->next = node;
            node->prev = free_bin;
        }
        else
        {
            node->prev = nullptr;
        }

        if(size) node->size = size;
        
        node->next = nullptr;
        free_bin = node;
    }

    void remove_free_node(free_node* node)
    {
        // delete this node
        if(node->prev)
        {
            node->prev->next = node->next;

            if(node->next)
            {
                node->next->prev = node->prev;
            }
            // this node is at top
            else
            {
                free_bin = node->prev;
            }
        }
        else
        {
            if(node->next)
            {
                node->next->prev = nullptr;
            }
            // this node is at top
            else
            {
                free_bin = nullptr;
            }
        }
    }
    void remove_heap_node(alloc_node* node)
    {
        // delete this node
        if(node->prev)
        {
            node->prev->next = node->next;

            if(node->next)
            {
                node->next->prev = node->prev;
            }
            // this node is at top
            else
            {
                prevAllocNode = node->prev;
            }
        }
        else
        {
            if(node->next)
            {
                node->next->prev = nullptr;
            }
            // this node is at top
            else
            {
                prevAllocNode = nullptr;
            }
        }
    }

    void initHeap()
    {
        heap_bin_top = (uint8_t*)alloc_pages_kernel(HEAP_BIN_SIZE);
        heap_bin_head = 0;
        heap_bin_size = HEAP_BIN_SIZE * PAGE_SIZE;
        free_bin = nullptr;
    }

    // align MUST be power of 2
    void* allocHeapBin(size_t size, size_t align = 0x4)
    {
        // align size to 4 bytes.
        size += getAlignPadding(size, 0x4) + sizeof(alloc_node);

        alloc_node* new_node = (alloc_node*)(heap_bin_top + heap_bin_head);
        ptr_t address = ptr_cast(new_node + 1);

        size_t padding = getAlignPadding(address, align);

        // heap bin full, allocate more heap.
        if(heap_bin_head + size + padding > heap_bin_size)
        {
            uint8_t* reallocBin = (uint8_t*)realloc_pages(heap_bin_top, heap_bin_size, HEAP_BIN_SIZE);
            size_t reqSize = HEAP_BIN_SIZE;

            // run out of space/RAM is fragmented.
            if(reallocBin == nullptr)
            {
                // no free space, quit
                if(getAllocatorStatus() & ALLOC_NO_FREE_SPACE) return nullptr;
                // free space available, get one page.
                else if(getAllocatorStatus() & ALLOC_REQ_SIZE_NAVAIL)
                {
                    reallocBin = (uint8_t*)realloc_pages(heap_bin_top, heap_bin_size, 1);
                    reqSize = 1;
                    clearAllocatorStatus();
                }
            }

            // if the pointer changed, then reset heap_head and heap_size
            // recalculate padding.
            if(reallocBin != heap_bin_top)
            {
                heap_bin_head = 0;
                heap_bin_size = reqSize * PAGE_SIZE;
                heap_bin_top = reallocBin;

                new_node = (alloc_node*)(heap_bin_top + heap_bin_head);
                address = ptr_cast(new_node + 1);

                padding = getAlignPadding(address, align);
            }
            else
            {
                heap_bin_size += reqSize * PAGE_SIZE;
            }
        }

        // if padding is more than MIN_NODE_SIZE, we can put it in the free bin
        // so that the space is not wasted
        if(padding > sizeof(alloc_node) + MIN_NODE_SIZE)
        {
            add_free_node(address, padding);
        }

        new_node = (alloc_node*)(address + padding - sizeof(alloc_node));

        heap_bin_head += size + padding;

        new_node->next = nullptr;
        new_node->prev = prevAllocNode;
        new_node->size = size;

        return (void*)(address + padding);
    }

    // align MUST be power of 2
    void* allocFreeBin(size_t size, size_t align = 0x4)
    {
        // align size to 4 bytes.
        size += getAlignPadding(size, 0x4) + sizeof(alloc_node);

        free_node* node = free_bin;

        // loop till node is not null
        while(node)
        {
            // TODO: align data area.
            if(size + sizeof(alloc_node) + MIN_NODE_SIZE < node->size)
            {
                //node->size -= size; // decrease node size

                alloc_node* alloc = (alloc_node*)((uint8_t*)node + node->size - size);
                ptr_t address = ptr_cast(alloc + 1);
                ptr_t padding = address & (align - 1);

                // padding makes it too big
                if(size + padding + sizeof(alloc_node) + MIN_NODE_SIZE > node->size)
                {
                    goto allocFullNode;
                }

                size += padding;
                node->size -= size;
                alloc = (alloc_node*)((uint8_t*)node + node->size);
                address = ptr_cast(alloc + 1);

                alloc->prev = prevAllocNode;
                alloc->next = nullptr;
                alloc->size = size;

                prevAllocNode->next = alloc;
                prevAllocNode = alloc;

                // return the data section
                return (void*)(address);
            }
            allocFullNode:
            if(size + getAlignPadding(ptr_cast((alloc_node*)node + 1), align) <= node->size)
            {
                remove_free_node(node);

                alloc_node* alloc = (alloc_node*)node;
                ptr_t address = ptr_cast((alloc_node*)node + 1);
                ptr_t padding = getAlignPadding(address, align);
                address += padding;

                alloc = (alloc_node*)(address - sizeof(alloc_node));

                alloc->prev = prevAllocNode;
                alloc->next = nullptr;
                alloc->size = size;

                prevAllocNode->next = alloc;
                prevAllocNode = alloc;

                // return the data section
                return (alloc_node*)(address);
            }

            node = node->prev;
        }

        // no free_bin node matches requirement
        return allocHeapBin(size, align);
    }

    void* malloc(size_t size)
    {
        if(!free_bin) return allocHeapBin(size);

        return allocFreeBin(size);
    }

    void* mallocAligned(size_t size, size_t align)
    {
        align = 1 << (31 - __builtin_clz(align));

        if(!free_bin) return allocHeapBin(size, align);
        return allocFreeBin(size, align);
    }

    void free(void* ptr)
    {
        if(!ptr) return;

        alloc_node* ptrNode = (alloc_node*)ptr - 1;

        // delete this node
        remove_heap_node(ptrNode);

        // if free bin exists, then add this node to end of linked list
        add_free_node(ptr_cast(ptr) - sizeof(alloc_node));
    }
}
