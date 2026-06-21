#pragma once

#include <threads/includes.h>
#include <utils/heap.h>

// main thread entry point for the syscore thread
void syscore_thread_entry();

// api functions for syscore thread

// sanity check function for syscore thread
err_t syscore_echo_test(const char* test);
