// defines special boot time initialization functions
#pragma once

#include <includes.h>
#include <utils/heap.h>
#include <resources/tty.h>
#include <resources/console.h>
#include <arch/paging/paging.h>

#include "handoff/handoff.h"

heap_allocator_t* m_aquire_global_allocator();
void m_release_global_allocator(volatile heap_allocator_t** allocator);
#define GET_GLOBAL_ALLOCATOR(galloc) volatile heap_allocator_t* galloc __attribute__((cleanup(m_release_global_allocator))) = m_aquire_global_allocator();

void initialize_timer();
tty_t* initialize_tty(const char* name, console_t* earlyconsole);
void initialize_console_manager(console_t earlyconsole);
err_t initialize_buddy_allocator(heap_allocator_t* heap_allocator, KernelInfo* kInfo);
void initialize_multitasking(x86_mmu_map_t* handoff_ptable, heap_allocator_t* kalloca);

void timer_setup_callback(u32 frequency_hz, x86_interrupt_handler_t callback);
void kmt_spawn_idle_thread(thread_entry_point_t entry, x86_mmu_map_t ptable);
