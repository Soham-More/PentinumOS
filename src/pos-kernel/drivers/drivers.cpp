#include "drivers.hpp"

#include "usb/hci/uhci.hpp"

#define KDRIVER_COUNT 1

namespace drivers
{
    typedef kernel_driver(*driver_init_function)(vfs::vfs_t*);
    
    static driver_init_function kdrivers_init_functions[KDRIVER_COUNT] = {
        usb::get_uhci_driver
    };

    static kernel_driver kdrivers[KDRIVER_COUNT];
    
    static uid_t kdriver_count;

    u32 new_uid()
    {
        uid_t uid = kdriver_count;
        kdriver_count++;
        return uid;
    }

    void get_all_kernel_drivers(vfs::vfs_t* gvfs, kernel_driver **drivers, size_t &driver_count)
    {
        for(size_t i = 0; i < KDRIVER_COUNT; i++)
        {
            kdrivers[i] = kdrivers_init_functions[i](gvfs);
            kdrivers[i].uid = i;
        }

        *drivers = kdrivers;
        driver_count = KDRIVER_COUNT;
        kdriver_count = KDRIVER_COUNT;
    }
}
