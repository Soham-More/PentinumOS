#include "kernel.h"
#include <boot/init.h>

#include <pools.h>
#include <arch/i686.h>
#include <utils/cstdlib.h>

#include <resources/timer.h>

#include "../mem/palloc.h"
#include "../panic/panic.h"

#define THREAD_STATUS_TERMINATED ((u8)0x00)
#define THREAD_STATUS_READY      ((u8)0x01)
#define THREAD_STATUS_RUNNING    ((u8)0x02)

#define THREAD_STATUS_IDLE       ((u8)0x03)
#define THREAD_STATUS_IDLE_SLEEP ((u8)0x04)
#define THREAD_STATUS_IDLE_MUTEX ((u8)0x05)
#define THREAD_STATUS_IDLE_RPC_CALLEE   ((u8)0x06)
#define THREAD_STATUS_IDLE_RPC_CALLER   ((u8)0x07)

#define KMT_MUTEX_FREE 0x0
#define KMT_MUTEX_USED 0x1

#define KMT_PREEMPTION_ENABLED  0x1

#define KMT_TIME_SLICE_US 20
#define KMT_AGE_TICKS 10

#define STOP_PREEMPTING() volatile u32 _kmt_flags __attribute__((cleanup(kmt_restore_flags))) = g_kmt_ctx.flags; kmt_disable_preemption();

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
    page_alloc_ctx_t* palloca_ctx;
    heap_allocator_t* heap;
    x86_mmu_map_t     ptable;

    // RPC
    kmt_rpc_queue_node_t* rpc_head;
    kmt_rpc_queue_node_t* rpc_tail;
    err_t rpc_return;
} thread_info_t;

typedef struct kmt_queue_node_t {
    thread_uid_t thread_id;
    struct kmt_queue_node_t* next;
} kmt_queue_node_t;

typedef struct kmt_queue_t {
    kmt_queue_node_t* head;
    kmt_queue_node_t* tail;
} kmt_queue_t;

typedef struct kmt_mutex_impl_t {
    u16           flags;
    thread_uid_t owner;
    kmt_queue_node_t* waiting_queue_head;
    kmt_queue_node_t* waiting_queue_tail;
} kmt_mutex_impl_t;

typedef struct kmt_sleep_request_t {
    thread_uid_t thread_id;
    time_ns_t wakeup_time;
} kmt_sleep_request_t;

bool kmt_is_initialized_value = false;
struct {
    // heap
    heap_allocator_t* kalloca;

    // pools
    tcb_t tcb_pool[MAX_KERNEL_THREADS];
    thread_info_t threads[MAX_KERNEL_THREADS];
    usize thread_count;
    
    kmt_mutex_impl_t mutex_pool[KMT_MAX_MUTEXES];
    kmt_mutex_impl_t* sch_mutex;
    usize alloc_mutex_count;
    // current state
    thread_uid_t current_thread;
    thread_uid_t idle_thread;
    u32 flags;
    
    // queues
    u32 ready_priority_bitmap;
    kmt_queue_t ready_queues[31];

    // min heap for sleeping threads, sorted by wakeup_time
    kmt_sleep_request_t sleep_heap[MAX_KERNEL_THREADS];
    usize sleep_heap_size;
} g_kmt_ctx;

// WARNING: Caller is expected to disable IRQs before calling, and enable IRQs again after function returns
_import tcb_t* _asmcall kmt_switch_task(tcb_t* current_thread, tcb_t* next_thread, tss_entry_t* tss);
// this function is used to setup a valid call frame for a new thread
_import void _asmcall _kmt_setup_cf();

// helper functions
//void x86_intr_dtor(u32* eflags) { x86_restore_intr_saved(*eflags); }
void kmt_disable_preemption() { g_kmt_ctx.flags &= ~KMT_PREEMPTION_ENABLED; }
void kmt_restore_flags(u32* flags) { 
    g_kmt_ctx.flags = *flags; 
}

// code to push to the queue, expects no PREEMPTION, and caller should ensure thread_id is valid
void kmt_queue_push(kmt_queue_t* queue, thread_uid_t thread_id) {
    kmt_queue_node_t* node = malloc(g_kmt_ctx.kalloca, sizeof(kmt_queue_node_t));
    *node = (kmt_queue_node_t){ .thread_id = thread_id, .next = nullptr };
    if(queue->tail) {
        queue->tail->next = node;
        queue->tail = node;
    } else {
        queue->head = node;
        queue->tail = node;
    }
}
// code to push to the queue, expects no PREEMPTION, and caller should ensure thread_id is valid
thread_uid_t kmt_queue_pop(kmt_queue_t* queue) {
    if(!queue->head) return KMT_INVALID_KTHREAD_UID;

    kmt_queue_node_t* node = queue->head;
    thread_uid_t thread_id = node->thread_id;
    queue->head = node->next;

    if(!queue->head) queue->tail = nullptr;
    
    free(g_kmt_ctx.kalloca, node);
    return thread_id;
}
// check if the queue is empty, expects no PREEMPTION
bool kmt_queue_is_empty(const kmt_queue_t* queue) {
    return queue->head == nullptr;
}

// schedule a thread to run, and set the current thread to the specified status
// if status is THREAD_STATUS_READY, the current thread will be put back to the ready queue
void kmt_schedule(u8 new_status) {
    STOP_PREEMPTING();

    // retire the current thread if it's not already terminated
    if(g_kmt_ctx.tcb_pool[g_kmt_ctx.current_thread].status != THREAD_STATUS_TERMINATED) {
        g_kmt_ctx.tcb_pool[g_kmt_ctx.current_thread].status = THREAD_STATUS_IDLE;
        // if the thread is not the idle thread, we put it back to the ready queue
        if(new_status == THREAD_STATUS_READY) {
            // reset the age and priority of the thread, and put it back to the ready queue
            g_kmt_ctx.tcb_pool[g_kmt_ctx.current_thread].age = 0;
            g_kmt_ctx.tcb_pool[g_kmt_ctx.current_thread].priority = g_kmt_ctx.tcb_pool[g_kmt_ctx.current_thread].base_priority;

            panic_on_err(kmt_wakeup_thread(g_kmt_ctx.current_thread), "Failed to wakeup thread to reschedule");
        } else {
            g_kmt_ctx.tcb_pool[g_kmt_ctx.current_thread].status = new_status;
        }
    }

    // get the next thread from the ready queue
    panic_if(g_kmt_ctx.ready_priority_bitmap == 0, PANIC_UNEXPECTED_FAILURE, "bitmap is zero!");

    u32 max_priority = 31 - __builtin_clz(g_kmt_ctx.ready_priority_bitmap);
    thread_uid_t next_thread_id = kmt_queue_pop(&g_kmt_ctx.ready_queues[max_priority]);
    if(kmt_queue_is_empty(&g_kmt_ctx.ready_queues[max_priority])) {
        g_kmt_ctx.ready_priority_bitmap &= ~(1 << max_priority);
    }

    u32 current_thread_id = g_kmt_ctx.current_thread;
    g_kmt_ctx.current_thread = next_thread_id;
    if(g_kmt_ctx.tcb_pool[next_thread_id].status != THREAD_STATUS_READY) {
        panic(
            PANIC_UNEXPECTED_FAILURE, 
            "thread in ready queue is not ready. thread id: {u}, thread status: {x}\n", 
            next_thread_id, g_kmt_ctx.tcb_pool[next_thread_id].status
        );
        return;
    }
    g_kmt_ctx.tcb_pool[next_thread_id].status = THREAD_STATUS_RUNNING;

    kmt_switch_task(&g_kmt_ctx.tcb_pool[current_thread_id], &g_kmt_ctx.tcb_pool[next_thread_id], get_global_tss());
}

// this function is the entry point for all threads, it will call the thread's actual entry point and handle thread termination
void _asmcall kmt_thread_start(thread_entry_point_t entry_point) {
    // enable preemption
    g_kmt_ctx.flags |= KMT_PREEMPTION_ENABLED;
    
    entry_point();

    // if the thread's entry point returns, we will just mark the thread as terminated and switch back to the boot thread
    kmt_schedule(THREAD_STATUS_TERMINATED);
}

// interrupt handler for preemptive multitasking
void kmt_pop_sleep_heap() {
    if(g_kmt_ctx.sleep_heap_size == 0) return;
    if(g_kmt_ctx.sleep_heap_size == 1) {
        g_kmt_ctx.sleep_heap_size = 0;
        return;
    }

    // move the last element to the root and balance the min heap
    g_kmt_ctx.sleep_heap[0] = g_kmt_ctx.sleep_heap[g_kmt_ctx.sleep_heap_size - 1];
    g_kmt_ctx.sleep_heap_size--;

    kmt_sleep_request_t* heap = &g_kmt_ctx.sleep_heap[0];

    usize idx = 0;
    while(true) {
        usize left_child_idx = (idx << 1) + 1;
        usize right_child_idx = (idx << 1) + 2;
        usize smallest_idx = idx;

        if(left_child_idx < g_kmt_ctx.sleep_heap_size && heap[left_child_idx].wakeup_time < heap[smallest_idx].wakeup_time) {
            smallest_idx = left_child_idx;
        }
        if(right_child_idx < g_kmt_ctx.sleep_heap_size && heap[right_child_idx].wakeup_time < heap[smallest_idx].wakeup_time) {
            smallest_idx = right_child_idx;
        }
        if(smallest_idx == idx) break;

        // swap the current node with the smallest child
        kmt_sleep_request_t temp = g_kmt_ctx.sleep_heap[smallest_idx];
        g_kmt_ctx.sleep_heap[smallest_idx] = g_kmt_ctx.sleep_heap[idx];
        g_kmt_ctx.sleep_heap[idx] = temp;

        idx = smallest_idx;
    }
}
void kmt_preemptive_intr_handler(registers_t* registers) {
    if(!(g_kmt_ctx.flags & KMT_PREEMPTION_ENABLED)) return;

    // update all the threads which are sleeping
    time_ns_t current_time = timer_time_since_init_ns();
    while(g_kmt_ctx.sleep_heap_size > 0 && g_kmt_ctx.sleep_heap[0].wakeup_time <= current_time) {
        thread_uid_t thread_id = g_kmt_ctx.sleep_heap[0].thread_id;
        kmt_pop_sleep_heap();
        // downgrade the thread's status to idle, and wake it up
        g_kmt_ctx.tcb_pool[thread_id].status = THREAD_STATUS_IDLE;
        panic_on_err(kmt_wakeup_thread(thread_id), "Failed to wakeup thread from sleep heap");
    }

    // age all the threads in the ready queue, except the no priority threads (priority 0), and promote them if they have aged enough
    u32 ready_bitmap = g_kmt_ctx.ready_priority_bitmap;
    while(ready_bitmap > 1) {
        u32 priority = 31 - __builtin_clz(ready_bitmap);
        kmt_queue_node_t* prev_node = nullptr;
        kmt_queue_node_t* node = g_kmt_ctx.ready_queues[priority].head;
        while(node) {
            g_kmt_ctx.tcb_pool[node->thread_id].age++;

            if(g_kmt_ctx.tcb_pool[node->thread_id].age >= KMT_AGE_TICKS) {
                // if the thread has aged enough, we will promote it to the next priority level
                g_kmt_ctx.tcb_pool[node->thread_id].age = 0;
                g_kmt_ctx.tcb_pool[node->thread_id].priority++;
                if(g_kmt_ctx.tcb_pool[node->thread_id].priority >= 31) g_kmt_ctx.tcb_pool[node->thread_id].priority = 30;
                // remove from the current queue and add it to the next priority queue
                if(prev_node) {
                    prev_node->next = node->next;
                } else {
                    g_kmt_ctx.ready_queues[priority].head = node->next;
                }
                g_kmt_ctx.tcb_pool[node->thread_id].status = THREAD_STATUS_IDLE;
                panic_on_err(kmt_wakeup_thread(node->thread_id), "Failed to raise thread priority from ready queue");
                // we just removed the current node from the list
                node = prev_node;
            }

            prev_node = node;
            node = node->next;
        }
        ready_bitmap &= ~(1 << priority);
    }

    kmt_schedule(THREAD_STATUS_READY);
}

// api;
bool kmt_is_initialized() { return kmt_is_initialized_value; }
void initialize_multitasking(x86_mmu_map_t* handoff_ptable, heap_allocator_t* kalloca) {
    g_kmt_ctx.kalloca = construct_heap(kalloca, __kmt_heap_start, PTR_DIFF_I32(__kmt_heap_end, __kmt_heap_start));
    // add the boot thread as the first thread in the thread pool
    g_kmt_ctx.thread_count = 1;
    
    memcpy(g_kmt_ctx.threads[0].name, "kboot", sizeof("kboot"));
    g_kmt_ctx.threads[0].ptable = *handoff_ptable;
    // the boot thread is not a real thread, it will never be switched into, so we don't need to worry about its heap
    g_kmt_ctx.threads[0].heap = nullptr;
    g_kmt_ctx.threads[0].palloca_ctx = nullptr;
    g_kmt_ctx.threads[0].rpc_head = nullptr;
    g_kmt_ctx.threads[0].rpc_tail = nullptr;

    g_kmt_ctx.tcb_pool[0] = (tcb_t){
        // the boot thread stack is already set up by the bootloader
        // the boot thread cannot be switched into, so we don't need to worry about its stack or registers
        .esp = nullptr,
        .esp0 = __idle_thread_intr_stack_end - 4,
        .cr3 = x86_get_ctx_map(handoff_ptable),
        .status = THREAD_STATUS_RUNNING,
    };
    g_kmt_ctx.current_thread = 0;

    // setup the ready queues
    g_kmt_ctx.ready_priority_bitmap = 0;
    for(usize i = 0; i < 31; i++) {
        g_kmt_ctx.ready_queues[i].head = nullptr;
        g_kmt_ctx.ready_queues[i].tail = nullptr;
    }

    for(usize i = 0; i < KMT_MAX_MUTEXES; i++) {
        g_kmt_ctx.mutex_pool[i].flags = KMT_MUTEX_FREE;
        g_kmt_ctx.mutex_pool[i].owner = invalid_u16;
        g_kmt_ctx.mutex_pool[i].waiting_queue_head = nullptr;
        g_kmt_ctx.mutex_pool[i].waiting_queue_tail = nullptr;
    }

    // setup sleeping thread tracking
    g_kmt_ctx.sleep_heap_size = 0;
    for(usize i = 0; i < MAX_KERNEL_THREADS; i++) {
        g_kmt_ctx.sleep_heap[i].thread_id = invalid_u16;
        g_kmt_ctx.sleep_heap[i].wakeup_time = 0;
    }

    g_kmt_ctx.idle_thread = invalid_u16;
    kmt_is_initialized_value = true;

    timer_setup_callback((1000 * 1000) / KMT_TIME_SLICE_US, kmt_preemptive_intr_handler);

    // preemptive multitasking is enabled when idle task is setup
}

thread_uid_t kmt_create_thread(const thread_desc_t *desc) {
    STOP_PREEMPTING();

    if(g_kmt_ctx.thread_count >= MAX_KERNEL_THREADS) return 0x8000 | EPOOLFULL;
    if(strlen(desc->name) >= sizeof(g_kmt_ctx.threads[0].name)) return 0x8000 | ESTRTOOBIG;
    
    // validate the thread descriptor
    if(desc->priority >= 31) return 0x8000 | EINVAL;
    if(desc->heap_size == 0) return 0x8000 | EINVAL;
    if(IS_ERR_PTR(desc->entry))                 return 0x8000 | EINVPTR;
    if(IS_ERR_PTR(desc->stack_top))             return 0x8000 | EINVPTR;
    if(IS_ERR_PTR(desc->interrupt_stack_top))   return 0x8000 | EINVPTR;
    if(IS_ERR_PTR(desc->ptable.directory))      return 0x8000 | EINVPTR;
    if(IS_ERR_PTR(desc->heap_base))             return 0x8000 | EINVPTR;

    thread_uid_t new_thread_id = g_kmt_ctx.thread_count;
    
    memcpy(g_kmt_ctx.threads[new_thread_id].name, desc->name, strlen(desc->name) + 1);
    g_kmt_ctx.threads[new_thread_id].ptable = desc->ptable;
    g_kmt_ctx.threads[new_thread_id].heap = construct_heap(g_kmt_ctx.kalloca, desc->heap_base, desc->heap_size);
    g_kmt_ctx.threads[new_thread_id].palloca_ctx = construct_page_alloc_ctx(g_kmt_ctx.threads[new_thread_id].heap);
    g_kmt_ctx.threads[new_thread_id].rpc_head = nullptr;
    g_kmt_ctx.threads[new_thread_id].rpc_tail = nullptr;

    if(IS_ERR_PTR(g_kmt_ctx.threads[new_thread_id].heap)) {
        return 0x8000 | ERR_CAST(g_kmt_ctx.threads[new_thread_id].heap);
    }

    // the thread will be valid after this point, so we can increment the thread count
    g_kmt_ctx.thread_count++;

    // push the correct values onto the new thread's stack so that when we switch to it, it will start executing at the entry point with a clean stack
	u32* stack_esp = desc->stack_top;
	*stack_esp = 0xDEADBEEF;            stack_esp--; // stack canary
    *stack_esp = (u32)desc->entry;      stack_esp--; // the thread entry point
	*stack_esp = (u32)kmt_thread_start; stack_esp--; // the thread start function
    *stack_esp = (u32)_kmt_setup_cf;    stack_esp--; // the thread setup function
	*stack_esp = 0; 			        stack_esp--; // ebx is 0(for entry point)
	*stack_esp = 0; 			        stack_esp--; // esi is 0(for entry point)
	*stack_esp = 0; 			        stack_esp--; // edi is 0(for entry point)
	*stack_esp = (u32)desc->stack_top; // ebp is the stack top (for entry point)

    g_kmt_ctx.tcb_pool[new_thread_id] = (tcb_t){
        .esp = stack_esp,
        .esp0 = desc->interrupt_stack_top,
        .cr3 = x86_get_ctx_map(&desc->ptable),
        .status = THREAD_STATUS_IDLE,
        .priority = desc->priority,
        .base_priority = desc->priority,
    };

    return new_thread_id;
}

void kmt_spawn_idle_thread(thread_entry_point_t entry, x86_mmu_map_t ptable) {
    kmt_disable_preemption();

    if(g_kmt_ctx.idle_thread != invalid_u16) return;

    g_kmt_ctx.idle_thread = kmt_create_thread(&(thread_desc_t){
		.name = "kidle",
		.ptable = ptable,
		.stack_top = __idle_thread_exec_stack_end - 4,
		.interrupt_stack_top = __idle_thread_intr_stack_end - 4,
		.entry = entry,
		.heap_base = __idle_heap_start,
		.heap_size = PTR_DIFF_I32(__idle_heap_start, __idle_heap_end),
		.priority = 1,
		.policy = KMT_POLICY_ROUND_ROBIN,
	});
    panic_if(KMT_IS_INVALID_UID(g_kmt_ctx.idle_thread), KMT_GET_ERR_UID(g_kmt_ctx.idle_thread), "Failed to create idle thread");

    // wakeup the idle thread
    kmt_wakeup_thread(g_kmt_ctx.idle_thread);
    // enable preemption
    g_kmt_ctx.flags |= KMT_PREEMPTION_ENABLED;
    // finally, we can kill the thread
    kmt_kill_current_thread();
    return ESUCCESS;
}

err_t kmt_wakeup_thread(thread_uid_t thread_id) {
    STOP_PREEMPTING();

    // sanitize thread_id
    if(thread_id >= g_kmt_ctx.thread_count) return EINVAL;

    // if the thread is already ready/running, we don't need to do anything
    if(g_kmt_ctx.tcb_pool[thread_id].status == THREAD_STATUS_READY) return ESUCCESS;
    else if(g_kmt_ctx.tcb_pool[thread_id].status == THREAD_STATUS_RUNNING) return ESUCCESS;
    else if(g_kmt_ctx.tcb_pool[thread_id].status == THREAD_STATUS_TERMINATED) return ETERMINATED;

    // TODO: deal with waking up a sleeping thread

    // add the thread to the correct ready queue
    u8 priority = g_kmt_ctx.tcb_pool[thread_id].priority;
    kmt_queue_push(&g_kmt_ctx.ready_queues[priority], thread_id);
    g_kmt_ctx.ready_priority_bitmap |= (1 << priority);

    // mark the thread as ready
    g_kmt_ctx.tcb_pool[thread_id].status = THREAD_STATUS_READY;
    
    return ESUCCESS;
}
void kmt_sleep() {
    STOP_PREEMPTING();
    kmt_schedule(THREAD_STATUS_IDLE);
}
void kmt_sleep_until(time_us_t wakeup_time_us) {
    STOP_PREEMPTING();

    // add the current thread to the sleep min heap
    if(g_kmt_ctx.sleep_heap_size >= MAX_KERNEL_THREADS) {
        panic(PANIC_OBJ_POOL_FULL, "object pool full");
        return;
    }

    // the wakeup time is in the past, we don't need to sleep
    if(timer_time_since_init_ns() >= wakeup_time_us * 1000) return;

    usize idx = g_kmt_ctx.sleep_heap_size;
    g_kmt_ctx.sleep_heap[idx].thread_id = g_kmt_ctx.current_thread;
    g_kmt_ctx.sleep_heap[idx].wakeup_time = wakeup_time_us * 1000;
    g_kmt_ctx.sleep_heap_size++;

    // balance the min heap
    while(idx > 0) {
        usize parent_idx = (idx - 1) >> 1;
        if(g_kmt_ctx.sleep_heap[parent_idx].wakeup_time <= g_kmt_ctx.sleep_heap[idx].wakeup_time) break;

        // swap the current node with its parent
        kmt_sleep_request_t temp = g_kmt_ctx.sleep_heap[parent_idx];
        g_kmt_ctx.sleep_heap[parent_idx] = g_kmt_ctx.sleep_heap[idx];
        g_kmt_ctx.sleep_heap[idx] = temp;

        idx = parent_idx;
    }

    kmt_schedule(THREAD_STATUS_IDLE_SLEEP);
}
void kmt_sleep_for(time_us_t sleep_duration_us) {
    time_us_t wakeup_time = div_ceil(timer_time_since_init_ns(), 1000) + sleep_duration_us;
    kmt_sleep_until(wakeup_time);
}
void kmt_kill_current_thread() {
    STOP_PREEMPTING();
    g_kmt_ctx.tcb_pool[g_kmt_ctx.current_thread].status = THREAD_STATUS_TERMINATED;
    kmt_schedule(false);
}
// mutexes
thread_mutex_t kmt_create_mutex() {
    STOP_PREEMPTING();

    usize mutex_id = g_kmt_ctx.alloc_mutex_count;
    if(g_kmt_ctx.alloc_mutex_count >= KMT_MAX_MUTEXES) {
        // search for a free mutex in the pool
        for(usize i = 0; i < KMT_MAX_MUTEXES; i++) {
            if(g_kmt_ctx.mutex_pool[i].flags == KMT_MUTEX_FREE) {
                mutex_id = i;
                break;
            }
        }
        if(mutex_id >= KMT_MAX_MUTEXES) { return 0x8000 | EPOOLFULL; }
    } else {
        g_kmt_ctx.alloc_mutex_count++;
    }

    g_kmt_ctx.mutex_pool[mutex_id].flags = KMT_MUTEX_USED;
    g_kmt_ctx.mutex_pool[mutex_id].owner = invalid_u16;
    g_kmt_ctx.mutex_pool[mutex_id].waiting_queue_head = nullptr;
    g_kmt_ctx.mutex_pool[mutex_id].waiting_queue_tail = nullptr;

    return mutex_id;
}
err_t kmt_lock_mutex(thread_mutex_t mutex) {
    STOP_PREEMPTING();

    kmt_mutex_impl_t* mutex_impl = &g_kmt_ctx.mutex_pool[mutex];
    if(mutex_impl->flags == KMT_MUTEX_FREE) return EUSEFREED;
    if(mutex_impl->owner == g_kmt_ctx.current_thread) return ESUCCESS;
    if(mutex_impl->owner == invalid_u16) {
        mutex_impl->owner = g_kmt_ctx.current_thread;
        return ESUCCESS;
    }
    // add the current thread to the mutex's waiting queue
    kmt_queue_node_t* new_queue_node = (kmt_queue_node_t*)malloc(g_kmt_ctx.kalloca, sizeof(kmt_queue_node_t));
    new_queue_node->thread_id = g_kmt_ctx.current_thread;
    new_queue_node->next = nullptr;
    if(mutex_impl->waiting_queue_tail) {
        mutex_impl->waiting_queue_tail->next = new_queue_node;
        mutex_impl->waiting_queue_tail = new_queue_node;
    } else {
        mutex_impl->waiting_queue_head = new_queue_node;
        mutex_impl->waiting_queue_tail = new_queue_node;
    }

    while(mutex_impl->owner != g_kmt_ctx.current_thread) {
        kmt_schedule(THREAD_STATUS_IDLE_MUTEX);
    }
    return ESUCCESS;
}
err_t kmt_unlock_mutex(thread_mutex_t mutex) {
    STOP_PREEMPTING();

    kmt_mutex_impl_t* mutex_impl = &g_kmt_ctx.mutex_pool[mutex];
    if(mutex_impl->flags == KMT_MUTEX_FREE) return EUSEFREED;
    if(mutex_impl->owner != g_kmt_ctx.current_thread) return ENOTOWNED;
    if(mutex_impl->waiting_queue_head) {
        // wake up the next thread in the waiting queue,
        // which is not terminated
        kmt_queue_node_t* next_thread_node = nullptr;
        do {
            next_thread_node = mutex_impl->waiting_queue_head;
            mutex_impl->waiting_queue_head = next_thread_node->next;
            if(mutex_impl->waiting_queue_head == nullptr) {
                mutex_impl->waiting_queue_tail = nullptr;
            }
        } while(g_kmt_ctx.tcb_pool[next_thread_node->thread_id].status != THREAD_STATUS_TERMINATED);

        mutex_impl->owner = next_thread_node->thread_id;
        // downgrade the thread's status to idle, and wake it up
        g_kmt_ctx.tcb_pool[next_thread_node->thread_id].status = THREAD_STATUS_IDLE;
        panic_on_err(kmt_wakeup_thread(next_thread_node->thread_id), "Failed to wakeup thread from mutex waiting queue");
        free(g_kmt_ctx.kalloca, next_thread_node);
    } else {
        mutex_impl->owner = invalid_u16;
    }
    return ESUCCESS;
}

bool kmt_is_mutex_owned(thread_mutex_t mutex) {
    STOP_PREEMPTING();

    kmt_mutex_impl_t* mutex_impl = &g_kmt_ctx.mutex_pool[mutex];
    if(mutex_impl->flags == KMT_MUTEX_FREE) return false;
    return mutex_impl->owner == g_kmt_ctx.current_thread;
}

err_t kmt_free_mutex(thread_mutex_t mutex) {
    STOP_PREEMPTING();

    if(g_kmt_ctx.mutex_pool[mutex].flags == KMT_MUTEX_FREE) return EINVAL;
    if(g_kmt_ctx.mutex_pool[mutex].owner != invalid_u16) return EINUSE;

    kmt_mutex_impl_t* mutex_impl = &g_kmt_ctx.mutex_pool[mutex];
    mutex_impl->flags = KMT_MUTEX_FREE;
    mutex_impl->owner = invalid_u16;

    return ESUCCESS;
}

// RPC
err_t kmt_rpc_call(thread_uid_t callee, u32 function, void* request, usize request_size, void* response, usize response_size, err_t* return_code) {
    STOP_PREEMPTING();

    // sanitize the callee thread id
    if(callee >= g_kmt_ctx.thread_count) return EOUTOFRANGE;
    // validate the request and response buffers
    if(request_size > 0 && IS_ERR_PTR(request)) return EINVPTR;
    if(response_size > 0 && IS_ERR_PTR(response)) return EINVPTR;
    if(IS_ERR_PTR(return_code)) return EINVPTR;

    // add the RPC request to the callee's RPC queue
    kmt_rpc_queue_node_t* new_rpc_node = (kmt_rpc_queue_node_t*)malloc(g_kmt_ctx.kalloca, sizeof(kmt_rpc_queue_node_t));
    new_rpc_node->rpc_desc = (thread_rpc_desc_t){
        .caller = g_kmt_ctx.current_thread,
        .callee = callee,
        .function = function,
        .request = request,
        .request_size = request_size,
        .response = response,
        .response_size = response_size,
    };
    new_rpc_node->next = nullptr;
    if(g_kmt_ctx.threads[callee].rpc_tail) {
        g_kmt_ctx.threads[callee].rpc_tail->next = new_rpc_node;
        g_kmt_ctx.threads[callee].rpc_tail = new_rpc_node;
    } else {
        g_kmt_ctx.threads[callee].rpc_head = new_rpc_node;
        g_kmt_ctx.threads[callee].rpc_tail = new_rpc_node;
        // the callee's RPC queue is empty, we will wake it up if it's idle
        if(g_kmt_ctx.tcb_pool[callee].status == THREAD_STATUS_IDLE_RPC_CALLEE) {
            g_kmt_ctx.tcb_pool[callee].status = THREAD_STATUS_IDLE;
            panic_on_err(kmt_wakeup_thread(callee), "Failed to wakeup thread from idle RPC");
        } else if(g_kmt_ctx.tcb_pool[callee].status == THREAD_STATUS_TERMINATED) {
            panic(
                PANIC_UNEXPECTED_FAILURE, 
                "Callee thread has been terminated. thread id: {u}, thread status: {x}\n", 
                callee, g_kmt_ctx.tcb_pool[callee].status
            );
        }
    }

    // wait for the callee to process the RPC request
    g_kmt_ctx.threads[g_kmt_ctx.current_thread].rpc_return = EPENDING;
    while(g_kmt_ctx.threads[g_kmt_ctx.current_thread].rpc_return == EPENDING) {
        kmt_schedule(THREAD_STATUS_IDLE_RPC_CALLER);
    }
    *return_code = g_kmt_ctx.threads[g_kmt_ctx.current_thread].rpc_return;
    return ESUCCESS;
}
thread_rpc_desc_t kmt_rpc_listen() {
    STOP_PREEMPTING();

    kmt_rpc_queue_node_t* rpc_node = g_kmt_ctx.threads[g_kmt_ctx.current_thread].rpc_head;
    if(!rpc_node) {
        // no RPC requests, we will just sleep until we get one
        kmt_schedule(THREAD_STATUS_IDLE_RPC_CALLEE);
        rpc_node = g_kmt_ctx.threads[g_kmt_ctx.current_thread].rpc_head;
        panic_if(IS_ERR_PTR(rpc_node), PANIC_UNEXPECTED_FAILURE, "RPC queue is empty after waking up from idle RPC");
    }

    // remove the RPC request from the queue
    g_kmt_ctx.threads[g_kmt_ctx.current_thread].rpc_head = rpc_node->next;
    if(!g_kmt_ctx.threads[g_kmt_ctx.current_thread].rpc_head) {
        g_kmt_ctx.threads[g_kmt_ctx.current_thread].rpc_tail = nullptr;
    }

    thread_rpc_desc_t rpc_desc = rpc_node->rpc_desc;
    free(g_kmt_ctx.kalloca, rpc_node);
    return rpc_desc;
}
err_t kmt_rpc_return(const thread_rpc_desc_t* desc, err_t return_code) {
    STOP_PREEMPTING();

    // validate the RPC descriptor
    if(IS_ERR_PTR(desc)) return EINVPTR;
    if(desc->caller >= g_kmt_ctx.thread_count) return EOUTOFRANGE;
    if(desc->callee != g_kmt_ctx.current_thread) return EINVAL;
    if(desc->request_size > 0 && IS_ERR_PTR(desc->request)) return EINVPTR;
    if(desc->response_size > 0 && IS_ERR_PTR(desc->response)) return EINVPTR;
    if(return_code == EPENDING) return EINVAL;

    // set the return code for the caller
    g_kmt_ctx.threads[desc->caller].rpc_return = return_code;

    // assuming the buffers have been filled by the callee, we will wake up the caller
    panic_if(
        g_kmt_ctx.tcb_pool[desc->caller].status != THREAD_STATUS_IDLE_RPC_CALLER, 
        PANIC_UNEXPECTED_FAILURE, "Caller thread is not in idle RPC caller state"
    );
    g_kmt_ctx.tcb_pool[desc->caller].status = THREAD_STATUS_IDLE;
    panic_on_err(kmt_wakeup_thread(desc->caller), "Failed to wakeup thread from RPC return");
    return ESUCCESS;
}

// thread info
thread_uid_t kmt_get_current_thread() { return g_kmt_ctx.current_thread; }
thread_uid_t kmt_get_thread(const char *name) {
    if(strlen(name) >= sizeof(g_kmt_ctx.threads[0].name)) return 0x8000 | ESTRTOOBIG;
    for(thread_uid_t i = 0; i < g_kmt_ctx.thread_count; i++) {
        if(strcmp(g_kmt_ctx.threads[i].name, name, strlen(name))) {
            return i;
        }
    }
    return 0x8000 | ENOTFOUND;
}
const char *kmt_get_thread_name(thread_uid_t thread_id)
{
    if(thread_id >= g_kmt_ctx.thread_count) return ERR_PTR(const char, EOUTOFRANGE);
    return g_kmt_ctx.threads[thread_id].name;
}
heap_allocator_t* kmt_get_heap() {
    thread_uid_t thread_id = g_kmt_ctx.current_thread;
    if(thread_id >= g_kmt_ctx.thread_count) return ERR_PTR(heap_allocator_t, EOUTOFRANGE);
    return g_kmt_ctx.threads[thread_id].heap;
}
page_alloc_ctx_t* kmt_get_page_alloc_ctx() {
    thread_uid_t thread_id = g_kmt_ctx.current_thread;
    if(thread_id >= g_kmt_ctx.thread_count) return ERR_PTR(page_alloc_ctx_t, EOUTOFRANGE);
    return g_kmt_ctx.threads[thread_id].palloca_ctx;
}

u8   kmt_get_thread_priority(thread_uid_t thread_id) {
    if(thread_id >= g_kmt_ctx.thread_count) return invalid_u8;
    return g_kmt_ctx.tcb_pool[thread_id].priority;
}
err_t kmt_override_thread_priority(thread_uid_t thread_id, u8 priority) {
    if(thread_id >= g_kmt_ctx.thread_count) return EOUTOFRANGE;
    g_kmt_ctx.tcb_pool[thread_id].priority = priority;
    return ESUCCESS;
}


