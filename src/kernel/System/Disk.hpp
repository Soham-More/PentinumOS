#pragma once

#include <includes.h>

namespace sys
{
    struct partition
    {
        uint8_t  isActive;
        uint8_t  firstCHS[3];
        uint8_t  fsID;
        uint8_t  lastCHS[3];
        uint32_t lba;
        uint32_t sector_count;
    } _packed;

    struct Disk
    {
        partition* disk_partitions;
        size_t partition_count;

        void load_partitions();
        partition getPartition(uint8_t index);
        partition getActivePartition();

        virtual uint32_t get_sector_size() = 0;
        virtual bool read_sectors(uint32_t lba, size_t count, void* buffer) = 0;
    };
}
