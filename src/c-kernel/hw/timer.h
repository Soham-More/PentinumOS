#pragma once

#include <includes.h>
#include <arch/i686.h>

typedef u64 time_ns_t;
typedef u64 time_us_t;
typedef u64 time_ms_t;

void initialize_timer();

time_ns_t timer_time_since_init_ns();
void timer_setup_callback(u32 frequency_hz, x86_interrupt_handler_t callback);
