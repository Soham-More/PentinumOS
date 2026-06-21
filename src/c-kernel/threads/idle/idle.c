#include "idle.h"

#include <utils/heap.h>

#include <pools.h>
#include <utils/logger.h>
#include <hw/cpu.h>

#include <multitasking/kernel.h>
#include <threads/syscore/syscore.h>


idle_thread_init_t g_idle_thread_init;

void idle_thread_entry() {
    log_info("idle thread started successfully\n");
    
    // create the syscore thread, which will be used to manage the memory
    thread_uid_t syscore_uid = kmt_create_thread(&(thread_desc_t){
        .name = "syscore",
        .entry = syscore_thread_entry,
        .ptable = g_idle_thread_init.page_table,
        .stack_top = __syscore_thread_exec_stack_end - 4,
        .interrupt_stack_top = __syscore_thread_intr_stack_end - 4,
        .heap_base = __syscore_heap_start,
        .heap_size = PTR_DIFF_I32(__syscore_heap_start, __syscore_heap_end),
        .priority = 1,
        .policy = KMT_POLICY_ROUND_ROBIN,
    });
    panic_if(KMT_IS_INVALID_UID(syscore_uid), KMT_GET_ERR_UID(syscore_uid), "Failed to create syscore thread");

    panic_on_err(kmt_wakeup_thread(syscore_uid), "Failed to wakeup syscore thread");

    for(usize i = 0; i < 4; i++) {
        // test the syscore thread by sending an rpc to it
        char test_data[16] = "Hello x !";
        test_data[8] = '0' + i; // Replace 'x' with the iteration number
        
        panic_on_err(syscore_echo_test(test_data), "Failed to test syscore thread");
    }

    page_alloc_info_t allocated_pages = syscore_alloc_pages(4);
    panic_if(allocated_pages.error != ESUCCESS, allocated_pages.error, "Failed to allocate pages from syscore thread");

    log_info("syscore thread allocated 4 pages at address {p}\n", allocated_pages.memory);

    for(;;);
}

