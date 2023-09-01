#include "Memory.h"
#include "memdefs.h"
#include "includes.h"
#include "IO.h"
#include "x86.h"

typedef struct MMAP_ENTRY
{
	uint64_t baseAddress;
	uint64_t regionSize;
	uint32_t regionType;
	uint32_t ACPI3_eattrib_bf;
} _packed MMAP_ENTRY;

void memcpy(void* src, void* dst, uint32_t size)
{
    for(uint32_t i = 0; i < size; i++)
    {
        ((char*)dst)[i] = ((char*)src)[i];
    }
}

void memset(void* src, uint8_t value, uint32_t size)
{
	for(uint32_t i = 0; i < size; i++)
    {
        ((char*)src)[i] = value;
    }
}

uint32_t bytesToSectors(uint32_t bytes)
{
	return (bytes + SECTOR_SIZE - 1) / SECTOR_SIZE;
}

static uint32_t allocated_sectors = 0;
void* alloc(uint32_t sector_size)
{
	void* m_allocated = MEMORY_FAT_ADDR + allocated_sectors * SECTOR_SIZE;
	allocated_sectors += sector_size;

	if(allocated_sectors * SECTOR_SIZE >= MEMORY_FAT_SIZE)
	{
		printf("Alloc Error: Ran out of usable memory\n");
		printf("allocated sectors: %u, requested: %u\n", allocated_sectors, sector_size);
		return nullptr;
	}

	return m_allocated;
}

void popAllocMem(uint32_t sectors)
{
    allocated_sectors -= sectors;
}

const char* mmap_types[6] = { "free", "reserved", "ACPI reclaimable", "ACPI NVS", "bad memory", "unknown" };

const char* get_mmap_type(uint32_t type)
{
	if(type == 0) return mmap_types[5];
	if(type > 5) return mmap_types[5];

	return mmap_types[type - 1];
}

void prettyPrintE820MMAP(void* e820_mmap)
{
	uint32_t entry_count = *(uint32_t*)e820_mmap;

	MMAP_ENTRY* list = (MMAP_ENTRY*)((uint32_t*)e820_mmap + 1);

	uint64_t RAM_size = 0;
	uint64_t free_ram_size = 0;

	printf("\nMemory Map: \n");
	printf("\t%16s | %16s | %16s | size(in bytes)\n", "start", "end", "type");

	for(uint32_t i = 0; i < entry_count; i++)
	{
		RAM_size += list[i].regionSize;

		if(list[i].regionType == MMAP_TYPE_FREE) free_ram_size += list[i].regionSize;

		printf("\t%16llx | %16llx | %16s | %u\n", list[i].baseAddress, list[i].baseAddress + list[i].regionSize, get_mmap_type(list[i].regionType), list[i].regionSize);
	}

	printf("RAM size: %llu bytes(%llu MiB)\n", RAM_size, RAM_size >> 20);
	printf("Free RAM: %llu bytes(%llu MiB)\n", free_ram_size, free_ram_size >> 20);
}

uint64_t getMemorySize(void* e820_mmap)
{
	uint32_t entry_count = *(uint32_t*)e820_mmap;

	MMAP_ENTRY* list = (MMAP_ENTRY*)((uint32_t*)e820_mmap + 1);

	uint64_t RAM_size = 0;
	for(uint32_t i = 0; i < entry_count; i++)
	{
		RAM_size = (list[i].baseAddress + list[i].regionSize);
	}

	return RAM_size;
}
