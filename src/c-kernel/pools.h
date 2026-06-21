#pragma once

#include <includes.h>

// important pages
_import u8 __stack_rsvd_start[];
_import u8 __stack_rsvd_end[];

_import u8 __heap_rsvd_start[];
_import u8 __heap_rsvd_end[];

// heaps
_import u8 __root_heap_start[];
_import u8 __root_heap_end[];

_import u8 __idle_heap_start[];
_import u8 __idle_heap_end[];

_import u8 __buddy_heap_start[];
_import u8 __buddy_heap_end[];

_import u8 __kmt_heap_start[];
_import u8 __kmt_heap_end[];

_import u8 __syscore_heap_start[];
_import u8 __syscore_heap_end[];

// thread stacks
_import u8 __idle_thread_intr_stack_start[];
_import u8 __idle_thread_intr_stack_end[];

_import u8 __idle_thread_exec_stack_start[];
_import u8 __idle_thread_exec_stack_end[];

_import u8 __syscore_thread_intr_stack_start[];
_import u8 __syscore_thread_intr_stack_end[];

_import u8 __syscore_thread_exec_stack_start[];
_import u8 __syscore_thread_exec_stack_end[];

//_import char __page_allocator_start[];
//_import char __page_allocator_end[];

