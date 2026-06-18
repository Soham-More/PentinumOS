#include "idle.h"

#include <memory/memory.h>

#include <pools.h>
#include <io/logger.h>
#include <hw/cpu.h>

#include <multitasking/kernel.h>
#include <threads/kmemd/kmemd.h>


idle_thread_init_t g_idle_thread_init;

void idle_thread_entry() {
    log_info("idle thread started successfully\n");
    
    // create the kmemd thread, which will be used to manage the memory
    thread_uid_t kmemd_uid = kmt_create_thread(&(thread_desc_t){
        .name = "kmemd",
        .entry = kmemd_thread_entry,
        .ptable = g_idle_thread_init.page_table,
        .stack_top = __kmemd_thread_exec_stack_end - 4,
        .interrupt_stack_top = __kmemd_thread_intr_stack_end - 4,
        .heap_base = __kmemd_heap_start,
        .heap_size = PTR_DIFF_I32(__kmemd_heap_start, __kmemd_heap_end),
        .priority = 1,
        .policy = KMT_POLICY_ROUND_ROBIN,
    });
    panic_if(KMT_IS_INVALID_UID(kmemd_uid), KMT_GET_ERR_UID(kmemd_uid), "Failed to create kmemd thread");

    panic_on_err(kmt_wakeup_thread(kmemd_uid), "Failed to wakeup kmemd thread");

    for(usize i = 0; i < 4; i++) {
        // test the kmemd thread by sending an rpc to it
        char test_data[16] = "Hello x !";
        test_data[8] = '0' + i; // Replace 'x' with the iteration number
        
        panic_on_err(kmemd_echo_test(test_data), "Failed to test kmemd thread");
    }

    page_alloc_info_t allocated_pages = kmemd_alloc_pages(4);
    panic_if(allocated_pages.error != ESUCCESS, allocated_pages.error, "Failed to allocate pages from kmemd thread");

    log_info("kmemd thread allocated 4 pages at address {p}\n", allocated_pages.memory);

    for(;;);
}

