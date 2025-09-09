#include "blk_driver.hpp"

namespace drivers::blk
{
    struct kdriver_data
    {
        ;
    };

    const char* kdriver_error_desc(u32 error)
    {
        const char* error_array[] = {
            "No Failiure",
            "Event parsing failed[this driver does not handle the given event]",
            "Critical Failiure",
            "Failed to initialize uhci controller detected on pci bus"
        };

        if(error >= sizeof(error_array)/sizeof(char*)) return nullptr;

        return error_array[error];
    }
    u32 kdriver_init(kernel_driver* driver)
    {
        driver->data = new kdriver_data();

        return DRIVER_SUCCESS;
    }
    u32 kdriver_process_event(kernel_driver* driver, vfs::event_t event)
    {
        if(event.flags != vfs::EVENT_BLOCK_ADD) return DRIVER_PARSE_FAIL;

        ;

        return DRIVER_SUCCESS;
    }

    kernel_driver get_blk_driver(vfs::vfs_t *gvfs)
    {
        return kernel_driver{
            // driver name and desciption
            .name = "dvr_uhci",
            .desc = "USB 1.0 UHCI Driver",

            // the filesystem
            .gvfs = gvfs,
            .data = nullptr,

            // some driver functions
            .get_error_desc = kdriver_error_desc,
            .init = kdriver_init,
            .process_event = kdriver_process_event,

            // the class of the driver
            .uid_class = UID_CLASS_USBCTRL
        };
    }
}
