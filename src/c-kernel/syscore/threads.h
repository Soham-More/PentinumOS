#pragma once

#include <includes.h>
#include <utils/heap.h>
//#include <arch/paging/paging.h>
#include <resources/timer.h>

// mt limits
#define MAX_KERNEL_THREADS 256
#define KMT_MAX_MUTEXES 1024

// policies
#define KMT_POLICY_ROUND_ROBIN 0

// types
typedef u16 thread_uid_t;
typedef u16 thread_mutex_t;
typedef u16 thread_rpc_t;
typedef void(*thread_entry_point_t)();

// error handling macros
#define KMT_INVALID_KTHREAD_UID (invalid_u16)
#define KMT_IS_INVALID_UID(tid)     ((u32)(tid)   & 0x8000)
#define KMT_IS_INVALID_MUTEX(mutex) ((u32)(mutex) & 0x8000)
#define KMT_GET_ERR_UID(tid)        ((u32)(tid)   & 0x7FFF)
#define KMT_GET_ERR_MUTEX(mutex)    ((u32)(mutex) & 0x7FFF)

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

// check if the current thread owns the mutex
bool kmt_is_mutex_owned(thread_mutex_t mutex);

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