#pragma once

#include <io/io.h>
#include <stdint.h>
#include <arch/GDT/GDT.h>
#include <arch/IDT/IDT.h>
#include <arch/ISR/ISR.h>
#include <arch/IRQ/IRQ.h>
#include <arch/IRQ/PIC.h>
#include <arch/x86.h>

inline void initialize_i686()
{
	//printf("loading GDT...  ");
	i686_init_gdt();
	//printf("ok\n");

	//printf("loading IDT...  ");
	i686_load_idt_current();\
	//printf("ok\n");

	//printf("loading default isrs...  ");
	i686_init_isr();
	//printf("ok\n");

	//printf("loading IRQs...  ");
	init_irq();
	//printf("ok\n");
}
