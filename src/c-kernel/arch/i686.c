#include "i686.h"

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
	// no IO permission bitmap in user land
	g_tss.iomap_base = sizeof(tss_entry_t);
	x86_gdt_add_tss(&g_tss, 3);
}

tss_entry_t* get_global_tss() { return &g_tss; }
u16 get_global_tss_selector() { return 3 << 3; }

