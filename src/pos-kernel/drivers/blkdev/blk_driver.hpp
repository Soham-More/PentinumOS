#pragma once

#include "../driver_defs.hpp"

#include <blkio/blkio.hpp>

namespace drivers::blk
{
    kernel_driver get_blk_driver(vfs::vfs_t* gvfs);
}


