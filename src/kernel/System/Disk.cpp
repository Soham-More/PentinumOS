#include "Disk.hpp"

#include <std/IO.hpp>
#include <std/stdmem.hpp>

namespace sys
{
    void Disk::load_partitions()
    {
        void* MBR = alloc_pages(DivRoundUp(get_sector_size(), PAGE_SIZE));

        // load MBR
        if(!read_sectors(0, 1, MBR))
        {
            log_error("[Disk] Failed to load Master Boot Record.\n");
        }

        partition* partition_table = (partition*)((char*)MBR + 0x1be);
        this->partition_count = 0;

        for(size_t i = 0; i < 4; i++)
        {
            // is this the end of partition table
            if(partition_table[i].sector_count == 0 && partition_table[i].lba == 0) break;
            this->partition_count++;
        }

        this->disk_partitions = (partition*)std::malloc(sizeof(partition) * this->partition_count);
        memcpy(partition_table, this->disk_partitions, sizeof(partition) * this->partition_count);

        free_pages(MBR, DivRoundUp(get_sector_size(), PAGE_SIZE));
    }

    partition Disk::getPartition(uint8_t index)
    {
        return this->disk_partitions[index];
    }

    partition Disk::getActivePartition()
    {
        for(size_t i = 0; i < this->partition_count; i++)
        {
            if(this->disk_partitions[i].isActive) return this->disk_partitions[i];
        }

        return partition{};
    }
}
