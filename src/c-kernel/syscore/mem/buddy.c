#include "buddy.h"
#include <boot/init.h>

#include "priv.h"
#include <panic/panic.h>

#include <pools.h>

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
    // but if the node is being freed and the mapping is being updated, then allow it to bypass the "can't change mapping while in use" rule
    if(is_used && is_mapping_updating) { return is_status_upgrading && bypass; }

    if(bypass) { return true; }

    // if the node is blocked memory, invalid update
    // unless new_flag is changing the mapping
    if(is_blocked) { return is_mapping_updating; }

    return true;
}
err_t pnode_repair_tree(ba_node_t* target) {
    // combine nodes when both children are free
    // go upwards and repair the tree
    ba_node_t* node = target;
    while(!IS_INV_PTR(node->parent)) {
        ba_node_t* parent = node->parent;
        if(BA_IS_LEAF(&parent->children[0]) && BA_IS_LEAF(&parent->children[1])) {
            // both children are leaves
            if(BA_FLAGS(parent->children[0].flags) == BA_FLAGS(parent->children[1].flags) && 
               BA_STATUS(parent->children[0].flags) == PNODE_FREE) {
                // both children are free, merge them
                free(g_buddy_alloc.allocator, parent->children);
                parent->children = nullptr;
            }
        }
        node = parent;
    }

    return ESUCCESS;
}
err_t pnode_set_status(ba_node_t* target, u8 flag, bool bypass, bool fail_silently) {
    if(!pnode_validate_flags_update(target, flag, bypass)) {
        if(fail_silently) return EINVAL;
        kpanic(PANIC_BAD_MEMORY_REQUEST, "invalid flag update. target node: {p}, target flag: {x}, new flag: {x}\n", target, target->flags, flag);
        return EINVAL;
    }

    // we only modify the status
    flag = flag & PNODE_STATUS_MASK;
    u8 target_status = target->flags & PNODE_STATUS_MASK;
    // if target is blocked memory, DO NOT MODIFY.
    if(target_status == PNODE_BLOCKED) { return EINVAL; }
    // if the new flag is the same return.
    if(target_status == flag) return EINVAL;

    // update
    target->flags = SET_MASKED_FLAGS(PNODE_STATUS_MASK, target->flags, flag);

    // now move it upwards
    ba_node_t* node = target->parent;
    while(!IS_INV_PTR(node)) {
        u8 new_status = BA_STATUS(node->children[0].flags) | BA_STATUS(node->children[1].flags);
        node->flags = SET_MASKED_FLAGS(PNODE_STATUS_MASK, node->flags, new_status);
        node = node->parent;
    }
    return pnode_repair_tree(target);
}
err_t pnode_set_mapping(ba_node_t* target, u8 flag, bool bypass, bool fail_silently) {
    if(!pnode_validate_flags_update(target, flag, bypass)) {
        if(fail_silently) return EINVAL;
        kpanic(PANIC_BAD_MEMORY_REQUEST, "invalid flag update. target node: {p}, target flag: {x}, new flag: {x}\n", target, target->flags, flag);
        return EINVAL;
    }

    // we only modify the mapping
    flag = flag & PNODE_MAP_MASK;
    u8 target_mapping = target->flags & PNODE_MAP_MASK;
    // if target is blocked memory, DO NOT MODIFY.
    if(target_mapping == PNODE_BLOCKED) { return EINVAL; }
    // if the new flag is the same return.
    if(target_mapping == flag) return EINVAL;

    // update
    target->flags = SET_MASKED_FLAGS(PNODE_MAP_MASK, target->flags, flag);

    // now move it upwards
    ba_node_t* node = target->parent;
    while(!IS_INV_PTR(node)) {
        u8 new_mapping = BA_MAPPING(node->children[0].flags) | BA_MAPPING(node->children[1].flags);
        node->flags = SET_MASKED_FLAGS(PNODE_MAP_MASK, node->flags, new_mapping);
        node = node->parent;
    }
    return pnode_repair_tree(target);
}
err_t pnode_set_flags(ba_node_t* target, u8 flag, bool bypass, bool fail_silently) {
    if(!pnode_validate_flags_update(target, flag, bypass)) {
        if(fail_silently) return EINVAL;
        kpanic(PANIC_BAD_MEMORY_REQUEST, "invalid flag update. target node: {p}, target flag: {x}, new flag: {x}\n", target, target->flags, flag);
        return EINVAL;
    }

    target->flags = SET_MASKED_FLAGS(PNODE_FLAG_MASK, target->flags, flag);
    // now move it upwards
    ba_node_t* node = target->parent;
    while(!IS_INV_PTR(node)) {
        u8 new_flag = node->children[0].flags | node->children[1].flags;
        node->flags = SET_MASKED_FLAGS(PNODE_FLAG_MASK, node->flags, new_flag);
        node = node->parent;
    }
    return pnode_repair_tree(target);
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

bool ba_match_flags(ba_node_t* node, u8 flags) {
    bool is_status_match = node->flags & flags & PNODE_STATUS_MASK;
    bool is_map_match    = node->flags & flags & PNODE_MAP_MASK;
    return is_status_match && is_map_match;
}

// find a node of given order,mapping and status
ba_node_t* ba_search(u8 m_order, u16 search_flag) {
    
    ba_node_t* stack[BA_MAX_ORDER];
    memset((u8*)stack, _BYTELEN(stack), 0);

    ba_node_t** top_stack = stack;
    ba_node_t* curr_node = g_buddy_alloc.root;
    // perform the search
    while(!IS_INV_PTR(curr_node)) {
        bool contains_req = ba_match_flags(curr_node, search_flag);
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

err_t ba_mark_stack(ptr_t stack_bottom, ptr_t stack_top) {
    // mark the pages occupied by the stack with flags
    ptr_t page_aligned_bottom = div_floor(stack_bottom, X86_PAGE_SIZE);
    ptr_t page_aligned_top = div_ceil(stack_top, X86_PAGE_SIZE);

    usize page_cnt = page_aligned_top - page_aligned_bottom;
    return ba_mark_pages(page_aligned_bottom, page_cnt, PNODE_ON_RAM | PNODE_USED, false, false);
}

// api functions;
err_t initialize_buddy_allocator(heap_allocator_t* heap_allocator, KernelInfo* kInfo) {
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

    u32 entry_cnt = *(u32*)kInfo->e820_mmap;
    E820_MMAP_ENTRY* entry = (E820_MMAP_ENTRY*)((u32*)kInfo->e820_mmap + 1);

    ba_mark_pages(0, 1, PNODE_UNMAPPED | PNODE_BLOCKED, true, false); // reserve the null page

    for(usize i = 0; i < entry_cnt; i++) {
        u32 page_addr = div_floor(entry->baseAddress, X86_PAGE_SIZE);
        u32 page_cnt  = div_ceil(entry->baseAddress + entry->regionSize, X86_PAGE_SIZE) - page_addr;

        if(page_addr == 0) {
            page_addr = 1;
            page_cnt -= 1;
        }
        
        err_t err = ba_mark_pages(page_addr, page_cnt, hdf_mmap_entry_type(entry), true, false);
        if(err != ESUCCESS) return err;
        
        entry += 1;
    }

    for(usize i = 0; i < kInfo->kernelMap->entryCount; i++) {
        u32 page_addr = div_floor(kInfo->kernelMap->entries[i].sectionBegin, X86_PAGE_SIZE);
        u32 page_cnt  = div_ceil(kInfo->kernelMap->entries[i].sectionBegin + kInfo->kernelMap->entries[i].sectionSize, X86_PAGE_SIZE) - page_addr;

        if(page_addr == 0) {
            page_addr = 1;
            page_cnt -= 1;
        }

        err_t err = ba_mark_pages(page_addr, page_cnt, PNODE_ON_RAM | PNODE_USED, false, false);
        if(err != ESUCCESS) return err;
    }
    
    // mark the kernel stack pages as used
    /*
    err_t err = ba_mark_stack((ptr_t)__idle_thread_intr_stack_start, (ptr_t)__idle_thread_intr_stack_end);
    if(err != ESUCCESS) return err;
    err = ba_mark_stack((ptr_t)__idle_thread_exec_stack_start, (ptr_t)__idle_thread_exec_stack_end);
    if(err != ESUCCESS) return err;
    err = ba_mark_stack((ptr_t)__master_thread_intr_stack_start, (ptr_t)__master_thread_intr_stack_end);
    if(err != ESUCCESS) return err;
    err = ba_mark_stack((ptr_t)__master_thread_exec_stack_start, (ptr_t)__master_thread_exec_stack_end);
    if(err != ESUCCESS) return err;
    */

    err_t err = ba_mark_stack((ptr_t)__stack_rsvd_start, (ptr_t)__stack_rsvd_end);
    if(err != ESUCCESS) return err;
    err = ba_mark_stack((ptr_t)__heap_rsvd_start, (ptr_t)__heap_rsvd_end);
    if(err != ESUCCESS) return err;

    // mark the page tables as used
    err = ba_mark_pages(((ptr_t)kInfo->pagingInfo->pageDirectory) >> 12, 1, PNODE_ON_RAM | PNODE_USED, false, false);
    if(err != ESUCCESS) return err;
    err = ba_mark_pages(((ptr_t)kInfo->pagingInfo->pageTableArray) >> 12, kInfo->pagingInfo->table_count, PNODE_ON_RAM | PNODE_USED, false, false);
    if(err != ESUCCESS) return err;

    // special case, mark the video memory as used
    err = ba_mark_pages(0xB8000 >> 12, 1, PNODE_ON_RAM | PNODE_USED, false, false);
    if(err != ESUCCESS) return err;

    return ESUCCESS;
}

void log_page_allocator_status() {
    ba_node_t* curr_node = ba_get_min_leaf(g_buddy_alloc.root);

    ptr_t start_addr = 0; ptr_t end_addr = 0;
    u8 curr_flags = 0;

    usize largest_contiguous_memory = 0;

    log_info("[buddy-allocator](log_page_allocator_status) Current Page Pool:\n");
    while(ERR_CAST(curr_node) != ENOPAGE) {
        kpanic_on_err_ptr(curr_node, "unexpected error");
        u32 size = (1 << curr_node->order) * X86_PAGE_SIZE;

        if(curr_flags == BA_FLAGS(curr_node->flags)) {
            end_addr += size;
            curr_node = ba_get_next_leaf(curr_node);
            continue;
        }

        if(curr_flags != 0) {
            if((largest_contiguous_memory < PTR_DIFF_I32(end_addr, start_addr)) && (curr_flags == (PNODE_ON_RAM | PNODE_FREE))) {
                largest_contiguous_memory = PTR_DIFF_I32(end_addr, start_addr);
            }

            char* status_str = "UNKNOWN";
            switch(BA_STATUS(curr_flags)) {
                case PNODE_FREE: status_str = "FREE"; break;
                case PNODE_USED: status_str = "USED"; break;
                case PNODE_BLOCKED: status_str = "BLOCKED"; break;
                case PNODE_ACPI: status_str = "ACPI"; break;
            }
            char* mapping_str = "UNKNOWN";
            switch(BA_MAPPING(curr_flags)) {
                case PNODE_UNMAPPED: mapping_str = "UNMAPPED"; break;
                case PNODE_ON_RAM: mapping_str = "ON_RAM"; break;
                case PNODE_IO_MAPPED: mapping_str = "IO_MAPPED"; break;
                case PNODE_BLK_MAPPED: mapping_str = "BLK_MAPPED"; break;
            }

            log_info("\tstart={p}, end={p}, status={8s%>}, mapping={10s%>}\n", 
                start_addr, end_addr,
                status_str, mapping_str
            );
        }
        
        start_addr = curr_node->address * X86_PAGE_SIZE;
        end_addr = start_addr + size;
        curr_flags = BA_FLAGS(curr_node->flags);

        curr_node = ba_get_next_leaf(curr_node);
    }

    if(curr_flags != 0) {
        if(end_addr == 0) end_addr = 0xFFFFFFFF; // if end_addr is 0, it means it is the end of the address space

        if((largest_contiguous_memory < PTR_DIFF_I32(end_addr, start_addr)) && (curr_flags == (PNODE_ON_RAM | PNODE_FREE))) {
            largest_contiguous_memory = PTR_DIFF_I32(end_addr, start_addr);
        }

        char* status_str = "UNKNOWN";
        switch(BA_STATUS(curr_flags)) {
            case PNODE_FREE: status_str = "FREE"; break;
            case PNODE_USED: status_str = "USED"; break;
            case PNODE_BLOCKED: status_str = "BLOCKED"; break;
            case PNODE_ACPI: status_str = "ACPI"; break;
        }
        char* mapping_str = "UNKNOWN";
        switch(BA_MAPPING(curr_flags)) {
            case PNODE_UNMAPPED: mapping_str = "UNMAPPED"; break;
            case PNODE_ON_RAM: mapping_str = "ON_RAM"; break;
            case PNODE_IO_MAPPED: mapping_str = "IO_MAPPED"; break;
            case PNODE_BLK_MAPPED: mapping_str = "BLK_MAPPED"; break;
        }

        log_info("\tstart={p}, end={p}, status={8s%>}, mapping={10s%>}\n", 
            start_addr, end_addr,
            status_str, mapping_str
        );
    }

    log_info("[buddy-allocator](log_page_allocator_status) Largest Contiguous Free Memory: {u} MiB\n", largest_contiguous_memory >> 20);
}

x86_mmu_map_t make_template_page_table() {
    page_ptr_t pages = (page_ptr_t) __ptable_template_start;
    usize page_count = div_floor(PTR_DIFF_I32(__ptable_template_end, __ptable_template_start), X86_PAGE_SIZE);
    usize page_idx = 0;

    x86_mmu_map_t ptable = x86_construct_pagetable(&pages[page_idx]); page_idx++;
    
    ba_node_t* curr_node = ba_get_min_leaf(g_buddy_alloc.root);
    while(ERR_CAST(curr_node) != ENOPAGE) {
        kpanic_on_err_ptr(curr_node, "unexpected error");
        
        if(BA_FLAGS(curr_node->flags) != (PNODE_ON_RAM | PNODE_USED)) {
            curr_node = ba_get_next_leaf(curr_node);
            continue;
        }
        u32 size = (1 << curr_node->order);
        u32 paddress = curr_node->address * X86_PAGE_SIZE;

        // map the pages in the template page table to the physical address of the node
        usize req_pages = x86_map_pages_get_page_count(&ptable, paddress, size);
        if((page_idx + req_pages) > page_count) {
            kpanic(PANIC_OBJ_POOL_FULL, "not enough pages in the template page table to map all used pages in the buddy allocator");
        }
        // identity map the pages in the template page table
        err_t err = x86_map_pages(&ptable, paddress, paddress, size, X86_PAGE_PRESENT | X86_PAGE_RW, &pages[page_idx], req_pages);
        if(err != ESUCCESS) {
            kpanic(PANIC_UNEXPECTED_FAILURE, "failed to map pages in the template page table. error code: {x}", err);
        }
        page_idx += req_pages;

        curr_node = ba_get_next_leaf(curr_node);
    }

    return ptable;
}


