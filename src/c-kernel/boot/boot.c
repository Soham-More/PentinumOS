#include <stdint.h>
#include <stdarg.h>

#include <boot/init.h>
#include <handoff/handoff.h>

#include <pools.h>
#include <panic/panic.h>
#include <resources/timer.h>
#include <arch/i686.h>
#include <arch/IRQ/PIC.h>
#include <utils/heap.h>
#include <utils/heap.h>
#include <arch/paging/paging.h>

#include <resources/console.h>
#include "dcom/dcom.h"
#include "vga_txt/vga_txt.h"

#include <utils/logger.h>
#include <syscore/threads.h>

#include <threads/idle/idle.h>

u8 g_kalloc_buffer[64];
thread_mutex_t g_shared_object_mutex;
heap_allocator_t* g_shared_object_allocator;

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
	tty_t* tty0 = initialize_tty("tty0", con_get_safe());

	logging_set_tty(tty0);

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

	g_shared_object_allocator = kalloca;
	g_idle_thread_init.allocator = kalloca;
	g_idle_thread_init.page_table = idle_ptable;

	// spawns the idle thread
	// and kills the boot thread which is executing this function
	// this thread must die, as this thread is not a real kernel thread
	kmt_spawn_idle_thread(idle_thread_entry, g_idle_thread_init.page_table);
}

heap_allocator_t* m_aquire_global_allocator() {
	err_t err = kmt_lock_mutex(g_shared_object_mutex);
	if(err != ESUCCESS) {
		panic(PANIC_UNEXPECTED_FAILURE, "Failed to acquire global allocator lock");
		return nullptr;
	}
	return g_shared_object_allocator;
}
void m_release_global_allocator(volatile heap_allocator_t** allocator) {
	err_t err = kmt_unlock_mutex(g_shared_object_mutex);
	if(err != ESUCCESS) {
		panic(PANIC_UNEXPECTED_FAILURE, "Failed to release global allocator lock");
	}
}

