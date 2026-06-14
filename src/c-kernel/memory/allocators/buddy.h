// private functions & data structures for buddy allocator
// TODO: Fix the code.
#pragma once

#include <includes.h>
#include "../memory.h"

// NOTE: (flags & PNODE_AVAIL) && IS_INV_PTR(children) is invalid state
typedef struct ba_node_t {
    struct ba_node_t* parent;
    struct ba_node_t* children;
    ptr_t address;
    u8 flags;
    u8 order;
} ba_node_t;

u8 get_highest_setbit_loc(u32 value);

err_t ba_validate_node(ba_node_t* node);

ba_node_t ba_make_error(err_t err);
ba_node_t ba_make_child(ba_node_t* parent, ptr_t child_idx);

// 'split' a node
err_t ba_split_node(ba_node_t* parent);

// get the leaf node with minimum possible address
ba_node_t* ba_get_min_leaf(ba_node_t* root);
// get the sucessor of this leaf
ba_node_t* ba_get_next_leaf(ba_node_t* leaf);

// get the leaf node with maximum possible address
ba_node_t* ba_get_max_leaf(ba_node_t* root);
// get the predecessor of this leaf
ba_node_t* ba_get_prev_leaf(ba_node_t* leaf);

// search a node with matching search_flag and m_order
ba_node_t* ba_search(u8 m_order, u16 search_flag);

// find a node containing the address
// if m_order is non-negative:
// - split the found node to req_order
ba_node_t* ba_find_node(ptr_t address, i8 req_order);

// mark some pages with flags
err_t ba_mark_pages(ptr_t address, usize page_cnt, u8 flags, bool bypass, bool fail_silently);

