#include "FAT.h"
#include "IO.h"
#include "Memory.h"
#include "Disk.h"

#include "FAT12.h"
#include "FAT32.h"

typedef struct Partition
{
	uint8_t isActive;
	uint8_t firstCHS[3];
	uint8_t type;
	uint8_t lastCHS[3];
	uint32_t lba;
	uint32_t sector_count;
} _packed Partition;

DISK bootDisk;
uint32_t currentPartitionOffset = 0;
bool isFAT32 = false;

// FAT API

bool initialiseFAT(uint8_t drive)
{
	void* MBR = alloc(1);

	if(!disk_initialise(drive, &bootDisk))
	{
		return false;
	}

	// load MBR
	if(!disk_readSectors(bootDisk, 0, 1, MBR))
	{
		return false;
	}

	Partition* partition_table = (Partition*)((char*)MBR + 0x1be);

	currentPartitionOffset = partition_table[0].lba;

	if(!disk_readSectors(bootDisk, currentPartitionOffset, 1, MBR))
	{
		return false;
	}

	if(*(uint16_t*)((char*)MBR + 0x16) == 0)
	{
		isFAT32 = true;
	}

	printf("\nDetected FileSystem: %s\n", isFAT32 ? "FAT32" : "FAT12");

	// free memory
	popAllocMem(1);

	return isFAT32 ? FAT32_initialise(bootDisk, currentPartitionOffset) : FAT12_initialise(bootDisk, currentPartitionOffset);
}
bool searchFile(const char* filepath)
{
	return isFAT32 ? FAT32_search(filepath) : FAT12_search(filepath);
}

FILE * fopen(const char* filename)
{
	return isFAT32 ? FAT32_open(filename) : FAT12_open(filename);
}

char fgetc(FILE * file)
{
	return isFAT32 ? FAT32_getc(file) : FAT12_getc(file);
}
bool fread(FILE * file, char* buffer, uint32_t size)
{
	return isFAT32 ? FAT32_read(file, buffer, size) : FAT12_read(file, buffer, size);
}
bool isEOF(FILE* file)
{
	return (isFAT32 ? FAT32_isEOF(file) : FAT12_isEOF(file));
}
uint32_t fsize(FILE* file)
{
	return isFAT32 ? FAT32_fsize(file) : FAT12_fsize(file);
}
bool fseek(FILE* file, uint32_t value, uint8_t seek_mode)
{
	return isFAT32 ? FAT32_seek(file, value, seek_mode) : FAT12_seek(file, value, seek_mode);
}
void fclose(FILE * file)
{
	isFAT32 ? FAT32_close(file) : FAT12_close(file);
}
