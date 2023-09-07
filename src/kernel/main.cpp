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

extern "C" void __cxa_pure_virtual(){;}

extern "C" int __cxa_atexit(void (*destructor) (void *), void *arg, void *dso)
{
    arg;
    dso;
    return 0;
}
extern "C" void __cxa_finalize(void *f)
{
    f;
}

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

	PCI::prettyPrintDevices();

	for (;;);
}
