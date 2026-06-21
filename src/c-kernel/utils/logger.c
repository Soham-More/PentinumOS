#include "logger.h"

#include <stdarg.h>
#include <resources/tty.h>

static tty_t* g_logging_tty = nullptr;

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
        thread_uid_t current_thread = kmt_get_current_thread();
        thread_name = kmt_get_thread_name(current_thread);
    }

    {
        TTY_AQUIRE_SCOPED_LOCK(g_logging_tty);
        tty_printf(CON_COLOR_BROWN, g_logging_tty, "({s}) ", thread_name);
        tty_vprintf(color, g_logging_tty, fmt, vlist);
    }
    

    va_end(vlist);
}

void logging_set_tty(tty_t* tty) {
    // this might seem dumb, but
    // we need to make sure that the tty is not being used by another thread
    // so we will acquire the lock for the previous tty, and then set the new tty
    // why? because sys_logf will acquire the lock for the previous tty
    // so if we don't acquire the lock for the previous tty, 
    // then sys_logf might end up using the new tty while it hasn't acquired the lock for it yet!
    // the scoped lock macro will release the lock for the previous tty when it goes out of scope(it stores a copy)
    // only do this if the logging tty is already set, otherwise we will just set it to the new tty
    if(g_logging_tty) {
        TTY_AQUIRE_SCOPED_LOCK(g_logging_tty);
        g_logging_tty = tty;
    } else {
        g_logging_tty = tty;
    }
}
