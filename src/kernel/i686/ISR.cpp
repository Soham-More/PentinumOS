#include "ISR.h"

#include <std/stdio.h>
#include <std/logger.h>
#include <System/Stack.hpp>
#include "isr_gen.h"
#include "IDT.h"

#define _PRINT_REGISTER(x) log_info("\t%3s: 0x%8x", #x, registers->x)

static ISRHandler isrHandlers[256];

static const char* const _ISR_Exceptions[] = {
    "Divide by zero error",
    "Debug",
    "Non-maskable Interrupt",
    "Breakpoint",
    "Overflow",
    "Bound Range Exceeded",
    "Invalid Opcode",
    "Device Not Available",
    "Double Fault",
    "Coprocessor Segment Overrun",
    "Invalid TSS",
    "Segment Not Present",
    "Stack-Segment Fault",
    "General Protection Fault",
    "Page Fault",
    "",
    "x87 Floating-Point Exception",
    "Alignment Check",
    "Machine Check",
    "SIMD Floating-Point Exception",
    "Virtualization Exception",
    "Control Protection Exception ",
    "",
    "",
    "",
    "",
    "",
    "",
    "Hypervisor Injection Exception",
    "VMM Communication Exception",
    "Security Exception",
    ""
};

void _no_stack_trace i686_dump_registers(Registers* registers)
{
    log_info("Registers: \n");
    _PRINT_REGISTER(eax);
    _PRINT_REGISTER(ebx);
    printf("\n");
    _PRINT_REGISTER(ecx);
    _PRINT_REGISTER(edx);
    printf("\n");
    _PRINT_REGISTER(edi);
    _PRINT_REGISTER(esi);
    printf("\n");
    _PRINT_REGISTER(ebp);
    _PRINT_REGISTER(ebx);
    printf("\n");
    _PRINT_REGISTER(eip);
    _PRINT_REGISTER(cs);
    printf("\n");
    _PRINT_REGISTER(ss);
    _PRINT_REGISTER(esp);
    printf("\n");
    _PRINT_REGISTER(eflags);
}

_export void _asmcall _no_stack_trace _default_isr_handler(Registers* registers)
{
    if(isrHandlers[registers->interrupt] != nullptr)
    {
        isrHandlers[registers->interrupt](registers);
    }
    else if(registers->interrupt < 32)
    {
        log_critical("\nKERNEL PANIC: Unhandled Interrupt %u: %s\n\t CPU Error Code: %u\n", registers->interrupt, _ISR_Exceptions[registers->interrupt], registers->error);

        i686_dump_registers(registers);

        sys::printStackTrace();

        __asm__("hlt");
    }
    else
    {
        log_critical("\nKERNEL PANIC: Unhandled Interrupt %u\n\t CPU Error Code: %u\n", registers->interrupt, registers->error);

        i686_dump_registers(registers);

        __asm__("hlt");
    }
}

void i686_init_isr()
{
    _i686_init_default_handlers();
    
    for(uint32_t i = 0; i < 256; i++)
    {
        i686_enable_interrupt(i);
    }

    i686_disable_interrupt(0x80);
}

void i686_set_isr(uint8_t isr_vector, ISRHandler isr)
{
    isrHandlers[isr_vector] = isr;
    i686_enable_interrupt(isr_vector);
}
