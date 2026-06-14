// define primitives for drivers
#pragma once

#include <includes.h>
#include <memory/memory.h>

typedef struct driver_info_t {
    char name[16];
    err_t (*init)();
} driver_info_t;

void initialize_driver_manager(heap_allocator_t* alloca);
heap_allocator_t* get_driver_allocator();

