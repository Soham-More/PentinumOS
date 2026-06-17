#include <stdint.h>
#include <stdarg.h>
#include <handoff/handoff.h>

#include <pools.h>
#include <hw/cpu.h>
#include <hw/timer.h>
#include <arch/i686.h>
#include <arch/IRQ/PIC.h>
#include <memory/memory.h>
#include <memory/internals.h>
#include <arch/paging/paging.h>

#include <io/console.h>
#include <drivers/dcom/dcom.h>
#include <drivers/vga_txt/vga_txt.h>
#include <c-utils/c-utils.h>

#include <io/logger.h>
#include <multitasking/kernel.h>

#include <idle/idle.h>

u8 g_kalloc_buffer[64];

// boot up the kernel,
// setup all important kernel objects and subsystems
// start the first kernel thread
// then terminate the (boot) thread which is executing this start function
_export void start(KernelInfo kernelInfo)
{
	// initialize the CPU
	initialize_i686();

	// initialize console
	g_idle_thread_init.earlyconsole = vga_txt_get_earlyconsole();
	initialize_console_manager(g_idle_thread_init.earlyconsole);
	clrscr();

	log_info("Starting kernel initialization...\n");
	log_info("Finished i686 CPU initialization...\n");
	
	// setup heap allocator
	// init the global allocator on the STACK
	heap_allocator_t* kalloca = nullptr;
	usize heap_obj_size = get_heap_allocator_bytesize();
	if(heap_obj_size > sizeof(g_kalloc_buffer)) {
		panic(PANIC_UNEXPECTED_FAILURE, "Heap allocator object size is larger than the provided global allocator memory");
		return;
	}
	kalloca = construct_heap(g_kalloc_buffer, __root_heap_start, PTR_DIFF_I32(__root_heap_end, __root_heap_start));
	panic_on_err_ptr(kalloca, "Failed to construct heap");
	log_info("idle thread heap... ok\n");

	// setup paging
	x86_mmu_map_t idle_ptable;
	idle_ptable = x86_from_handoff(kernelInfo.pagingInfo);
	log_info("x86 paging... ok\n");

	// setup buddy allocator
	panic_on_err(initialize_buddy_allocator(kalloca, &kernelInfo), "Failed to initialize buddy allocator");
	log_info("buddy allocator... ok\n");

	// initialize timer for multitasking preemption
	initialize_timer();
	log_info("PIT timer... ok\n");
	
	// setup multitasking
	initialize_multitasking(&idle_ptable, kalloca);
	// setup logging after multitasking is enabled, so that logging is thread safe
	logging_initialize_post_multitasking();
	log_info("multitasking... ok\n");
	log_info("handoff to idle thread\n");

	g_idle_thread_init.allocator = kalloca;
	g_idle_thread_init.page_table = idle_ptable;

	// spawns the idle thread
	// and kills the boot thread which is executing this function
	// this thread must die, as this thread is not a real kernel thread
	panic_on_err(kmt_spawn_idle_thread(&(kthread_desc_t){
		.name = "kidle",
		.ptable = g_idle_thread_init.page_table,
		.stack_top = __idle_thread_exec_stack_end - 4,
		.interrupt_stack_top = __idle_thread_intr_stack_end - 4,
		.entry = idle_thread_entry,
		.heap_base = __idle_heap_start,
		.heap_size = PTR_DIFF_I32(__idle_heap_start, __idle_heap_end),
		.priority = 1,
		.policy = KMT_POLICY_ROUND_ROBIN,
	}), "Failed to spawn idle thread");
}
