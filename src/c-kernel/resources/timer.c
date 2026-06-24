#include "timer.h"
#include <boot/init.h>

#include <arch/IRQ/IRQ.h>
#include <arch/IRQ/PIC.h>

#define PIT_FREQUENCY_HZ 1193182

struct {
    u64 ticks_since_init;
    u32 frequency_hz;
    u32 callback_acuumulator;
    u32 callback_frequency_hz;
    x86_interrupt_handler_t callback;
} g_timer_ctx;

void _timer_interrupt(registers_t* registers) {
	g_timer_ctx.ticks_since_init++;

    if(!g_timer_ctx.callback) return;

    g_timer_ctx.callback_acuumulator += g_timer_ctx.callback_frequency_hz;
    if(g_timer_ctx.callback_acuumulator < g_timer_ctx.frequency_hz) return;

    g_timer_ctx.callback_acuumulator -= g_timer_ctx.frequency_hz;
    g_timer_ctx.callback(registers);
}

void initialize_timer() {
    x86_disable_interrupts();
	
	u16 divider = (PIT_FREQUENCY_HZ / 16500) & ~(0x1);
	
	x86_outb(0x43, 0b00110110);
    x86_outb(0x40, (u8)divider);
    x86_outb(0x40, (u8)(divider >> 8));
    g_timer_ctx.frequency_hz = PIT_FREQUENCY_HZ;
    g_timer_ctx.callback = nullptr;
    g_timer_ctx.callback_frequency_hz = 0;

	PIC_irq_unmask(0);
    IRQ_registerHandler(0, _timer_interrupt);

    x86_enable_interrupts();
}

time_ns_t timer_time_since_init_ns() {
    u64 ticks = g_timer_ctx.ticks_since_init;
    u64 ticks_quotient = ticks / g_timer_ctx.frequency_hz;
    u64 ticks_remainder = ticks % g_timer_ctx.frequency_hz;

    // avoids overflow by calculating the remainder first, then dividing by the frequency
    return (ticks_quotient * (1000 * 1000 * 1000)) + ((ticks_remainder * (1000 * 1000 * 1000)) / g_timer_ctx.frequency_hz);
}
void timer_setup_callback(u32 frequency_hz, x86_interrupt_handler_t callback) {
    g_timer_ctx.callback_frequency_hz = frequency_hz;
    g_timer_ctx.callback = callback;
}
