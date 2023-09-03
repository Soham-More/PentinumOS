#include <stdint.h>
#include <std/memory.h>
#include <std/stdio.h>
#include <Drivers/VGA/VGA.hpp>
#include <i686/GDT.h>
#include <i686/IDT.h>
#include <i686/ISR.h>
#include <i686/IRQ.h>
#include <i686/PIC.h>
#include <Drivers/PIT.h>
#include <std/memory.h>
#include <Drivers/Keyboard/PS2.hpp>
#include <Drivers/Keyboard/PS2Keyboard.hpp>

typedef struct 
{
	uint8_t bootDrive;
	void* e820_mmap;
	void* kernelMap;
} _packed KernelInfo;

_import void _init();

void kernel_init()
{
	_init();

	clrscr();

	printf("loading GDT...  ");
	i686_init_gdt();
	printf("ok\n");

	printf("loading IDT...  ");
	i686_load_idt_current();\
	printf("ok\n");

	printf("loading default isrs...  ");
	i686_init_isr();
	printf("ok\n");

	printf("loading IRQs...  ");
	init_irq();
	PIT_init();
	printf("ok\n");

	printf("initialising PS2...  ");
	PS2_init();
	printf("ok\n");

	printf("initialising Keyboard...  ");
	PS2::init_keyboard();
	printf("ok\n");
}

// export "C" to prevent g++ from
// mangling this function's name
// and confusing the linker	
_export void start(KernelInfo kernelInfo)
{
	kernel_init();

	// initalise memory manager
	// after this both memnory maps become useless
	mem_init(kernelInfo.e820_mmap, kernelInfo.kernelMap);

	PS2::flush_keyboard();

	printf("Console Mode\n>");

	char c = 0;

	while(c != '\n')
	{
		c = getchar();
	}

	for (;;);
}
