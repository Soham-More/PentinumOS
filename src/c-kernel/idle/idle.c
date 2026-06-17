#include "idle.h"

#include <pools.h>
#include <io/logger.h>

idle_thread_init_t g_idle_thread_init;

void idle_thread_entry() {
    log_info("idle thread started successfully\n");
    
    heap_allocator_t* kalloca = g_idle_thread_init.allocator;

    for(;;);
}

