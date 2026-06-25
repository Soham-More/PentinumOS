#pragma once

#include <includes.h>
#include <arch/i686.h>

typedef u64 time_ns_t;
typedef u64 time_us_t;
typedef u32 time_ms_t;

#define TIME_MS_TO_NS(ms) ((time_ns_t)(ms) * 1000 * 1000)
#define TIME_US_TO_NS(us) ((time_ns_t)(us) * 1000)

time_ns_t timer_time_since_init_ns();
