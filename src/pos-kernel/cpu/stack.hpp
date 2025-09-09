#pragma once

#include <includes.h>

_export void _no_stack_trace __cyg_profile_func_enter (void *this_fn, void *call_site);
_export void _no_stack_trace __cyg_profile_func_exit  (void *this_fn, void *call_site);

namespace cpu
{
    // MUST be called from main, pass main pointer
    void _no_stack_trace beginStackTrace(void* _main);

    // Prints stack trace
    void _no_stack_trace printStackTrace();
}
