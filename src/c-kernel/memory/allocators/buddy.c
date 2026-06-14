#include "../memory.h"
#include "buddy.h"

#include <pools.h>
#include <arch/defs.h>
#include <c-utils/c-utils.h>
#include <io/logger.h>
#include <hw/cpu.h>

#define BA_MAX_ORDER 20

#define BA_IS_LEAF(x) IS_INV_PTR(x->children)
#define BA_STATUS(flags) (flags & PNODE_STATUS_MASK)
#define BA_MAPPING(flags) (flags & PNODE_MAP_MASK)

struct {
    heap_allocator_t* allocator;
    ba_node_t* root;
} g_buddy_alloc;

u8 get_highest_setbit_loc(u32 value) {
    return 32 - __builtin_clz(value);
}
bool pnode_validate_flags_update(ba_node_t* target, u8 new_flag, bool bypass) {
    if(target->flags == new_flag) return true; // no change, valid update

    bool is_status_updating  = (target->flags & PNODE_STATUS_MASK) != (new_flag & PNODE_STATUS_MASK);
    bool is_status_upgrading = (target->flags & PNODE_STATUS_MASK)  > (new_flag & PNODE_STATUS_MASK);
    bool is_blocked = (target->flags & PNODE_STATUS_MASK) == PNODE_BLOCKED;
    bool is_used = (target->flags & PNODE_STATUS_MASK) == PNODE_USED;

    bool is_mapping_updating = (target->flags & PNODE_MAP_MASK) != (new_flag & PNODE_MAP_MASK);
    
    // can't change the mapping while in use
    // cannot be bypassed by PNODE_FORCE_OVERRIDE
    if(is_used && !is_mapping_updating) { return false; }

    if(bypass) { return true; }

    // if the node is blocked memory, invalid update
    // unless new_flag is changing the mapping
    if(is_blocked) { return is_mapping_updating; }

    return true;
}
void pnode_set_status(ba_node_t* target, u8 flag, bool bypass, bool fail_silently) {
    if(!pnode_validate_flags_update(target, flag, bypass)) {
        if(fail_silently) return;
        log_critical("[buddy allocator] invalid flag update. target node: {p}, target flag: {x}, new flag: {x}\n", target, target->flags, flag);
        panic(PANIC_BAD_MEMORY_REQUEST);
        return;
    }

    // we only modify the status
    flag = flag & PNODE_STATUS_MASK;
    u8 target_status = target->flags & PNODE_STATUS_MASK;
    // if target is blocked memory, DO NOT MODIFY.
    if(target_status == PNODE_BLOCKED) { return; }
    // if the new flag is the same return.
    if(target_status == flag) return;

    // update
    target->flags = SET_MASKED_FLAGS(PNODE_STATUS_MASK, target->flags, flag);

    // now move it upwards
    ba_node_t* node = target->parent;
    while(!IS_INV_PTR(node)) {
        u8 new_status = BA_STATUS(node->children[0].flags) | BA_STATUS(node->children[1].flags);
        node->flags = SET_MASKED_FLAGS(PNODE_STATUS_MASK, node->flags, new_status);
        node = node->parent;
    }
}
void pnode_set_mapping(ba_node_t* target, u8 flag, bool bypass, bool fail_silently) {
    if(!pnode_validate_flags_update(target, flag, bypass)) {
        if(fail_silently) return;
        log_critical("[buddy allocator] invalid flag update. target node: {p}, target flag: {x}, new flag: {x}\n", target, target->flags, flag);
        panic(PANIC_BAD_MEMORY_REQUEST);
        return;
    }

    // we only modify the mapping
    flag = flag & PNODE_MAP_MASK;
    u8 target_mapping = target->flags & PNODE_MAP_MASK;
    // if target is blocked memory, DO NOT MODIFY.
    if(target_mapping == PNODE_BLOCKED) { return; }
    // if the new flag is the same return.
    if(target_mapping == flag) return;

    // update
    target->flags = SET_MASKED_FLAGS(PNODE_MAP_MASK, target->flags, flag);

    // now move it upwards
    ba_node_t* node = target->parent;
    while(!IS_INV_PTR(node)) {
        u8 new_mapping = BA_MAPPING(node->children[0].flags) | BA_MAPPING(node->children[1].flags);
        node->flags = SET_MASKED_FLAGS(PNODE_MAP_MASK, node->flags, new_mapping);
        node = node->parent;
    }
}
void pnode_set_flags(ba_node_t* target, u8 flag, bool bypass, bool fail_silently) {
    if(!pnode_validate_flags_update(target, flag, bypass)) {
        if(fail_silently) return;
        log_critical("[buddy allocator] invalid flag update. target node: {p}, target flag: {x}, new flag: {x}\n", target, target->flags, flag);
        panic(PANIC_BAD_MEMORY_REQUEST);
        return;
    }

    target->flags = SET_MASKED_FLAGS(PNODE_FLAG_MASK, target->flags, flag);
    // now move it upwards
    ba_node_t* node = target->parent;
    while(!IS_INV_PTR(node)) {
        u8 new_flag = node->children[0].flags | node->children[1].flags;
        node->flags = SET_MASKED_FLAGS(PNODE_FLAG_MASK, node->flags, new_flag);
        node = node->parent;
    }
}

// helper functions
err_t ba_validate_node(ba_node_t* node) {
    if(IS_ERR_PTR(node)) return ERR_CAST(node);
    if(IS_ERR_PTR(node->parent)) return ERR_CAST(node->parent);
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
        .flags = (parent->flags & PNODE_FLAG_MASK) | ((-(u8)child_idx) & PNODE_BUDDY_OFFSET_NEGATIVE),
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
        bool contains_req = (curr_node->flags & PNODE_FLAG_MASK) & (search_flag & PNODE_FLAG_MASK);
        bool is_leaf = BA_IS_LEAF(curr_node);
        // no match: pop
        if(!contains_req) {
            goto pop;
        }
        // partial match: push
        else if(!is_leaf) {
            goto push;
        }
        // perfect match: check if order is correct
        else {
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

        // pushing
        push:
        if(search_flag & PNODE_SEARCH_REVERSE) {
            *top_stack = &curr_node->children[0];
            curr_node  = &curr_node->children[1];
        } else {
            *top_stack = &curr_node->children[1];
            curr_node  = &curr_node->children[0];
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
err_t ba_mark_pages(ptr_t address, usize page_cnt, u8 flags, bool bypass, bool fail_silently)
{
    usize cnt = page_cnt;
    
    ba_node_t* node = ba_find_node(address, 0);
    if(IS_ERR_PTR(node)) return ERR_CAST(node);
    pnode_set_flags(node, flags, bypass, fail_silently);
    cnt--;

    while(cnt > 0) {
        u32 last_page_addr = node->address + (1 << node->order);
        node = ba_get_next_leaf(node);
        if(IS_ERR_PTR(node)) return ERR_CAST(node);

        if((1 << node->order) > cnt) {
            node = ba_find_node(node->address, 0);
            if(IS_ERR_PTR(node)) return ERR_CAST(node);
        }

        pnode_set_flags(node, flags, bypass, fail_silently);
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

    ba_mark_pages(0, 1, PNODE_UNMAPPED | PNODE_BLOCKED, true, false); // reserve the null page

    //entry_cnt = 1;
    for(usize i = 0; i < entry_cnt; i++) {
        u32 page_addr = div_floor(entry->baseAddress, X86_PAGE_SIZE);
        u32 page_cnt  = div_ceil(entry->baseAddress + entry->regionSize, X86_PAGE_SIZE) - page_addr;

        if(page_addr == 0) {
            page_addr = 1;
            page_cnt -= 1;
        }
        
        err_t err = ba_mark_pages(page_addr, page_cnt, hdf_mmap_entry_type(entry), true, false);
        if(IS_ERROR(err)) return err;
        
        entry += 1;
    }

    return ESUCCESS;
}

// allocate page_cnt pages(may allocate more)
const page_alloc_info_t allocate_pages(usize page_cnt) {
    if(page_cnt == 0) return (page_alloc_info_t) { .error = ESUCCESS, .memory = nullptr, .size = 0 };

    u8 req_order = get_highest_setbit_loc(page_cnt);
    if((page_cnt & (page_cnt - 1)) == 0) req_order--;

    ba_node_t* node = ba_search(req_order, PNODE_FREE | PNODE_ON_RAM | PNODE_SEARCH_MODIFY);
    if(IS_ERR_PTR(node)) {
        if(ERR_CAST(node) == ENOTFOUND) return (page_alloc_info_t) { .error = ENOPAGE, .memory = nullptr, .size = 0 };
        else return (page_alloc_info_t) { .error = ERR_CAST(node), .memory = nullptr, .size = 0 };
    }

    pnode_set_status(node, PNODE_USED, false, false);

    return (page_alloc_info_t) { .error = ESUCCESS, .memory = (void*)(node->address * X86_PAGE_SIZE), .size = 1 << node->order };
}

void log_page_allocator_status() {
    ba_node_t* curr_node = ba_get_min_leaf(g_buddy_alloc.root);

    log_info("[buddy-allocator](log_page_allocator_status) Current Page Pool:\n");
    while(ERR_CAST(curr_node) != ENOPAGE) {
        if(IS_ERR_PTR(curr_node)) {
            log_error("[buddy-allocator](log_page_allocator_status) unexpected error. code: {x}\n", ERR_CAST(curr_node));
            panic(PANIC_UNEXPECTED_FAILURE);
            return;
        }

        u32 size = 1 << curr_node->order;

        char* status_str = "UNKNOWN";
        switch(BA_STATUS(curr_node->flags)) {
            case PNODE_FREE: status_str = "FREE"; break;
            case PNODE_USED: status_str = "USED"; break;
            case PNODE_BLOCKED: status_str = "BLOCKED"; break;
            case PNODE_ACPI: status_str = "ACPI"; break;
        }
        char* mapping_str = "UNKNOWN";
        switch(BA_MAPPING(curr_node->flags)) {
            case PNODE_UNMAPPED: mapping_str = "UNMAPPED"; break;
            case PNODE_ON_RAM: mapping_str = "ON_RAM"; break;
            case PNODE_IO_MAPPED: mapping_str = "IO_MAPPED"; break;
            case PNODE_BLK_MAPPED: mapping_str = "BLK_MAPPED"; break;
        }

        log_info("\tstart={p}, end={p}, status={6s%>}, mapping={10s%>}\n", 
            curr_node->address * X86_PAGE_SIZE, (curr_node->address + size) * X86_PAGE_SIZE,
            status_str, mapping_str
        );

        curr_node = ba_get_next_leaf(curr_node);
    }
}
