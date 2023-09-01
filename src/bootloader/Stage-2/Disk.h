#pragma once

#include <stdint.h>
#include <stdbool.h>

typedef struct DISK
{
    uint8_t driveType;
    uint16_t cyl_count;
    uint16_t sector_count;
    uint16_t head_count;
    uint16_t drive_id;
} DISK;

bool disk_initialise(int drive, DISK* disk);
bool disk_readSectors(DISK disk, uint32_t lba, uint16_t no_of_sectors, void* read_bytes);
