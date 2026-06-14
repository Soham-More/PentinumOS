#include "../memory.h"
#include "buddy.h"

#include <pools.h>
#include <arch/defs.h>
#include <c-utils/c-utils.h>
#include <io/logger.h>

#define BA_MAX_ORDER 20

#define BA_IS_LEAF(x) IS_INV_PTR(x->children)

struct {
    heap_allocator_t* allocator;
    ba_node_t* root;
} g_buddy_alloc;

u8 get_highest_setbit_loc(u32 value) {
    return 32 - __builtin_clz(value);
}
void pnode_set_status(ba_node_t* target, u8 flag) {
    // we only modify the status
    flag = flag & PNODE_STATUS_MASK;
    u8 target_status = target->flags & PNODE_STATUS_MASK;
    // if target is blocked memory, DO NOT MODIFY.
    if(target_status == PNODE_BLOCKED) {
        return;
    }
    // if the new flag is the same return.
    if(target_status == flag) return;

    // update
    target->flags = (target->flags & ~PNODE_STATUS_MASK) | (flag & PNODE_STATUS_MASK);

    // now move it upwards
    ba_node_t* node = target->parent;
    while(!IS_INV_PTR(node)) {
        u8 new_status = PNODE_FREE;

        u8 free_cnt  = 0;
        if((node->children[0].flags & PNODE_STATUS_MASK) == PNODE_FREE)  free_cnt++;
        if((node->children[1].flags & PNODE_STATUS_MASK) == PNODE_FREE)  free_cnt++;
        u8 avail_cnt = 0;
        if((node->children[0].flags & PNODE_STATUS_MASK) == PNODE_AVAIL) avail_cnt++;
        if((node->children[1].flags & PNODE_STATUS_MASK) == PNODE_AVAIL) avail_cnt++;
        u8 blocked_cnt = 0;
        if((node->children[0].flags & PNODE_STATUS_MASK) == PNODE_BLOCKED) blocked_cnt++;
        if((node->children[1].flags & PNODE_STATUS_MASK) == PNODE_BLOCKED) blocked_cnt++;
        u8 cnt = avail_cnt + free_cnt;

        if(cnt == 0)      new_status = PNODE_USED;
        else if(cnt == 1) new_status = PNODE_AVAIL;
        else if(avail_cnt == 2) new_status = PNODE_AVAIL;
        else if(free_cnt == 2)  new_status = PNODE_FREE;
        else new_status = PNODE_USED;

        if(blocked_cnt == 2) new_status = PNODE_BLOCKED;

        node->flags = (node->flags & ~PNODE_STATUS_MASK) | new_status;

        node = node->parent;
    }
}
void pnode_sudo_set(ba_node_t* target, u8 flag) {
    // set the status
    pnode_set_status(target, flag);

    // now set the mapping, dangerous!
    u8 new_mapping = flag & PNODE_MAP_MASK;
    u8 target_mapping = target->flags & PNODE_MAP_MASK;

    if(new_mapping == target_mapping) return;

    // update
    target->flags = (target->flags & ~PNODE_MAP_MASK) | (flag & PNODE_MAP_MASK);

    ba_node_t* node = target->parent;
    while(!IS_INV_PTR(node)) {
        u8 mapping_A = node->children[0].flags & PNODE_MAP_MASK;
        u8 mapping_B = node->children[1].flags & PNODE_MAP_MASK;

        u8 new_mapping = PNODE_MIX_MAPPED;
        if(mapping_A == mapping_B) {
            new_mapping = mapping_A;
        }

        node->flags = MIX_MASKED(node->flags, new_mapping, PNODE_MAP_MASK);

        node = node->parent;
    }
}

// helper functions
err_t ba_validate_node(ba_node_t* node) {
    if(IS_ERR_PTR(node)) return ERR_CAST(node);
    if(IS_ERR_PTR(node->parent)) return ERR_CAST(node->parent);
    if(BA_IS_LEAF(node) && (node->flags & PNODE_AVAIL)) return EINVSELF;
    return ESUCCESS;
}

ba_node_t ba_make_error(err_t err) {
    return (ba_node_t) {
        .parent = ERR_PTR(ba_node_t, err),
        .children = nullptr,
        .address = err,
        .flags = invalid_u8,
        .order = invalid_u8,
    };
}
ba_node_t ba_make_child(ba_node_t* parent, ptr_t child_idx) {
    if(child_idx > 1) return ba_make_error(EUNEXPEXEC);
    return (ba_node_t) {
        .parent = parent,
        .children = nullptr,
        .address = parent->address + (child_idx << (parent->order - 1)),
        .flags = parent->flags | ((-(u8)child_idx) & PNODE_BUDDY_OFFSET_NEGATIVE),
        .order = parent->order - 1,
    };
}

err_t ba_split_node(ba_node_t* node) {
    ba_node_t* children = malloc(g_buddy_alloc.allocator, 2*sizeof(*node->children));
    if(IS_ERR_PTR(children)) return ERR_CAST(children);

    children[0] = ba_make_child(node, 0);
    children[1] = ba_make_child(node, 1);

    node->children = children;

    return ESUCCESS;
}

// match status flags
// returns 0 - if not matched
// returns 1 - if exact match
// returns 2 - if inexact match
u8 ba_match_flag_status(u8 flag, u16 search_flag, bool is_leaf) {
    // status match rules:
    // if:
    // - PNODE_SEARCH_ANY_STATUS is set: return 1
    // - search_flag & PNODE_STATUS_MASK == flag & PNODE_STATUS_MASK: return 1
    // - if search_flag & PNODE_STATUS_MASK = PNODE_FREE and !is_leaf and flag & PNODE_STATUS_MASK == PNODE_AVAIL: return 2
    // - otherwise return 0

    u8 search_flag_status = search_flag & PNODE_STATUS_MASK;
    u8 flag_status = flag & PNODE_STATUS_MASK;

    if(search_flag & PNODE_SEARCH_ANY_STATUS) return 1;
    if(is_leaf && (flag_status == search_flag_status))     return 1;
    if(!is_leaf && (search_flag_status == PNODE_FREE) && (flag_status == PNODE_AVAIL)) return 2;

    return 0;
}
// match mapping flags
// return 0xFF for successfull match
u8 ba_match_flag_mapping(u8 flag, u16 search_flag) {
    // status match rules:
    // if:
    // - PNODE_SEARCH_ANY_MAPPING is set: return true
    // - PNODE_MIX_MAPPING is set in flag: return true
    // - search_flag & PNODE_STATUS_MASK == flag & PNODE_STATUS_MASK: return true
    // - otherwise return false

    u8 search_flag_mapping = search_flag & PNODE_MAP_MASK;
    u8 flag_mapping = flag & PNODE_MAP_MASK;

    if(search_flag & PNODE_SEARCH_ANY_MAPPING) return 0xFF;
    if(flag_mapping == PNODE_MIX_MAPPED)       return 0xFF;
    if(flag_mapping == search_flag_mapping)      return 0xFF;
    return 0;
}

// match a flag to node
u8 ba_match_node(ba_node_t* node, u16 search_flag) {
    return 
        ba_match_flag_status (node->flags, search_flag, BA_IS_LEAF(node)) &
        ba_match_flag_mapping(node->flags, search_flag);
}

// get the leaf node with minimum possible address
ba_node_t* ba_get_min_leaf(ba_node_t* root) {
    ba_node_t* node = root;
    while(!IS_INV_PTR(node->children)) {
        node = &node->children[0];
    }
    return node;
}
// get the sucessor of this leaf
ba_node_t* ba_get_next_leaf(ba_node_t* leaf) {
    ba_node_t* node = leaf;
    while(node->flags & PNODE_BUDDY_OFFSET_NEGATIVE) {
        if(IS_INV_PTR(node->parent)) return ERR_PTR(ba_node_t, ENOPAGE);
        node = node->parent;
    }
    if(IS_INV_PTR(node->parent)) return ERR_PTR(ba_node_t, ENOPAGE);
    node = node->parent;
    return ba_get_min_leaf(&node->children[1]);
}

// get the leaf node with maximum possible address
ba_node_t* ba_get_max_leaf(ba_node_t* root) {
    ba_node_t* node = root;
    while(!IS_INV_PTR(node->children)) {
        node = &node->children[1];
    }
    return node;
}
// get the predecessor of this leaf
ba_node_t* ba_get_prev_leaf(ba_node_t* leaf)  {
    ba_node_t* node = leaf;
    while((node->flags & PNODE_BUDDY_OFFSET_NEGATIVE) == 0) {
        if(IS_INV_PTR(node->parent)) return ERR_PTR(ba_node_t, ENOPAGE);
        node = node->parent;
    }
    if(IS_INV_PTR(node->parent)) return ERR_PTR(ba_node_t, ENOPAGE);
    node = node->parent;
    return ba_get_max_leaf(&node->children[0]);
}

// find a node of given order,mapping and status
ba_node_t* ba_search(u8 m_order, u16 search_flag) {
    
    ba_node_t* stack[BA_MAX_ORDER];
    memset((u8*)stack, _BYTELEN(stack), 0);

    ba_node_t** top_stack = stack;
    ba_node_t* curr_node = g_buddy_alloc.root;
    // perform the search
    while(!IS_INV_PTR(curr_node)) {
        u8 match_type = ba_match_node(curr_node, search_flag);
        // no match: pop
        if(match_type == 0) {
            goto pop;
        }
        // partial match: push
        else if(match_type == 2) {
            goto push;
        }
        // perfect match: check if order is correct
        else if(match_type == 1) {
            // exact order! return this node
            if(curr_node->order == m_order) {
                return curr_node;
            }
            // a larger order, if we can split - split & push!
            else if((curr_node->order > m_order) && (search_flag & PNODE_SEARCH_MODIFY)) {
                err_t res = ba_split_node(curr_node);
                if(IS_ERROR(res)) return ERR_PTR(ba_node_t, res);
                goto push;
            }
            // otherwise pop
            else {
                goto pop;
            }
        }
        // no other possibility should be present
        else {
            return ERR_PTR(ba_node_t, EUNEXPEXEC);
        }

        // pushing
        push:
        if(search_flag & PNODE_SEARCH_REVERSE) {
            *top_stack = &curr_node->children[1];
            curr_node  = &curr_node->children[0];
        } else {
            *top_stack = &curr_node->children[0];
            curr_node  = &curr_node->children[1];
        }
        top_stack++;
        *top_stack = nullptr;
        continue;

        // poping
        pop:
        *top_stack = nullptr;
        if(top_stack == stack) return ERR_PTR(ba_node_t, ENOTFOUND);
        top_stack--;
        curr_node = *top_stack;
        continue;
    }

    return ERR_PTR(ba_node_t, ENOTFOUND);
}

// find a node containing the address
// if m_order is non-negative:
// - split the found node to req_order
ba_node_t* ba_find_node(ptr_t address, i8 req_order) {
    bool can_split = (req_order >= 0);
    if(!can_split) req_order = 0;

    ba_node_t* curr_node = g_buddy_alloc.root;
    for(i8 order = BA_MAX_ORDER; order > req_order; order -= 1) {
        if(IS_INV_PTR(curr_node->children)) {
            if(can_split) {
                err_t err = ba_split_node(curr_node);
                if(IS_ERROR(err)) return ERR_PTR(ba_node_t, err);
            } else {
                return curr_node;
            }
        }

        u8 child_idx = 0;
        if(address & (((ptr_t)1 << order) >> 1)) child_idx = 1;
        curr_node = &(curr_node->children[child_idx]);
    }

    if(can_split) {
        return curr_node;
    }
    return ERR_PTR(ba_node_t, EUNEXPEXEC);
}

// mark some pages with flags
err_t ba_mark_pages(ptr_t address, usize page_cnt, u8 flags)
{
    usize cnt = page_cnt;
    
    ba_node_t* node = ba_find_node(address, 0);
    if(IS_ERR_PTR(node)) return ERR_CAST(node);
    pnode_sudo_set(node, flags);

    while(cnt > 0) {
        node = ba_get_next_leaf(node);
        if(IS_ERR_PTR(node)) return ERR_CAST(node);

        if((1 << node->order) > cnt) {
            node = ba_find_node(node->address, 0);
            if(IS_ERR_PTR(node)) return ERR_CAST(node);
        }

        pnode_sudo_set(node, flags);
        cnt -= 1 << node->order;
    }

    return ESUCCESS;
}

// api functions;
err_t initialize_buddy_allocator(heap_allocator_t* heap_allocator, void* e820_mmap) {
    // initialize the heap allocator for buddy allocator
    g_buddy_alloc.allocator = initialize_heap(heap_allocator, __buddy_heap_start, PTR_DIFF_I32(__buddy_heap_start, __buddy_heap_end));

    // make a giant 4GiB(2^20 pages) node - initially free
    g_buddy_alloc.root = malloc(g_buddy_alloc.allocator, sizeof(ba_node_t));

    *g_buddy_alloc.root = (ba_node_t) {
        .parent = nullptr,
        .children = nullptr,
        .address = 0,
        .flags = PNODE_UNMAPPED | PNODE_FREE,
        .order = 20,
    };

    u32 entry_cnt = *(u32*)e820_mmap;
    E820_MMAP_ENTRY* entry = (E820_MMAP_ENTRY*)((u32*)e820_mmap + 1);

    ba_mark_pages(0, 1, PNODE_BLOCKED | PNODE_UNMAPPED);

    for(usize i = 0; i < entry_cnt; i++) {
        u32 page_addr = div_floor(entry->baseAddress, X86_PAGE_SIZE);
        u32 page_cnt  = div_ceil(entry->baseAddress + entry->regionSize, X86_PAGE_SIZE) - page_addr;

        err_t err = ba_mark_pages(page_addr, page_cnt, hdf_mmap_entry_type(entry));
        if(IS_ERROR(err)) return err;
        
        entry += 1;
    }

    return ESUCCESS;
}

// allocate page_cnt pages(may allocate more)
void* allocate_pages(usize page_cnt) {
    if(page_cnt == 0) return nullptr;

    u8 req_order = get_highest_setbit_loc(page_cnt);
    if(page_cnt & (page_cnt - 1)) req_order++;

    ba_node_t* node = ba_search(req_order, PNODE_FREE | PNODE_ON_RAM | PNODE_SEARCH_MODIFY);
    if(IS_ERR_PTR(node)) {
        if(ERR_CAST(node) == ENOTFOUND) return ERR_PTR(void, ENOPAGE);
        else return (void*)node;
    }

    pnode_set_status(node, PNODE_USED);

    return (void*)node->address;
}

void log_page_allocator_status() {
    ba_node_t* curr_node = ba_get_min_leaf(g_buddy_alloc.root);

    log_info("[buddy-allocator](log_page_allocator_status) Current Page Pool:\n");
    while(ERR_CAST(curr_node) != ENOPAGE) {
        if(IS_ERR_PTR(curr_node)) {
            log_error("[buddy-allocator](log_page_allocator_status) unexpected error. code: 0x%x\n", ERR_CAST(curr_node));
            // TODO: kill
        }

        u32 size = 1 << curr_node->order;

        log_info("\taddr=0x%x, size=0x%x, status=0x%x, mapping=0x%x\n", 
            curr_node->address, size, 
            curr_node->flags & PNODE_STATUS_MASK, curr_node->flags & PNODE_MAP_MASK
        );

        curr_node = ba_get_next_leaf(curr_node);
    }
}
