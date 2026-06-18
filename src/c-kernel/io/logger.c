#include "logger.h"
#include "console.h"

#include <hw/cpu.h>
#include <multitasking/kernel.h>

#include <stdarg.h>
#include <c-utils/stdio.h>

thread_mutex_t g_logging_mutex;

void sys_logf(u8 severity, const char* fmt, ...) {
    va_list vlist;
    va_start(vlist, fmt);

    u8 color;

    switch (severity)
    {
    case Severity_Debug:
        color = CON_COLOR_MAGENTA;
        break;
    case Severity_Info:
        color = CON_COLOR_CYAN;
        break;
    case Severity_Warn:
        color = CON_COLOR_BLUE;
        break;
    case Severity_Error:
        color = CON_COLOR_RED;
        break;
    case Severity_Critical:
        color = CON_COLOR_RED;
        break;
    default:
        color = CON_COLOR_WHITE;
        break;
    }
    
    const char* thread_name = "kboot";
    if(kmt_is_initialized()) {
        // lock the mutex to ensure thread-safe logging
        panic_on_err(kmt_lock_mutex(g_logging_mutex), "Failed to lock logging mutex");

        thread_uid_t current_thread = kmt_get_current_thread();
        thread_name = kmt_get_thread_name(current_thread);
    }

    con_get_default()->text_color = CON_COLOR_BROWN;
    printf("({s}) ", thread_name);
    flush_terminal();
    
    con_get_default()->text_color = color;
    
    vprintf(fmt, vlist);
    flush_terminal();
    
    con_get_default()->text_color = CON_COLOR_WHITE;
    
    if(kmt_is_initialized()) {
        // unlock the mutex after logging
        panic_on_err(kmt_unlock_mutex(g_logging_mutex), "Failed to unlock logging mutex");
    }

    va_end(vlist);
}

// setup mutexes and other things for stdio to be thread safe
void logging_initialize_post_multitasking() {
    g_logging_mutex = kmt_create_mutex();
}
