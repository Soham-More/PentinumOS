#include "i686.h"

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
}


