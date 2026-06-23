#include "syscore.h"

#include <panic/panic.h>
#include <utils/heap.h>
#include <pools.h>

#include "mt/kernel.h"
#include "threads.h"

#define SYSCORE_FUNC_ECHO 0x0
#define SYSCORE_FUNC_ALLOC_PAGES 0x1

#define PAGE_ON_RAM 0x1
#define PAGE_MMIO   0x2

typedef struct page_alloc_request_t {
    usize num_pages;
    ptr_t vaddress;
    u32 flags;
    u32 page_flags;
} page_alloc_request_t;

thread_uid_t syscore_uid = KMT_INVALID_KTHREAD_UID;
x86_mmu_map_t template_ptable;
heap_allocator_t* ptable_heap = nullptr;
page_mgr_ctx_t* syscore_pmgr_ctx = nullptr;

void* syscore_get_stack_top(void* pages, usize page_count) {
    return (void*)(((u8*)pages) + page_count * X86_PAGE_SIZE - 4);
}

err_t syscore_spawn_thread(char* name, thread_entry_point_t entry_point, u8 priority) {
    page_alloc_info_t* pages = allocate_pages(syscore_pmgr_ctx, 1);
    if(IS_ERR_PTR(pages)) {return ERR_CAST(pages);}

    x86_mmu_map_t ptable = x86_copy_pagetable(pages->memory, &template_ptable);
    page_mgr_ctx_t pmgr_ctx = construct_page_mgr_ctx(ptable_heap, ptable);
    
    page_ptr_t page_pool = (page_ptr_t)0x7FFF0000;
    err_t err = pmgr_alloc_pages(&pmgr_ctx, (u32)page_pool,
        4 + // stack pages
        4 + // interrupt stack pages
        8, 
        X86_PAGE_PRESENT | X86_PAGE_RW
    );
    if(err != ESUCCESS) {
        return err;
    }

    thread_desc_t thread_desc = (thread_desc_t){
        .name = name,
        .entry = entry_point,
        .pmgr_ctx = pmgr_ctx,
        .stack_top = syscore_get_stack_top(&page_pool[0], 4),
        .interrupt_stack_top = syscore_get_stack_top(&page_pool[4], 4),
        .heap_base = (void*)&page_pool[8],
        .heap_size = 8 * X86_PAGE_SIZE,
        .priority = priority,
        .policy = KMT_POLICY_ROUND_ROBIN,
    };

    thread_uid_t uid = kmt_create_thread(&thread_desc);
    if(KMT_IS_INVALID_UID(uid)) {
        return KMT_GET_ERR_UID(uid);
    }
    err = kmt_wakeup_thread(uid);
    if(err != ESUCCESS) {
        return err;
    }
    return ESUCCESS;
}

void test(){
    log_info("syscore test function run!\n");

    // get some pages from the syscore page manager context
    panic_on_err(syscore_alloc_pages(4, 0x08000000), "Failed to allocate pages from syscore page manager context");

    // test read/write to the allocated pages
    u32* test_page = (u32*)0x08000000;
    for(u32 i = 0; i < 1024; i++) {
        test_page[i] = i;
    }
    for(u32 i = 0; i < 1024; i++) {
        if(test_page[i] != i) {
            panic(PANIC_ASSERTION_FAILED, "syscore test failed: test_page[{u32}] = {u32}, expected {u32}", i, test_page[i], i); 
        }
    }

    log_debug("rw syscore test passed!\n");

    for(;;);
}

void syscore_thread_entry() {
    {
        log_info("syscore thread started successfully\n");
        // setup the syscore thread's page manager context to use the new heap
        {
            STOP_PREEMPTING();
            thread_info_t* syscore_thread = kmt_get_thread_info(syscore_uid);
            syscore_pmgr_ctx = &syscore_thread->pmgr_ctx;
    
            thread_uid_t idle_id = kmt_get_thread("kidle");
            panic_if(KMT_IS_INVALID_UID(idle_id), KMT_GET_ERR_UID(idle_id), "Failed to get idle thread");
            err_t err = kmt_override_thread_priority(idle_id, 0);
            panic_on_err(err, "Failed to override idle thread priority");
        }
        log_info("syscore handoff completed\n");
        
        usize heap_size_pages = 4;
        page_alloc_info_t* heapinfo = allocate_pages(syscore_pmgr_ctx, heap_size_pages);
        panic_on_err_ptr(heapinfo, "Failed to allocate heap for syscore thread");
    
        ptable_heap = initialize_heap(kmt_get_heap(), heapinfo->memory, heap_size_pages * X86_PAGE_SIZE);
        syscore_pmgr_ctx->heap_allocator = ptable_heap;
    
        log_info("setting up page manager... done\n");
        
        // this must be last
        template_ptable = make_template_page_table();
        log_info("building template page table... done\n");
    }

    syscore_spawn_thread("test", test, 2);

    // setup some test rpc
    while(true) {
        thread_rpc_desc_t rpc = kmt_rpc_listen();
        //log_info("received rpc from caller {u16} to callee {u16} with function {u32}\n", rpc.caller, rpc.callee, rpc.function);

        if(rpc.function == SYSCORE_FUNC_ECHO) {
            log_info("ECHO: recv=\"{S}\"\n", rpc.request, rpc.request_size);

            if(rpc.request_size != rpc.response_size) {
                log_info("request size {usize} does not match response size {usize}, returning error code\n", rpc.request_size, rpc.response_size);
                panic_on_err(kmt_rpc_return(&rpc, EINVAL), "Failed to return rpc response");
                continue;
            }
            if(IS_ERR_PTR(rpc.request) || IS_ERR_PTR(rpc.response)) {
                log_info("request/response buffer is/are invalid, returning error code\n");
                panic_on_err(kmt_rpc_return(&rpc, EINVPTR), "Failed to return rpc response");
                continue;
            }

            memcpy(rpc.response, rpc.request, rpc.request_size);

            panic_on_err(kmt_rpc_return(&rpc, ESUCCESS), "Failed to return rpc response");
        } else if(rpc.function == SYSCORE_FUNC_ALLOC_PAGES) {
            if(rpc.request_size != sizeof(page_alloc_request_t)) {
                log_info("invalid page alloc request size {usize}, returning error code\n", rpc.request_size);
                panic_on_err(kmt_rpc_return(&rpc, EINVAL), "Failed to return rpc response");
                continue;
            }
            if(IS_ERR_PTR(rpc.request)) {
                log_info("request buffer is invalid, returning error code\n");
                panic_on_err(kmt_rpc_return(&rpc, EINVPTR), "Failed to return rpc response");
                continue;
            }

            page_alloc_request_t* request = (page_alloc_request_t*)rpc.request;
            log_info("PAGE_ALLOCATOR: num_pages={usize}, vaddress={p}, flags={x}\n", request->num_pages, request->vaddress, request->flags);

            
            err_t err = ESUCCESS;
            if(request->flags & PAGE_ON_RAM) {
                STOP_PREEMPTING();
                thread_info_t* caller = kmt_get_thread_info(rpc.caller);
                log_mmu_map(&caller->pmgr_ctx.ptable);
                err = pmgr_alloc_pages(&caller->pmgr_ctx, request->vaddress, request->num_pages, X86_PAGE_PRESENT | X86_PAGE_RW);
                log_mmu_map(&caller->pmgr_ctx.ptable);
            } else if(request->flags & PAGE_MMIO) {
                STOP_PREEMPTING();
                thread_info_t* caller = kmt_get_thread_info(rpc.caller);
                err = pmgr_alloc_pages(&caller->pmgr_ctx, request->vaddress, request->num_pages, X86_PAGE_PRESENT | request->page_flags);
            } else {
                log_info("invalid page alloc flags {x}, returning error code\n", request->flags);
                panic_on_err(kmt_rpc_return(&rpc, EINVAL), "Failed to return rpc response");
                    continue;
            }
            panic_on_err(kmt_rpc_return(&rpc, err), "Failed to return rpc response");
        } else {
            log_info("unknown rpc function {u32}, returning error code\n", rpc.function);
            panic_on_err(kmt_rpc_return(&rpc, EUNKNOWNREQ), "Failed to return rpc response");
        }
    }
}

err_t syscore_start_thread(x86_mmu_map_t page_table) {
    syscore_uid = kmt_get_thread("syscore");
    if(!KMT_IS_INVALID_UID(syscore_uid)) return EEXISTS;

    syscore_uid = kmt_create_thread(&(thread_desc_t){
        .name = "syscore",
        .entry = syscore_thread_entry,
        .pmgr_ctx = construct_page_mgr_ctx(nullptr, page_table),
        .stack_top = __syscore_thread_exec_stack_end - 4,
        .interrupt_stack_top = __syscore_thread_intr_stack_end - 4,
        .heap_base = __syscore_heap_start,
        .heap_size = PTR_DIFF_I32(__syscore_heap_start, __syscore_heap_end),
        .priority = 6,
        .policy = KMT_POLICY_ROUND_ROBIN,
    });
    if(KMT_IS_INVALID_UID(syscore_uid)) {
        return KMT_GET_ERR_UID(syscore_uid);
    }
    err_t err = kmt_wakeup_thread(syscore_uid);
    if(err != ESUCCESS) {
        return err;
    }
    return ESUCCESS;
}
err_t syscore_echo_test(const char* test) {
    usize len = strlen(test) + 1;
    void* response_data = malloc(kmt_get_heap(), len);
    char* request_data = (char*)malloc(kmt_get_heap(), len);
    memcpy(request_data, test, len);

    err_t rpc_err = EPENDING;
    err_t err = kmt_rpc_call(syscore_uid, SYSCORE_FUNC_ECHO, request_data, len, response_data, len, &rpc_err);
    if(err != ESUCCESS) {
        goto end;
    }
    if(rpc_err != ESUCCESS) {
        err = rpc_err;
        goto end;
    }

    // check if the response data matches the request data
    if(!strcmp(test, response_data, len)) {
        err = EINVAL;
        goto end;
    }

    end:
    //log_info("received response from syscore thread: {s}\n", response_data);
    free(kmt_get_heap(), response_data);
    return err;
}
err_t syscore_alloc_pages(usize num_pages, ptr_t vaddress) {
    if(num_pages == 0) return ESUCCESS;
    page_alloc_request_t* request = malloc(kmt_get_rpc_heap(), sizeof(page_alloc_request_t));
    *request = (page_alloc_request_t){
        .num_pages = num_pages,
        .vaddress = vaddress,
        .flags = PAGE_ON_RAM,
        .page_flags = X86_PAGE_PRESENT | X86_PAGE_RW,
    };
    
    err_t rpc_err = EPENDING;
    err_t err = kmt_rpc_call(syscore_uid, SYSCORE_FUNC_ALLOC_PAGES, request, sizeof(page_alloc_request_t), nullptr, 0, &rpc_err);
    if(err != ESUCCESS) {
        goto end;
    }
    if(rpc_err != ESUCCESS) {
        err = ERPC | rpc_err;
        goto end;
    }
end:
    free(kmt_get_rpc_heap(), request);
    return ESUCCESS;

}
err_t syscore_alloc_mmio_pages(usize num_pages, ptr_t vaddress, u32 page_flags) {
    page_alloc_request_t* request = malloc(kmt_get_rpc_heap(), sizeof(page_alloc_request_t));
    *request = (page_alloc_request_t){
        .num_pages = num_pages,
        .vaddress = vaddress,
        .flags = PAGE_MMIO,
        .page_flags = page_flags,
    };
    
    err_t rpc_err = EPENDING;
    err_t err = kmt_rpc_call(syscore_uid, SYSCORE_FUNC_ALLOC_PAGES, request, sizeof(page_alloc_request_t), nullptr, 0, &rpc_err);
    if(err != ESUCCESS) {
        goto end;
    }
    if(rpc_err != ESUCCESS) {
        err = ERPC | rpc_err;
        goto end;
    }
end:
    free(kmt_get_rpc_heap(), request);
    return ESUCCESS;
}