#pragma once

#include <stdint.h>
#include <arch/GDT/GDT.h>
#include <arch/IDT/IDT.h>
#include <arch/ISR/ISR.h>
#include <arch/IRQ/IRQ.h>
#include <arch/paging/paging.h>
#include <arch/x86.h>

typedef u8 (*page_ptr_t)[X86_PAGE_SIZE];

typedef void (*x86_interrupt_handler_t)(registers_t* registers);

void initialize_i686();

tss_entry_t* get_global_tss();
u16 get_global_tss_selector();
