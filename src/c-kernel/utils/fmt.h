#pragma once

#include <includes.h>
#include <stdarg.h>

#include "heap.h"

// format to buffer, returns 0 on failure
usize format(char* buffer, usize buffer_size, const char* fmt, ...);
// format to heap allocated buffer, returns err ptr on failure
char* formatA(heap_allocator_t* heap, const char* fmt, ...);

