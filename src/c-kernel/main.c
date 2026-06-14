#include <stdint.h>
#include <handoff/handoff.h>

#include <pools.h>
#include <arch/i686.h>
#include <memory/memory.h>

#include <io/console.h>
#include <drivers/dcom/dcom.h>
#include <c-utils/c-utils.h>

_export void start(KernelInfo kernelInfo)
{
	// initialize the CPU
	initialize_i686();

	// initialize console
	console_t earlyconsole = dcom_get_earlyconsole();
	initialize_console_manager(earlyconsole);
	
	// setup heap allocator
	// init the global allocator on the STACK
	usize heap_obj_size = get_heap_allocator_bytesize();
	u8    tmp_heap_obj_mem[heap_obj_size];
	heap_allocator_t* kalloca = construct_heap(tmp_heap_obj_mem, __global_heap_start, PTR_DIFF_I32(__global_heap_end, __global_heap_start));
	if(IS_INV_PTR(kalloca)) printf("[init] Failed to construct heap. CODE: %x\n", -ERR_CAST(kalloca));
	// setup buddy allocator
	//err_t err = initialize_buddy_allocator(kalloca, kernelInfo.e820_mmap);
	//if(IS_ERROR(err)) printf("[init] Failed to initialize buddy allocator. CODE: %x\n", -err);

	void* test = malloc(kalloca, 0x100);

	//printf("Testing buddy allocator... \n");
	//void* p0 = allocate_pages(1);
	//if(IS_ERR_PTR(p0)) printf("[test] Failed to allocate. CODE: %x\n", ERR_CAST(p0));

	log_heap_status(kalloca);
	log_page_allocator_status();

	printf("Finished Executing, Halting...!\n");
	flush_terminal();
	for (;;);
}
