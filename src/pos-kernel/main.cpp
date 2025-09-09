#include <cpp/defines.hpp>
#include <stdint.h>

#include <io/io.h>
#include <hw/hw.hpp>
#include <cpu/exceptions.hpp>
#include <std/std.hpp>
#include <hw/pci/pci.hpp>

#include <std/memory/heap.hpp>

#include <drivers/drivers.hpp>

// export "C" to prevent g++ from
// mangling this function's name
// and confusing the linker
_export void start(KernelInfo kernelInfo)
{
	_init();

	vfs::vfs_t* g_vfs = initialize_hw(kernelInfo);

	// setup stack trace
    cpu::beginStackTrace(reinterpret_cast<void*>(start));
	// setup exceptions
	cpu::initialize_exceptions();

	//vfs::log_children(g_vfs, "/hw/pci");
    //bus::pci::pretty_print_bus(g_vfs);

	size_t driver_count;
	drivers::kernel_driver* drivers;

	// get all kernel drivers
	drivers::get_all_kernel_drivers(g_vfs, &drivers, driver_count);

	for(size_t i = 0; i < driver_count; i++)
	{
		log_info("Initializing %s... ", drivers[i].name);
		u32 error_code = drivers[i].init(&drivers[i]);
		
		// serious error
		if(error_code >= DRIVER_EXEC_FAIL)
		{
			const char* desc = drivers[i].get_error_desc(error_code);
			log_error("Failed!\nerror(%u): %s\n", error_code, desc);
			goto finish;
		}

		log_info("Success\n");
	}

	while (true)
	{
		if(!vfs::poll(g_vfs)) continue;
		vfs::event_t event = vfs::pop_event(g_vfs);
		if(event.flags == vfs::EVENT_INVALID) continue;

		log_info("[VFS] Processing event flags=%x, due to node %s\n", event.flags, event.trigger_node->name);

		if(UID_IS_DRIVER_HINT(event.driver_uid))
		{
			// it is a hint
			// find the driver that might handle this
			for(size_t i = 0; i < driver_count; i++)
			{
				if(drivers[i].uid_class != event.driver_uid) continue;
				log_info("[VFS] Found driver to process event: %s\n", drivers[i].name);
				u32 error_code = drivers[i].process_event(&drivers[i], event);

				if(error_code == DRIVER_SUCCESS)
				{
					log_info("Successfully handled event\n");
					break;
				}
				else if(error_code >= DRIVER_EXEC_FAIL)
				{
					const char* desc = drivers[i].get_error_desc(error_code);
					log_error("[VFS] Failed to process event!\nerror(%u): %s\n", error_code, desc);
					goto finish;
				}
			}
		}
		else
		{
			// it the exact driver
			if(event.driver_uid >= driver_count)
			{
				log_warn("[VFS] Event requested non-existent driver, skipping event\n");
				continue;
			}

			size_t i = event.driver_uid;
			
			if(drivers[i].uid_class != event.driver_uid) continue;
			u32 error_code = drivers[i].process_event(&drivers[i], event);

			if(error_code == DRIVER_SUCCESS) break;
			else if(error_code >= DRIVER_EXEC_FAIL)
			{
				const char* desc = drivers[i].get_error_desc(error_code);
				log_error("[VFS] Failed to process event!\nerror(%u): %s\n", error_code, desc);
				goto finish;
			}
		}
		
	}

finish:
	printf("Finished Executing, Halting...!\n");
	for (;;);
}
