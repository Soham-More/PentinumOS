#include "cpu.h"

#include <c-utils/stdio.h>

cpu_info_t cpu_get_info() {
    return (cpu_info_t) {
        .vendor = "GenuineIntel",
        .family = 6,
        .model = 14,
    };
}

void panic(u32 error_code) {
    // TODO: implement proper panic handler
    flush_terminal();
    for(;;);
}

