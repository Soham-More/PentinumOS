#pragma once

#include <includes.h>

#define HEAP_NODE_IDFLAG 0xDEAD0000

typedef struct heap_node
{
    struct heap_node* next;
    struct heap_node* prev;
    u32 size;
    u32 flags; // the high 16 bits are heap flags
}_packed heap_node;

typedef struct HeapBin{
    heap_node* head;
    heap_node* tail;
} HeapBin;

heap_node construct_heap_node(u32 size);
bool heap_node_valid(heap_node node);

HeapBin construct_heap_bin();

// standard linked list operations
void bin_push(HeapBin* heap_bin, heap_node* node);
void bin_remove(HeapBin* heap_bin, heap_node* node);

// logging
void bin_log(HeapBin heap_bin, const char* spacing, u32 entry_count);

// get a node with size >= req_size
heap_node* bin_find_node(HeapBin heap_bin, u32 req_size);
