#include "i686.h"

#include <pools.h>

tss_entry_t g_tss;

void initialize_i686()
{
	//printf("loading GDT...  ");
	i686_init_gdt();
	//printf("ok\n");

	//printf("loading IDT...  ");
	i686_load_idt_current();
	//printf("ok\n");

	//printf("loading default isrs...  ");
	i686_init_isr();
	//printf("ok\n");

	//printf("loading IRQs...  ");
	init_irq();
	//printf("ok\n");

	g_tss = (tss_entry_t){0};
	g_tss.ss0 = i686_GDT_DATA_SEGMENT;
	// set the kernel interrupt stack pointer to the end of the interrupt stack (the stack grows downwards)
	g_tss.esp0 = (u32)(__idle_thread_intr_stack_end - 4);
	// no IO permission bitmap in user land
	g_tss.iomap_base = sizeof(tss_entry_t);
	x86_gdt_add_tss(&g_tss, 3);
}

tss_entry_t* get_global_tss() { return &g_tss; }
u16 get_global_tss_selector() { return 3 << 3; }

