#include "panic.h"
#include <boot/init.h>

#include <arch/i686.h>
#include "tty.h"

void kernel_panic(const char* filename, const char* function, usize lineno, err_t error, const char* fmt, ...) {
    // disable interrupts to prevent further damage
    x86_disable_interrupts();
    va_list vlist;
    va_start(vlist, fmt);

    tty_panic_printf("KERNEL PANIC at {s}:{u} in function {s}(), ERROR=0x{x}\n", filename, lineno, function, error);
    tty_panic_printf("PANIC MESSAGE: ");
    tty_panic_vprintf(fmt, vlist);
    tty_panic_printf("\n");

    va_end(vlist);
    for(;;);
}

