#pragma once

#include <threads/includes.h>
#include <arch/paging/paging.h>

// api functions for syscore thread
err_t syscore_start_thread(x86_mmu_map_t page_table);
// sanity check function for syscore thread
err_t syscore_echo_test(const char* test);
// allocate contiguous pages and map them into the thread's address space
err_t syscore_alloc_pages(usize num_pages, ptr_t vaddress);
// allocate MMIO pages
err_t syscore_alloc_mmio_pages(usize num_pages, ptr_t vaddress, u32 page_flags);
// start a new thread
//err_t syscore_spawn_thread(char* name, thread_entry_point_t entry_point, u8 priority);