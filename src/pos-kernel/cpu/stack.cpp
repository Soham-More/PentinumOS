#include "stack.hpp"
#include <io/io.h>

bool traceStack = false;
u32* stack;
u32 stack_size;
u32 max_stack_size;

_import char __stack_start[];
_import char __stack_end[];
_import char __text_start[];

void _no_stack_trace __cyg_profile_func_enter (void *this_fn, void *call_site)
{
    if(!traceStack) return;
    
    stack[stack_size] = reinterpret_cast<u32>(this_fn);
    stack_size++;
}
void _no_stack_trace __cyg_profile_func_exit  (void *this_fn, void *call_site)
{
    if(!traceStack) return;
    stack_size--;
}

namespace cpu
{
    void _no_stack_trace beginStackTrace(void* _main)
    {
        traceStack = true;

        stack = (u32*)__stack_start;
        stack_size = 0;
        max_stack_size = (reinterpret_cast<u32>(__stack_end) - reinterpret_cast<u32>(__stack_end)) / 4;
    
        // first stack value is entry point
        stack[0] = reinterpret_cast<u32>(_main);
        stack_size++;
    }

    void _no_stack_trace printStackTrace()
    {
        if(!traceStack) return;

        log_info("Stack Trace[%d]: \n", stack_size);

        for(u32 i = stack_size - 1; i > 0; i--)
        {
            u32 offset = stack[i] - reinterpret_cast<u32>(__text_start);
            log_info("\t%u: 0x%x(.text + 0x%x)\n", i, stack[i], offset);
        }

        log_info("\t%u: 0x%x[main]\n", 0, stack[0]);
    }
}
