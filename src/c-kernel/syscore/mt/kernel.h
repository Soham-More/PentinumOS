#pragma once

#include <includes.h>
#include "../threads.h"

#include <arch/paging/paging.h>
#include <utils/heap.h>
#include <resources/timer.h>

typedef void(*thread_entry_point_t)();

typedef struct thread_desc_t {
    char* name;
    x86_mmu_map_t ptable;
    void* stack_top;
    void* interrupt_stack_top;
    thread_entry_point_t entry;
    void* heap_base;
    usize heap_size;
    u8 priority;
    u8 policy;
    u8 reserved;
} thread_desc_t;

// make a new thread and get it's uid
thread_uid_t kmt_create_thread(const thread_desc_t* desc);
// wakeup a thread that is sleeping
err_t kmt_wakeup_thread(thread_uid_t thread_id);

u8    kmt_get_thread_priority(thread_uid_t thread_id);
err_t kmt_override_thread_priority(thread_uid_t thread_id, u8 priority);