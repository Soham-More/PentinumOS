#include "logger.h"
#include "console.h"

#include <stdarg.h>
#include <resources/tty.h>

volatile static tty_t* g_logging_tty = nullptr;

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
    g_logging_tty = tty;
}
