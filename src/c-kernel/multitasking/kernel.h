#pragma once

#include <includes.h>
#include <arch/paging/paging.h>
#include <memory/memory.h>
#include <hw/timer.h>

#define MAX_KERNEL_THREADS 256
#define KMT_MAX_MUTEXES 1024

#define KMT_POLICY_ROUND_ROBIN 0

typedef u16 thread_uid_t;
typedef u16 thread_mutex_t;
typedef u16 thread_rpc_t;
typedef void(*thread_entry_point_t)();

#define KMT_INVALID_KTHREAD_UID (invalid_u16)
#define KMT_IS_INVALID_UID(tid) ((u32)(tid) & 0x8000)
#define KMT_GET_ERR_UID(tid)    ((u32)(tid) & 0x7FFF)

#define KMT_IS_INVALID_MUTEX(mutex) ((u32)(mutex) & 0x8000)
#define KMT_GET_ERR_MUTEX(mutex)    ((u32)(mutex) & 0x7FFF)

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

typedef struct thread_rpc_desc_t {
    thread_uid_t caller;
    thread_uid_t callee;
    u32 function;
    void* request;
    usize request_size;
    void* response;
    usize response_size;
} thread_rpc_desc_t;

// is multitasking initialized?
bool kmt_is_initialized();
// initialize the multitasking subsystem
void initialize_multitasking(x86_mmu_map_t* handoff_ptable, heap_allocator_t* kalloca);
// make a new thread and get it's uid
thread_uid_t kmt_create_thread(const thread_desc_t* desc);
// make a new thread and get it's uid
err_t kmt_spawn_idle_thread(const thread_desc_t* desc);
// wakeup a thread that is sleeping
err_t kmt_wakeup_thread(thread_uid_t thread_id);
// put the current thread to sleep
void kmt_sleep();
// put the current thread to sleep until the specified wakeup time (in micro seconds since boot)
void kmt_sleep_until(time_us_t wakeup_time_us);
// sleep for the specified duration (in micro seconds)
void kmt_sleep_for(time_us_t sleep_duration_us);
// kill the current thread
void kmt_kill_current_thread();

// mutexes
thread_mutex_t kmt_create_mutex();

err_t kmt_lock_mutex(thread_mutex_t mutex);
err_t kmt_unlock_mutex(thread_mutex_t mutex);

err_t kmt_free_mutex(thread_mutex_t mutex);

// ipc-related functions will go here

// this function will block until the callee thread has finished executing the function and returned a result
err_t kmt_rpc_call(thread_uid_t callee, u32 function, void* request, usize request_size, void* response, usize response_size, err_t* return_code);
// listen for RPC calls from other threads, and execute the specified function when a call is received
thread_rpc_desc_t kmt_rpc_listen();
// return a response to an RPC call
err_t kmt_rpc_return(const thread_rpc_desc_t* desc, err_t return_code);

// thread info
thread_uid_t kmt_get_current_thread();
thread_uid_t kmt_get_thread(const char* name);
const char* kmt_get_thread_name(thread_uid_t thread_id);
heap_allocator_t* kmt_get_heap();

u8    kmt_get_thread_priority(thread_uid_t thread_id);
err_t kmt_override_thread_priority(thread_uid_t thread_id, u8 priority);