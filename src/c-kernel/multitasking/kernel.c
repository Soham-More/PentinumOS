#include "kernel.h"

#include <pools.h>
#include <arch/i686.h>
#include <c-utils/base.h>

#define KTHREAD_STATUS_READY      0x0
#define KTHREAD_STATUS_RUNNING    0x1
#define KTHREAD_STATUS_IDLE       0x2
#define KTHREAD_STATUS_TERMINATED 0x3

typedef struct ktcb_t {
    void* esp;
    void* esp0;
    u32  cr3;
    u8 status;
} _packed ktcb_t;

typedef struct kthread_t {
    char name[16];
    x86_mmu_map_t ptable;
} kthread_t;

struct {
    ktcb_t tcb_pool[MAX_KERNEL_THREADS];
    kthread_t threads[MAX_KERNEL_THREADS];
    heap_allocator_t* kalloca;
    usize thread_count;
    kthread_uid_t current_thread;
} g_kmt_ctx;

// WARNING: Caller is expected to disable IRQs before calling, and enable IRQs again after function returns
_import ktcb_t* _asmcall kmt_switch_task(ktcb_t* current_thread, ktcb_t* next_thread, tss_entry_t* tss);

void initialize_multitasking(x86_mmu_map_t* handoff_ptable) {
    // add the boot thread as the first thread in the thread pool
    g_kmt_ctx.thread_count = 1;
    
    memcpy(g_kmt_ctx.threads[0].name, "init", sizeof("init"));
    g_kmt_ctx.threads[0].ptable = *handoff_ptable;
    
    g_kmt_ctx.tcb_pool[0] = (ktcb_t){
        // the boot thread stack is already set up by the bootloader
        // the boot thread cannot be switched into, so we don't need to worry about its stack or registers
        .esp = nullptr,
        .cr3 = x86_get_ctx_map(handoff_ptable),
        .status = KTHREAD_STATUS_RUNNING,
    };
    g_kmt_ctx.current_thread = 0;
}

