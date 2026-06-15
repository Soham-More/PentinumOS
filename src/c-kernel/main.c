#include <stdint.h>
#include <handoff/handoff.h>

#include <pools.h>
#include <hw/cpu.h>
#include <arch/i686.h>
#include <memory/memory.h>
#include <memory/internals.h>

#include <io/console.h>
#include <drivers/dcom/dcom.h>
#include <c-utils/c-utils.h>

u8 g_kalloc_buffer[64];
heap_allocator_t* kalloca;
console_t earlyconsole;

// boot up the kernel,
// setup all important kernel objects and subsystems
// start the first kernel thread
// then terminate the boot thread which is executing this start function
_export void start(KernelInfo kernelInfo)
{
	// initialize the CPU
	initialize_i686();

	// initialize console
	earlyconsole = dcom_get_earlyconsole();
	initialize_console_manager(earlyconsole);
	
	// setup heap allocator
	// init the global allocator on the STACK
	usize heap_obj_size = get_heap_allocator_bytesize();
	if(heap_obj_size > sizeof(g_kalloc_buffer)) {
		printf("[init] Heap allocator object size is larger than the provided global allocator memory. Halting...!\n");
		panic(PANIC_UNEXPECTED_FAILURE);
		return;
	}
	kalloca = construct_heap(g_kalloc_buffer, __global_heap_start, PTR_DIFF_I32(__global_heap_end, __global_heap_start));
	if(IS_INV_PTR(kalloca)) printf("[init] Failed to construct heap. CODE: %x\n", -ERR_CAST(kalloca));
	// setup buddy allocator
	err_t err = initialize_buddy_allocator(kalloca, &kernelInfo);
	if(IS_ERROR(err)) printf("[init] Failed to initialize buddy allocator. CODE: %x\n", -err);

	printf("Finished Executing, Halting...!\n");
	flush_terminal();
	for (;;);
}
