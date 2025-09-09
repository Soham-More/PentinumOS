#include "internals.hpp"

#include <io/io.h>
#include <arch/x86.h>

//#define DBG_MODE

#define TOKEN_TO_CSTR(x) #x

#ifdef DBG_MODE
#define VALIDATE_HEAP_BIN(heap_bin) \
        if (heap_bin == nullptr) \
        { \
            log_warn("[heap manager] %s called with nullptr args\n", __func__); \
            return; \
        } \
        if  ((heap_bin->head != nullptr || heap_bin->tail != nullptr) && \
            (heap_bin->head == nullptr || heap_bin->tail == nullptr)) \
        { \
            log_error("[heap manager] invalid heap bin passed to %s\n", __func__); \
            x86_raise(0); \
            return; \
        }
#endif
#ifndef DBG_MODE
#define VALIDATE_HEAP_BIN(heap_bin) 
#endif

#define WARN_NULLPTR(node) \
        if (node == nullptr) \
        { \
            log_warn("[heap manager] nullptr argument %s passed to %s\n", TOKEN_TO_CSTR(node), __func__); \
            return; \
        }

namespace std::internal
{
    heap_node construct_heap_node(u32 size)
    {
        return heap_node{.next = nullptr, .prev = nullptr, .size = size, .flags = HEAP_NODE_IDFLAG};
    }
    bool heap_node_valid(heap_node node)
    {
        return (node.flags & 0xFFFF0000) == HEAP_NODE_IDFLAG;
    }

    HeapBin construct_heap_bin()
    {
        return HeapBin{.head = nullptr, .tail = nullptr};
    }

    void bin_push(HeapBin* heap_bin, heap_node *node)
    {
        VALIDATE_HEAP_BIN(heap_bin);
        WARN_NULLPTR(node);

        // heap bin is not initialized
        if(heap_bin->head == nullptr)
        {
            node->next = nullptr;
            node->prev = nullptr;

            heap_bin->head = node;
            heap_bin->tail = node;
            return;
        }

        // otherwise 
        node->next = nullptr;
        node->prev = heap_bin->tail;

        heap_bin->tail->next = node;
        heap_bin->tail = node;
    }
    void bin_remove(HeapBin* heap_bin, heap_node* node)
    {
        VALIDATE_HEAP_BIN(heap_bin);
        WARN_NULLPTR(node);
        if(heap_bin->head == nullptr) {
            log_warn("[heap manager] empty heap_bin passed to bin_remove\n");
            return;
        }

        // this is the tail
        if(node->next == nullptr)
        {
            // this is the only node
            if(node->prev == nullptr)
            {
                heap_bin->head = nullptr;
                heap_bin->tail = nullptr;
                return;
            }
            // otherwise
            heap_bin->tail = node->prev;

            // the last node should not point to anything
            heap_bin->tail->next = nullptr;
            return;
        }
        // this is the head
        else if(node->prev == nullptr)
        {
            heap_bin->head = node->next;

            // don't point to this node anymore
            heap_bin->head->prev = nullptr;
            return;
        }

        // generic node
        node->prev->next = node->next;
        node->next->prev = node->prev;
    }

    void bin_log(HeapBin heap_bin, const char* spacing, u32 entry_count)
    {
        if  ((heap_bin.head != nullptr || heap_bin.tail != nullptr) && 
            (heap_bin.head == nullptr || heap_bin.tail == nullptr)) 
        {
            log_error("[heap manager] invalid heap bin passed to bin_log\n");
            x86_raise(0);
            return;
        }
        u32 count = 0;
        heap_node* node = heap_bin.head;
        while(node != nullptr)
        {
            if(entry_count + 4 > count)
            {
                log_debug("%s%p(size=0x%x FLAGS=0x%x) -> \n", spacing, node, node->size, node->flags );
            }
            else if(node->next == nullptr)
            {
                log_debug("%s...(%u more entries) -> \n", spacing, count - entry_count + 1);
                log_debug("%s%p(size=0x%x FLAGS=0x%x) -> \n", spacing, node, node->size, node->flags );
            }
            node = node->next;
            count++;
        }

        log_debug("%snullptr\n", spacing);
    }

    heap_node* bin_find_node(HeapBin heap_bin, u32 req_size)
    {
        heap_node* node = heap_bin.head;

        while(node != nullptr)
        {
            if (node->size >= req_size) return node;
            node = node->next;
        }

        return nullptr;
    }
}
