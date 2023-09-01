#pragma once
#include <stdint.h>
#include <stdbool.h>

#define MMAP_TYPE_USED_CRITICAL     0
#define MMAP_TYPE_FREE              1
#define MMAP_TYPE_RESERVED          2
#define MMAP_TYPE_ACPI_RECLAIMABLE  3
#define MMAP_TYPE_ACPI_NVS          4
#define MMAP_TYPE_BAD               5

#define ACPI_ATTRIB_IGNORE          0x01
#define ACPI_ATTRIB_NON_VOLATILE    0x02

typedef struct MMapEntry
{
    uint64_t begin;
    uint64_t end;
    uint32_t type;
    uint32_t ACPI_attrib;
 } MMapEntry;

typedef struct MemoryMap
{
    bool isACPIenabled;
    uint32_t size;
    MMapEntry* entries;
} MemoryMap;

void memcpy(void* src, void* dst, uint32_t size);
void memset(void* src, uint8_t value, uint32_t size);

uint32_t bytesToSectors(uint32_t bytes);

void* alloc(uint32_t sector_size);
void popAllocMem(uint32_t sectors);

uint32_t get_MMAP_size();
MemoryMap getMemoryMap();

void printMemoryMap();

void prettyPrintE820MMAP(void* e820_mmap);

uint64_t getMemorySize(void* e820_mmap);
