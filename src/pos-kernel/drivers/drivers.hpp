#pragma once

#include "driver_defs.hpp"

namespace drivers
{
    u32 new_uid();

    void get_all_kernel_drivers(vfs::vfs_t* gvfs, kernel_driver** drivers, size_t& driver_count);
}


