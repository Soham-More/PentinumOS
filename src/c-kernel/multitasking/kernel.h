#pragma once

#include <includes.h>
#include <arch/paging/paging.h>
#include <memory/memory.h>

#define MAX_KERNEL_THREADS 256
typedef u8 kthread_uid_t;

void initialize_multitasking(x86_mmu_map_t* handoff_ptable);
