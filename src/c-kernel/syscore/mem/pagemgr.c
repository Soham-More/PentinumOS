#include "pagemgr.h"

#include <panic/panic.h>

page_alloc_info_t g_tmp_info;

// allocate page_cnt pages(may allocate more)
page_alloc_info_t* allocate_pages(page_mgr_ctx_t* ctx, usize page_cnt) {
    u8 req_order = get_highest_setbit_loc(page_cnt);
    if((page_cnt & (page_cnt - 1)) == 0) req_order--;

    ba_node_t* node = ba_search(req_order, PNODE_FREE | PNODE_ON_RAM | PNODE_SEARCH_MODIFY);
    if(IS_ERR_PTR(node)) {
        if(ERR_CAST(node) == ENOTFOUND) return ERR_PTR(page_alloc_info_t, ENOPAGE);
        else return (page_alloc_info_t*) node;
    }

    pnode_set_status(node, PNODE_USED, false, false);

    // if heap is null, we just allocate the pages without tracking them in the page manager context
    if(!ctx->heap_allocator) {
        g_tmp_info.memory = (void*)(node->address * X86_PAGE_SIZE);
        g_tmp_info.count = 1 << node->order;
        g_tmp_info.next = nullptr;
        return &g_tmp_info;
    }

    if(ctx->alloc_pages_head) {
        ctx->alloc_pages_tail->next = malloc(ctx->heap_allocator, sizeof(page_alloc_info_t));
        ctx->alloc_pages_tail = ctx->alloc_pages_tail->next;
    } else {
        ctx->alloc_pages_head = malloc(ctx->heap_allocator, sizeof(page_alloc_info_t));
        ctx->alloc_pages_tail = ctx->alloc_pages_head;
    }

    ctx->alloc_pages_tail->next = nullptr;
    ctx->alloc_pages_tail->memory = (void*)(node->address * X86_PAGE_SIZE);
    ctx->alloc_pages_tail->count = 1 << node->order;

    return ctx->alloc_pages_tail;
}

// allocate page_cnt pages(may allocate more) on memory not on ram
// if status is PNODE_BLK_MAPPED - then mark allocated pages as BLK_MAPPED
// if status is PNODE_IO_MAPPED - then mark allocated pages as IO_MAPPED
page_alloc_info_t* allocate_mapped_pages(page_mgr_ctx_t* ctx, u16 status, usize page_cnt) {
    if(page_cnt == 0) return ERR_PTR(page_alloc_info_t, EINVAL);
    u8 validated_map = status & (PNODE_BLK_MAPPED | PNODE_IO_MAPPED);
    if(validated_map == 0) return ERR_PTR(page_alloc_info_t, EINVAL);

    u8 req_order = get_highest_setbit_loc(page_cnt);
    if((page_cnt & (page_cnt - 1)) == 0) req_order--;

    ba_node_t* node = ba_search(req_order, PNODE_FREE | PNODE_UNMAPPED | PNODE_SEARCH_MODIFY | PNODE_SEARCH_REVERSE);
    if(IS_ERR_PTR(node)) {
        if(ERR_CAST(node) == ENOTFOUND) return ERR_PTR(page_alloc_info_t, ENOPAGE);
        else return (page_alloc_info_t*) node;
    }

    pnode_set_flags(node, PNODE_USED | validated_map, false, false);

    if(ctx->alloc_pages_head) {
        ctx->alloc_pages_tail->next = malloc(ctx->heap_allocator, sizeof(page_alloc_info_t));
        ctx->alloc_pages_tail = ctx->alloc_pages_tail->next;
    } else {
        ctx->alloc_pages_head = malloc(ctx->heap_allocator, sizeof(page_alloc_info_t));
        ctx->alloc_pages_tail = ctx->alloc_pages_head;
    }

    ctx->alloc_pages_tail->next = nullptr;
    ctx->alloc_pages_tail->memory = (void*)(node->address * X86_PAGE_SIZE);
    ctx->alloc_pages_tail->count = 1 << node->order;

    return ctx->alloc_pages_tail;
}

// free allocated pages
err_t free_pages(const page_alloc_info_t* page_info) {
    // validate page_info
    if(!page_info) return EINVAL;
    if(page_info->count == 0)                             return EINVAL;
    if((page_info->count & (page_info->count - 1)) != 0)  return EINVAL;
    if((ptr_t)page_info->memory % page_info->count != 0)  return EINVAL;

    ba_node_t* node = ba_find_node((ptr_t)page_info->memory / X86_PAGE_SIZE, get_highest_setbit_loc(page_info->count) - 1);
    if(IS_ERR_PTR(node)) {
        return ERR_CAST(node);
    }

    if(BA_STATUS(node->flags) != PNODE_USED) {
        panic(
            PANIC_BAD_MEMORY_REQUEST, 
            "invalid page free request. target node: {p}, target mapping: {x}\n", 
            node, BA_MAPPING(node->flags)
        );
        return EINVAL;
    }

    if(BA_MAPPING(node->flags) == PNODE_ON_RAM) {
        // if the page is not on ram, just mark it as free
        pnode_set_status(node, PNODE_FREE, true, false);
        return ESUCCESS;
    } else {
        // if the page is on ram, mark it as unmapped and free
        pnode_set_flags(node, PNODE_FREE | PNODE_UNMAPPED, true, false);
    }

    return ESUCCESS;
}

// construct a page allocator context using the heap allocator
page_mgr_ctx_t construct_page_mgr_ctx(heap_allocator_t* heap_allocator, x86_mmu_map_t ptable) {
    page_mgr_ctx_t ctx = {
        .heap_allocator = heap_allocator,
        .alloc_pages_head = nullptr,
        .alloc_pages_tail = nullptr,
        .ptable = ptable,
        .page_count = 0,
        .ram_page_count = 0,
    };

    return ctx;
}

// allocate pages & map them to a virtual address
err_t pmgr_alloc_pages(page_mgr_ctx_t* ctx, ptr_t vaddress, usize page_cnt, u32 flags) {
    if(!ctx) return EINVAL;
    if(page_cnt == 0) return ESUCCESS;
    
    page_alloc_info_t* pages = allocate_pages(ctx, page_cnt);
    if(IS_ERR_PTR(pages)) return ERR_CAST(pages);
    
    // if vaddress is 0
    // identity map the allocated pages to the physical address
    // if not already mapped
    if(vaddress == 0) {
        vaddress = (ptr_t)pages->memory;
    }
    // map the allocated pages to the virtual address

    // TODO: this code leaks memory if mapping fails, we should free the allocated pages
    // also leaks if remapping the same virtual address with different physical pages
    void* mapping_pages = nullptr;
    usize req_page_cnt = x86_map_pages_get_page_count(&ctx->ptable, vaddress, page_cnt);
    // need to allocate pages for page tables
    if(req_page_cnt) {
        page_alloc_info_t* mapping_pages_info = allocate_pages(ctx, req_page_cnt);
        if(IS_ERR_PTR(mapping_pages_info)) return ERR_CAST(mapping_pages_info);
        mapping_pages = mapping_pages_info->memory;
    }
    err_t err = x86_map_pages(&ctx->ptable, vaddress, (ptr_t)pages->memory, page_cnt, flags, mapping_pages, req_page_cnt);
    // TODO: if mapping fails, we should free the allocated pages
    if(err != ESUCCESS) return err;

    return ESUCCESS;
}

// allocate unmapped pages & map them to a virtual address
err_t pmgr_alloc_unmapped_pages(page_mgr_ctx_t* ctx, ptr_t vaddress, usize page_cnt, u32 map_flags, u32 page_flags) {
    if(!ctx) return EINVAL;
    if(page_cnt == 0) return ESUCCESS;

    page_alloc_info_t* pages = allocate_mapped_pages(ctx, map_flags, page_cnt);
    if(IS_ERR_PTR(pages)) return ERR_CAST(pages);

    // map the allocated pages to the virtual address
    void* mapping_pages = nullptr;
    usize req_page_cnt = x86_map_pages_get_page_count(&ctx->ptable, vaddress, page_cnt);
    // need to allocate pages for page tables
    if(req_page_cnt) {
        page_alloc_info_t* mapping_pages_info = allocate_pages(ctx, req_page_cnt);
        if(IS_ERR_PTR(mapping_pages_info)) return ERR_CAST(mapping_pages_info);
        mapping_pages = mapping_pages_info->memory;
    }
    err_t err = x86_map_pages(&ctx->ptable, vaddress, (ptr_t)pages->memory, page_cnt, page_flags, mapping_pages, req_page_cnt);
    if(err != ESUCCESS) return err;

    return ESUCCESS;
}

// destroy the page allocator context and free all allocated pages
err_t destroy_page_mgr_ctx(page_mgr_ctx_t* ctx) {
    // free all allocated pages
    page_alloc_info_t* curr = ctx->alloc_pages_head;
    while(curr) {
        free_pages(curr);
        curr = curr->next;
    }
    return ESUCCESS;
}
