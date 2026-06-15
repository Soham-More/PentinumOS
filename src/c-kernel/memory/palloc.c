#include "palloc.h"

typedef struct palloc_node_t {
    page_alloc_info_t page_info;
    struct palloc_node_t* next;
} palloc_node_t;

typedef struct page_alloc_ctx_t {
    heap_allocator_t* alloca;
    palloc_node_t* head;
    palloc_node_t* tail;
} page_alloc_ctx_t;

// TODO: optimize the page allocator
// by pre-allocating a large chunk of pages and then sub-allocating

// construct a page allocator context using the heap allocator
page_alloc_ctx_t* construct_page_alloc_ctx(heap_allocator_t* heap_allocator) {
    page_alloc_ctx_t* ctx = malloc(heap_allocator, sizeof(page_alloc_ctx_t));
    if(IS_INV_PTR(ctx)) return ctx;

    ctx->alloca = heap_allocator;
    ctx->head = nullptr;
    ctx->tail = nullptr;

    return ctx;
}

// allocate pages using the page allocator context
void* palloc_allocate(page_alloc_ctx_t* ctx, usize page_cnt) {
    if(ctx == nullptr) return ERR_PTR(void, EINVAL);

    const page_alloc_info_t page_info = allocate_pages(page_cnt);
    if(page_info.error != ESUCCESS) return ERR_PTR(void, page_info.error);

    palloc_node_t* node = malloc(ctx->alloca, sizeof(palloc_node_t));
    if(IS_INV_PTR(node)) {
        free_pages(&page_info);
        return node;
    }

    node->page_info = page_info;
    node->next = nullptr;

    if(ctx->head == nullptr) {
        ctx->head = node;
        ctx->tail = node;
    } else {
        ctx->tail->next = node;
        ctx->tail = node;
    }

    return page_info.memory;
}

// destroy the page allocator context and free all allocated pages
err_t destroy_page_alloc_ctx(page_alloc_ctx_t* ctx) {
    if(ctx == nullptr) return EINVAL;

    palloc_node_t* current = ctx->head;
    while(current != nullptr) {
        palloc_node_t* next = current->next;
        // TODO: manage the error case
        free_pages(&current->page_info);
        free(ctx->alloca, current);
        current = next;
    }
    free(ctx->alloca, ctx);
    return ESUCCESS;
}
