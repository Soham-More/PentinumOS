#include "hw.hpp"

#include <arch/i686.h>
#include <io/console.hpp>

#include <cpu/boot.hpp>
#include <cpu/paging.hpp>
#include <cpu/memory.hpp>
#include <cpu/stack.hpp>

#include <vfs/vfs.hpp>

// all the drivers
#include "vga/VGA.hpp"
#include "dcom/DebugCOM.hpp"
#include "ps2/PS2.hpp"
#include "ps2/PS2Keyboard.hpp"
#include "pit/PIT.h"

#include "pci/pci.hpp"

// initialize the hardware
vfs::vfs_t* initialize_hw(KernelInfo& kernel_info)
{
    initialize_i686();

    // initialize the console with vga monitor console
    con::init(VGA::get_console());

    printf("Kernel loaded\n");
    printf("initialising VGA Monitor...  Ok");
	clrscr();

	printf("initialising Memory... ");
	// setup paging
	cpu::initializePaging(*kernel_info.pagingInfo);
	// initalise memory manager
	cpu::initialize_memory(kernel_info);
	// invalidate first page
	u32 flags_page_zero = cpu::getFlagsPage(0);
	flags_page_zero &= ~cpu::PAGE_PRESENT;
	cpu::setFlagsPage(0, flags_page_zero);
	// done
	printf("Ok\n");

	printf("initialising Kernel Heap... ");
	std::initialize_heap();
	printf("Ok\n");

	printf("initialising VFS... ");
	
	vfs::vfs_t* g_vfs = vfs::init();
	vfs::add_vnode(g_vfs, "/", vfs::make_node("hw", true));
	vfs::add_vnode(g_vfs, "/", vfs::make_node("dev", true));

	printf("Ok\n");

	printf("initialising PIT...  ");
	PIT_init();
	printf("Ok\n");

	u16 divider = (PIT_MAX_FREQ / 16500) & ~(0x1);

	x86_outb(0x43, 0b00110110);
    x86_outb(0x40, (u8)divider);
    x86_outb(0x40, (u8)(divider >> 8));

	printf("initialising PS2...  ");
	PS2_init();
	printf("Ok\n");

	printf("initialising Keyboard...  ");
	PS2::init_keyboard();
	PS2::flush_keyboard();
	printf("Ok\n");

	printf("initialising PCI...  ");
	bus::pci::init(g_vfs);
	printf("Ok\n");

    return g_vfs;
}


