#pragma once

#include <includes.h>

_import __attribute__((section("global_heap"))) u8 __global_heap_start[];
_import __attribute__((section("global_heap"))) u8 __global_heap_end[];

_import __attribute__((section("buddy_heap"))) u8 __buddy_heap_start[];
_import __attribute__((section("buddy_heap"))) u8 __buddy_heap_end[];

//_import char __page_allocator_start[];
//_import char __page_allocator_end[];

