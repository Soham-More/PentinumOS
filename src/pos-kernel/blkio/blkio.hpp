#pragma once

#include <includes.h>

namespace blkio
{
    enum REQUEST_TYPE
    {
        REQ_INVALID = 0x1,
        REQ_READ = 0x0,
        REQ_WRITE = 0x1,
    };

    struct blk_info_t
    {
        u64 sector_size;
        u32 flags;

        uid_t driver_uid;
    };

    struct partition_t
    {
        u64 begin;
        u64 size;
        blk_info_t* parent_block;
    };

    struct request_t
    {
        u32 type;
        void* buffer;
        u64 block_count;
        blk_info_t* block;

        uid_t pid;
        void (*end_io)(request_t);
    };

    void init();

    void submit_request(request_t& request);
    
    bool poll();
    request_t pop_request();
}


