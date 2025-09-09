#pragma once

#include <arch/i686.h>
#include <io/console.hpp>

#include <cpu/boot.hpp>
#include <cpu/paging.hpp>
#include <cpu/memory.hpp>
#include <cpu/stack.hpp>

#include <vfs/vfs.hpp>

// initialize the hardware
vfs::vfs_t* initialize_hw(KernelInfo& kernel_info);
