#pragma once
#include "includes.h"

//DiskAddressPacket:
//    .size:              db 10h
//                        db 0
//    .count:             dw 0
//    .offset:            dw 0
//    .segment:           dw 0
//    .lba:               dq 0

typedef struct DataAddressPacket
{
    // must be 10h
    uint8_t sizeDAP;
    // must be 0
    uint8_t reserved;
    // no of sectors to read
    uint16_t sector_count;
    // offset of memory to write to
    uint16_t offset;
    // segement of memory to write to
    uint16_t segment;
    // lba of first sector to read
    uint64_t lba;
} _packed DataAddressPacket;

// bus
void _cdecl x86_outb(uint16_t port, uint8_t value);
uint8_t _cdecl x86_inb(uint16_t port);

// Disk
bool _cdecl x86_read_disk(uint8_t drive_no, uint16_t cylinder, uint16_t sector_no, uint16_t head, uint8_t no_of_sectors_to_read, void* read_bytes);
bool _cdecl x86_reset_disk(uint8_t drive_no);
bool _cdecl x86_read_disk_params(uint8_t drive_no, uint8_t* driveType, uint16_t* cylinder_count, uint16_t* sector_count, uint16_t* head_count);

bool _cdecl x86_read_disk_ext(uint8_t drive_no, DataAddressPacket* dap);

// Memory
void _cdecl x86_set_page_directory(void* page_directory_ptr);
uint32_t _cdecl x86_get_cr0_register();

uint32_t _cdecl x86_flushTLB();
