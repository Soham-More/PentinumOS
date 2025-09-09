#pragma once

#include <includes.h>
#include <vfs/vfs.hpp>

#define DRIVER_SUCCESS 0x0
#define DRIVER_PARSE_FAIL 0x1
#define DRIVER_EXEC_FAIL  0x2

namespace drivers
{
    struct kernel_driver
    {
        // driver name and desciption
        const char* name;
        const char* desc;

        // the filesystem
        vfs::vfs_t* gvfs;
        void* data;

        // some driver functions
        const char* (*get_error_desc)(u32 error);
        u32 (*init)(kernel_driver*);
        u32 (*process_event)(kernel_driver*, vfs::event_t);

        uid_t uid;
        uid_t uid_class;
    };
}

