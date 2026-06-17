#pragma once

#include <includes.h>

#include <arch/i686.h>
#include <io/console.h>
#include <memory/memory.h>
#include <arch/paging/paging.h>

typedef struct idle_thread_init_t {
    console_t earlyconsole;
    x86_mmu_map_t page_table;
    heap_allocator_t* allocator;
} idle_thread_init_t;

extern idle_thread_init_t g_idle_thread_init;

void idle_thread_entry();

