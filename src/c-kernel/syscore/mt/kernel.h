#pragma once

#include <includes.h>
#include "../threads.h"

#include <arch/paging/paging.h>
#include <utils/heap.h>
#include <resources/timer.h>
#include "../mem/pagemgr.h"

typedef void(*thread_entry_point_t)();

typedef struct thread_desc_t {
    char* name;
    page_mgr_ctx_t pmgr_ctx;
    void* stack_top;
    void* interrupt_stack_top;
    thread_entry_point_t entry;
    void* heap_base;
    usize heap_size;
    u8 priority;
    u8 policy;
    u8 reserved;
} thread_desc_t;

typedef struct tcb_t {
    void* esp;
    void* esp0;
    u32  cr3;
    u32  age;
    u8 status;
    u8 priority;
    u8 base_priority;
} _packed tcb_t;

typedef struct kmt_rpc_queue_node_t {
    thread_rpc_desc_t rpc_desc;
    struct kmt_rpc_queue_node_t* next;
} kmt_rpc_queue_node_t;

typedef struct thread_info_t {
    // thread info
    char name[16];
    // thread objects
    //page_alloc_ctx_t* palloca_ctx;
    heap_allocator_t* heap;
    page_mgr_ctx_t    pmgr_ctx;

    // RPC
    kmt_rpc_queue_node_t* rpc_head;
    kmt_rpc_queue_node_t* rpc_tail;
    err_t rpc_return;
    heap_allocator_t* rpc_shared_heap;
} thread_info_t;

#define STOP_PREEMPTING() u32 _kmt_flags __attribute__((cleanup(kmt_restore_flags))) = kmt_disable_preemption();
u32 kmt_disable_preemption();
void kmt_restore_flags(u32* flags);

// make a new thread and get it's uid
thread_uid_t kmt_create_thread(const thread_desc_t* desc);
// wakeup a thread that is sleeping
err_t kmt_wakeup_thread(thread_uid_t thread_id);

// get the thread's info by its uid
// WARNING: NOT THREAD-SAFE, caller should use STOP_PREEMPTING before calling
thread_info_t* kmt_get_thread_info(thread_uid_t thread_id);

u8    kmt_get_thread_priority(thread_uid_t thread_id);
err_t kmt_override_thread_priority(thread_uid_t thread_id, u8 priority);