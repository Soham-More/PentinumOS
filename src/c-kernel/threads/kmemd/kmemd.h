#pragma once

#include <threads/includes.h>
#include <memory/memory.h>

// main thread entry point for the kmemd thread
void kmemd_thread_entry();

// api functions for kmemd thread

// sanity check function for kmemd thread
err_t kmemd_echo_test(const char* test);
page_alloc_info_t kmemd_alloc_pages(usize page_count);
