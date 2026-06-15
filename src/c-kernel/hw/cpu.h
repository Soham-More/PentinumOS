#pragma once

#include <includes.h>

typedef struct cpu_info_t {
    char vendor[13]; // 12 chars + null terminator
    u8 family;
    u8 model;
} cpu_info_t;

cpu_info_t cpu_get_info();
void panic(u32 error_code);
