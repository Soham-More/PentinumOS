#include "thread_panic.h"
#include <resources/tty.h>

tty_t* panic_tty = nullptr;
void init_thread_panic(tty_t* tty) {
    panic_tty = tty;
}

void thread_panic(const char* filename, const char* function, usize lineno, err_t error, const char* fmt, ...) {
    va_list vlist;
    va_start(vlist, fmt);

    const char* thread_name = kmt_get_thread_name(kmt_get_current_thread());

    tty_printf(CON_COLOR_BROWN, panic_tty, "({s}) ", thread_name);
    tty_printf(CON_COLOR_RED, panic_tty, "THREAD PANICKED at {s}:{u} in function {s}(), ERROR=0x{x}\n", filename, lineno, function, error);
    tty_printf(CON_COLOR_RED, panic_tty, "PANIC MESSAGE: ");
    tty_vprintf(CON_COLOR_RED, panic_tty, fmt, vlist);
    tty_printf(CON_COLOR_RED, panic_tty, "\n");
    tty_printf(CON_COLOR_WHITE, panic_tty, "killing thread {s}...\n", thread_name);

    va_end(vlist);

    // kill the damn thread
    kmt_kill_current_thread();
    for(;;);
}

