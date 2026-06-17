#include "cpu.h"

#include <arch/i686.h>
#include <c-utils/stdio.h>
#include <io/console.h>

cpu_info_t cpu_get_info() {
    return (cpu_info_t) {
        .vendor = "GenuineIntel",
        .family = 6,
        .model = 14,
    };
}

void cpu_panic(const char* filename, const char* function, usize lineno, err_t error, const char* fmt, ...) {
    // disable interrupts to prevent further damage
    x86_disable_interrupts();
    va_list vlist;
    va_start(vlist, fmt);

    con_get_default()->text_color = CON_COLOR_RED;

    printf("KERNEL PANIC at {s}:{u} in function {s}(), ERROR=0x{x}\n", filename, lineno, function, error);
    printf("PANIC MESSAGE: ");
    vprintf(fmt, vlist);
    printf("\n");
    flush_terminal();

    va_end(vlist);
    for(;;);
}

