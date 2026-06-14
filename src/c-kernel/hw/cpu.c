#include "cpu.h"

#include <c-utils/stdio.h>

cpuinfo_t get_cpu_info() {
    return (cpuinfo_t) {
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

