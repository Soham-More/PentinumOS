#include "ISR.h"

#include <utils/logger.h>
#include "isr_gen.h"
#include "../IDT/IDT.h"
#include <panic/tty.h>

#define _PRINT_REGISTER(x) tty_panic_printf("\t{6s%>}: {12h}", #x, registers->x)

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

void _no_stack_trace i686_dump_registers(registers_t* registers)
{

    tty_panic_printf("Registers: \n");
    _PRINT_REGISTER(eax);
    _PRINT_REGISTER(ebx);
    tty_panic_printf("\n");
    _PRINT_REGISTER(ecx);
    _PRINT_REGISTER(edx);
    tty_panic_printf("\n");
    _PRINT_REGISTER(edi);
    _PRINT_REGISTER(esi);
    tty_panic_printf("\n");
    _PRINT_REGISTER(ebp);
    _PRINT_REGISTER(ebx);
    tty_panic_printf("\n");
    _PRINT_REGISTER(eip);
    _PRINT_REGISTER(cs);
    tty_panic_printf("\n");
    _PRINT_REGISTER(ss);
    _PRINT_REGISTER(esp);
    tty_panic_printf("\n");
    _PRINT_REGISTER(eflags);
}

_export void _asmcall _no_stack_trace _default_isr_handler(registers_t* registers)
{
    if(isrHandlers[registers->interrupt] != nullptr)
    {
        isrHandlers[registers->interrupt](registers);
    }
    else if(registers->interrupt < 32)
    {
        x86_disable_interrupts();

        tty_panic_printf("\nKERNEL PANIC: Unhandled Interrupt {u}: {s}\n\t CPU Error Code: {u}\n", registers->interrupt, _ISR_Exceptions[registers->interrupt], registers->error);

        i686_dump_registers(registers);

        tty_panic_printf("\n");

        //cpu::printStackTrace();

        for(;;) __asm__("hlt");
    }
    else
    {
        x86_disable_interrupts();

        tty_panic_printf("\nKERNEL PANIC: Unhandled Interrupt {u}\n\t CPU Error Code: {u}\n", registers->interrupt, registers->error);

        i686_dump_registers(registers);

        tty_panic_printf("\n");

        //cpu::printStackTrace();

        for(;;) __asm__("hlt");
    }
}

void i686_init_isr()
{
    _i686_init_default_handlers();
    
    for(u32 i = 0; i < 256; i++)
    {
        i686_enable_interrupt(i);
    }

    i686_disable_interrupt(0x80);
}

void i686_set_isr(u8 isr_vector, ISRHandler isr)
{
    isrHandlers[isr_vector] = isr;
    i686_enable_interrupt(isr_vector);
}
