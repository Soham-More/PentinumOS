#include "Stack.hpp"
#include <std/memory.h>
#include <std/logger.h>

bool traceStack = false;
uint32_t* stack;
uint32_t stackPosition;

void _no_stack_trace __cyg_profile_func_enter (void *this_fn, void *call_site)
{
    if(!traceStack) return;
    stackPosition++;

    stack[stackPosition] = reinterpret_cast<uint32_t>(this_fn);
}
void _no_stack_trace __cyg_profile_func_exit  (void *this_fn, void *call_site)
{
    if(!traceStack) return;
    stackPosition--;
}

namespace sys
{
    void _no_stack_trace beginStackTrace(void* _main)
    {
        traceStack = true;

        stack = reinterpret_cast<uint32_t*>(alloc_pages_aligned(MAX_STACK_DEPTH * sizeof(uint32_t), MAX_STACK_DEPTH  * sizeof(uint32_t)));
    
        // first stack value is entry point
        stack[0] = reinterpret_cast<uint32_t>(_main);
    }

    void _no_stack_trace printStackTrace()
    {
        traceStack = false;

        log_info("\nStack Trace: \n");

        for(uint32_t i = stackPosition; i > 0; i--)
        {
            log_info("\t%u: 0x%x\n", i, stack[i]);
        }

        log_info("\t%u: 0x%x[main]\n", 0, stack[0]);

        traceStack = true;
    }
}
