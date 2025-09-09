#include "heap.hpp"
#include "internals.hpp"

#include <c/math.h>
#include <io/io.h>
#include <cpu/memory.hpp>
#include <cpu/exceptions.hpp>

#define MIN_NODE_SIZE 0x10

#define HEAP_FLAGS 0xBEEF0000

#define HEAP_BIN_SIZE 0x100 // 1 MiB
#define HEAP_POOL_SIZE 0x100000 // 1 MiB

#define HEAP_FREE_NODE 0x1
#define HEAP_HINT_RESIZEABLE 0x2

_import char __heap_start[];
_import char __heap_end[];

namespace std
{
    using std::internal::heap_node;

    u8* heap_bin_top;
    size_t heap_bin_head;
    size_t heap_bin_size;

    std::internal::HeapBin alloc_bin;
    std::internal::HeapBin free_bin;

    u32 get_aligned_size(u32 req_size, u32 align=0x1)
    {
        u32 padding = ~(req_size & (align - 1)) + 1;
        // make sure padding is less than align, i.e padding = padding % align
        padding &= align - 1;

        return req_size + padding;
    }

    void initialize_heap()
    {
        heap_bin_top = (u8*)__heap_start;
        heap_bin_head = 0;
        heap_bin_size = (u8*)__heap_end - (u8*)__heap_start;
    }

    void* malloc_aligned(u32 req_size, u32 align)
    {
        // to meet alignment requirements
        // figure out the padding needed
        
        align = RoundUpTo2Power(align);
        // if normal alignment
        if(align <= 0x10) return malloc(req_size);

        // otherwise, find out padding
        u32 heap_head_addr = reinterpret_cast<u32>(heap_bin_top + heap_bin_head);

        u32 data_addr_nopad = heap_head_addr + sizeof(heap_node);

        u32 padding = ~(data_addr_nopad & (align - 1)) + 1;
        // make sure padding is less than align, i.e padding = padding % align
        padding &= align - 1;

        // if no padding present, do normal malloc
        if(padding == 0) return malloc(req_size);

        // make sure there is enough space
        if(heap_bin_size < padding + get_aligned_size(req_size))
        {
            x86_raise(NO_HEAP_MEMORY);
            return nullptr;
        }

        heap_node* padding_node = reinterpret_cast<heap_node*>(heap_bin_top + heap_bin_head);
        *padding_node = std::internal::construct_heap_node(padding - sizeof(heap_node));
        
        // add the padding node to free bin
        padding_node->flags |= HEAP_FREE_NODE;
        std::internal::bin_push(&free_bin, padding_node);

        heap_bin_head += padding;
        heap_bin_size -= padding;

        // return normal malloc
        return malloc(req_size);
    }

    void* malloc(u32 req_size)
    {
        if(req_size == 0)
        {
            log_warn("[heap manager] 0 bytes requested from malloc!\n");
            return nullptr;
        }

        // align size properly
        u32 aligned_req_size = get_aligned_size(req_size, 0x10);

        u32 req_node_size = sizeof(heap_node) + aligned_req_size;

        // heap bin is not finished
        if (req_node_size <= heap_bin_size)
        {
            heap_node* node = (heap_node*)(heap_bin_top + heap_bin_head);
            *node = std::internal::construct_heap_node(aligned_req_size);

            std::internal::bin_push(&alloc_bin, node);

            heap_bin_head += req_node_size;
            heap_bin_size -= req_node_size;

            // return the data region
            return (void*)(node + 1);
        }

        // heap bin is empty
        // find the a free node with space
        heap_node* node = std::internal::bin_find_node(free_bin, aligned_req_size + sizeof(heap_node) + MIN_NODE_SIZE);

        // there was nothing big enough
        if (node == nullptr)
        {
            node = std::internal::bin_find_node(free_bin, aligned_req_size);
            // out of memory
            if(node == nullptr)
            {
                x86_raise(NO_HEAP_MEMORY);
                return nullptr;
            }

            std::internal::bin_remove(&free_bin, node);
            std::internal::bin_push(&alloc_bin, node);

            // set free bit to 0
            node->flags &= ~HEAP_FREE_NODE;

            // return the data region
            return (void*)(node + 1);
        }

        // remove the found node
        std::internal::bin_remove(&free_bin, node);

        // split the node
        heap_node* split_node = (heap_node*)((u8*)node + req_node_size);
        *split_node = std::internal::construct_heap_node(node->size - aligned_req_size - sizeof(heap_node));

        
        // reconstruct node, to get rid of flags
        *node = std::internal::construct_heap_node(aligned_req_size);
        node->flags |= HEAP_HINT_RESIZEABLE;
        
        std::internal::bin_push(&alloc_bin, node);

        split_node->flags |= HEAP_FREE_NODE;
        std::internal::bin_push(&free_bin, split_node);

        // return the data region
        return (void*)(node + 1);
    }

    void* realloc(void *prev_ptr, u32 new_req_size)
    {
        if(new_req_size == 0)
        {
            log_warn("[heap manager] 0 bytes requested from realloc!\n");
            return nullptr;
        }

        heap_node* node = ((heap_node*)prev_ptr) - 1;
        if(!std::internal::heap_node_valid(*node))
        {
            log_warn("[heap manager] invalid pointer passed to realloc\n");
            return nullptr;
        }

        u32 aligned_req_size = get_aligned_size(new_req_size, 0x10);
        if(node->size > aligned_req_size)
        {
            log_warn("[heap manager] reducing allocated memory size using realloc is not supported yet!\n");
            return prev_ptr;
        }
        // node size could be larger, because of alignment
        else if(node->size > new_req_size) { return prev_ptr; }

        // check if this node is at the top of heap
        u32 node_end_pos = reinterpret_cast<u32>(prev_ptr) - reinterpret_cast<u32>(heap_bin_top) + node->size;
        if(node_end_pos == heap_bin_head)
        {
            heap_bin_head += aligned_req_size - node->size;
            heap_bin_size -= aligned_req_size - node->size;
            node->size = aligned_req_size;

            return prev_ptr;
        }

        // check if it might be possible to "remerge"
        if(node->flags & HEAP_HINT_RESIZEABLE)
        {
            heap_node* next_node = reinterpret_cast<heap_node*>(reinterpret_cast<u8*>(prev_ptr) + node->size);

            // check if the new node is valid, is free and has enough memory
            // otherwise do the normal free followed by malloc + memcpy
            if(!std::internal::heap_node_valid(*next_node)) goto free_and_alloc;
            if((next_node->flags & HEAP_FREE_NODE) == 0) goto free_and_alloc;
            if((next_node->size + sizeof(heap_node)) < (aligned_req_size - node->size)) goto free_and_alloc;

            // check if the next node can be split
            if(next_node->size + sizeof(heap_node) >= (aligned_req_size - node->size) + sizeof(heap_node) + MIN_NODE_SIZE)
            {
                u32 split_offset = (aligned_req_size - node->size);
                u32 split_size = (aligned_req_size - node->size) - sizeof(heap_node);

                // remove next_node from free bin
                std::internal::bin_remove(&free_bin, next_node);

                // yes, so now split it
                // split the node
                heap_node* split_node = (heap_node*)((u8*)next_node + split_offset);
                *split_node = std::internal::construct_heap_node(next_node->size - split_size);

                // set the current node's new size
                node->size = aligned_req_size;
                // we might be able to resize it again
                node->flags |= HEAP_HINT_RESIZEABLE;
                // but it isn't free
                node->flags &= ~HEAP_FREE_NODE;

                split_node->flags |= HEAP_FREE_NODE;

                // add the split node to free bin
                std::internal::bin_push(&free_bin, split_node);

                return prev_ptr;
            }

            // it is not possible to split next node

            // remove from free bin
            std::internal::bin_remove(&free_bin, next_node);

            // merge with current node
            node->size += next_node->size + sizeof(heap_node);
            // if the next node is resizable
            node->flags = HEAP_NODE_IDFLAG | (next_node->flags & HEAP_HINT_RESIZEABLE);

            return prev_ptr;
        }

        free_and_alloc:

        // nothing is possible :(
        void* new_ptr = malloc(new_req_size);
        memcpy(prev_ptr, new_ptr, new_req_size);
        free(prev_ptr);

        // return the new ptr
        return new_ptr;
    }

    void log_heap_status()
    {
        log_debug("Heap: Bin(loc = 0x%x, pos = 0x%x, free = 0x%x)\n", heap_bin_top, heap_bin_head, heap_bin_size);

        log_debug("\tAllocated Bin:\n");
        std::internal::bin_log(alloc_bin, "\t\t", 0);
        log_debug("\tFree Bin:\n");
        std::internal::bin_log(free_bin, "\t\t", 0);
    }

    void free(void* ptr)
    {
        heap_node* node = ((heap_node*)ptr) - 1;

        if(!std::internal::heap_node_valid(*node))
        {
            log_warn("[heap manager] invalid pointer passed to free\n");
            return;
        }

        if(node->flags & HEAP_FREE_NODE)
        {
            log_warn("[heap manager] double free detected!\n");
            return;
        }

        // mark as free
        node->flags |= HEAP_FREE_NODE;

        std::internal::bin_remove(&alloc_bin, node);

        // wait let's see if the end of this heap
        // is the position of the top of the heap
        u32 node_end_pos = reinterpret_cast<u32>(ptr) - reinterpret_cast<u32>(heap_bin_top) + node->size;
        
        //if(true)
        if(node_end_pos != heap_bin_head)
        {
            // more likely
            std::internal::bin_push(&free_bin, node);
        }
        else
        {
            // damn! then we will just merge it
            heap_bin_head -= sizeof(heap_node) + node->size;
            heap_bin_size += sizeof(heap_node) + node->size;
            // no need to add to free bin
        }
    }
}
