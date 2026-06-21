#pragma once

#include <includes.h>
#include <arch/i686.h>

typedef u64 time_ns_t;
typedef u64 time_us_t;
typedef u64 time_ms_t;

time_ns_t timer_time_since_init_ns();
