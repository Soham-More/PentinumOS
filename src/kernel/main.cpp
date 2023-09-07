#include <stdint.h>
#include <std/memory.h>
#include <std/stdio.h>
#include <i686/GDT.h>
#include <i686/IDT.h>
#include <i686/ISR.h>
#include <i686/IRQ.h>
#include <i686/PIC.h>
#include <Drivers/Drivers.h>
#include <Drivers/PIT.h>
#include <std/memory.h>
#include <Bus/PCI/PCI.hpp>
#include <System/Paging.hpp>
#include <System/Boot.h>
#include <std/logger.h>
#include <std/vector.hpp>

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

	sys::initialisePaging(*kernelInfo.pagingInfo);

	// initalise memory manager
	mem_init(kernelInfo);

	PS2::flush_keyboard();

	PCI::enumeratePCIBus();

	std::vector<uint32_t> v = {10, 20, 30};

	printf("\n{");

	for(uint32_t i = 0; i < v.size(); i++)
	{
		printf("%d, ", v[i]);
	}

	printf("}\n");

	for (;;);
}
