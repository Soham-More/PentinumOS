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
#include <Bus/USB/UHCI/UHCI.hpp>
#include <System/Stack.hpp>
#include <std/Heap/heap.hpp>

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

	uint16_t divider = (PIT_MAX_FREQ / 16500) & ~(0x1);

	x86_outb(0x43, 0b00110110);
    x86_outb(0x40, (uint8_t)divider);
    x86_outb(0x40, (uint8_t)(divider >> 8));

	printf("initialising PS2...  ");
	PS2_init();
	printf("ok\n");

	printf("initialising Keyboard...  ");
	PS2::init_keyboard();
	printf("ok\n");
}

// https://github.com/Remco123/CactusOS/blob/master/kernel/src/system/components/pci.cpp

// export "C" to prevent g++ from
// mangling this function's name
// and confusing the linker
_export void start(KernelInfo kernelInfo)
{
	kernel_init();

	sys::beginStackTrace(reinterpret_cast<void*>(start));

	sys::initialisePaging(*kernelInfo.pagingInfo);

	// initalise memory manager
	mem_init(kernelInfo);

	PS2::flush_keyboard();

	PCI::enumeratePCIBus();

	PCI::prettyPrintDevices();

	std::initHeap();

	// TODO: generate an exception when nullptr is accessed.

	//void* ptrA = std::mallocAligned(0x1000, 12);
	//void* ptrB = std::mallocAligned(0x20, 5);
	//std::free(ptrA);
	//void* ptrE = std::mallocAligned(0x20, 5);
	//std::free(ptrB);
	//void* ptrC = std::mallocAligned(0x20, 5);
	//void* ptrD = std::mallocAligned(0x20, 5);

	//prettyPrintMemory();

	// get USB UHCI controller
	USB::UHCIController controller(PCI::getPCIDevice(0x0C, 0x03));

	controller.Init();
	controller.Setup();

	const std::vector<USB::uhci_device>& devices = controller.getAllDevices();

	log_info("All Connected USB Devices: \n");
	for(int i = 0; i < devices.size(); i++)
	{
		log_info("\tUSB[%u]: \n", i);
		log_info("\t\tManufacturer Name: %s\n", devices[i].manufactureName);
		log_info("\t\tProduct Name: %s\n", devices[i].productName);
		log_info("\t\tSerial Number: %s\n", devices[i].serialNumber);
	}

	for (;;);
}
