#include "idle.h"

#include <utils/heap.h>

#include <pools.h>
#include <utils/logger.h>
#include <panic/panic.h>

#include <syscore/threads.h>
#include <syscore/syscore.h>

idle_thread_init_t g_idle_thread_init;

void idle_thread_entry() {
    log_info("idle thread started successfully\n");
    log_info("handoff to syscore thread\n");
    // create the syscore thread, which will be used to manage the memory
    kpanic_on_err(syscore_start_thread(g_idle_thread_init.page_table), "Failed to start syscore thread");

    /*
    for(usize i = 0; i < 4; i++) {
        // test the syscore thread by sending an rpc to it
        char test_data[16] = "Hello x !";
        test_data[7] = '0' + i; // Replace 'x' with the iteration number
        
        panic_on_err(syscore_echo_test(test_data), "Failed to test syscore thread");
    }
    */

    for(;;);
}

