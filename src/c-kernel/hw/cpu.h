#pragma once

#include <includes.h>

typedef struct cpuinfo_t {
    char vendor[13]; // 12 chars + null terminator
    u8 family;
    u8 model;
} cpuinfo_t;

cpuinfo_t get_cpu_info();

void panic(u32 error_code);


// panic error codes
#define PANIC_OK ((u32)0x00)

#define PANIC_CLASS_GENERIC      ((u32)0x00)
#define PANIC_UNEXPECTED_FAILURE (PANIC_CLASS_GENERIC | 0x01)

#define PANIC_CLASS_MEMORY       ((u32)0x10)
#define PANIC_BAD_MEMORY_REQUEST (PANIC_CLASS_MEMORY | 0x01)
