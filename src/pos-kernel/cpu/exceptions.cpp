#include "exceptions.hpp"

#include <arch/i686.h>
#include "stack.hpp"

#include <std/errors.hpp>

static const char* const _system_exception_desc[] = {
    "Out of Heap Memory",
    "USB Exception"
};

_no_stack_trace void exception_handler(Registers* registers)
{
    if((registers->eax > STD_EXCEPTION_BASE) && (registers->eax < STD_EXCEPTION_BASE + STD_EXCEPTION_SIZE))
    {
        log_critical("[Exception] Error Code: 0x%x\n", registers->eax);
        log_critical("\t%s\n", std::errors_desc[registers->eax - STD_EXCEPTION_BASE]);
    }
    else if(registers->eax >= STD_EXCEPTION_BASE + STD_EXCEPTION_SIZE)
    {
        registers->eax -= STD_EXCEPTION_BASE + STD_EXCEPTION_SIZE;

        if(registers->eax < sizeof(_system_exception_desc) / sizeof(char*))
        {
            log_critical("[Exception] Error Code: 0x%x, Info=\"%s\"\n", registers->eax, _system_exception_desc[registers->eax]);
        }

    }
    else
    {
        log_critical("[Exception] Error Code: 0x%x, Info=\"Unknown\"\n", registers->eax);
    }

	// print the stack trace
	cpu::printStackTrace();

	// halt
	cpu::halt();
}

namespace cpu
{
    void initialize_exceptions()
    {
        i686_set_isr(0x81, exception_handler);
    }
}

