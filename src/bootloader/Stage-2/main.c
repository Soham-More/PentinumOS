#include "includes.h"
#include "IO.h"
#include "memdefs.h"
#include "FAT.h"
#include "Memory.h"
#include "Paging.h"
#include "elf.h"

static MemoryMap g_memoryMap;

typedef struct 
{
	uint8_t bootDrive;
	void* e820_mmap;
	void* kernelMap;
} _packed KernelInfo;

typedef void (*KernelStart)(KernelInfo kernelInfo);

void printBootDriveInfo(uint8_t bootDrive)
{
	// Get bootdrive type
	// Src: Wikipedia 13H Article

	// practically bios always assignes HDD bootdrives 0x80
	// and CD drives 0x00???? For some reason bios differentiates between devices
	// based on the fomatting
	// which explains why bios hangs when a FAT12 formatted USB device
	// is inserted
	const char* driveTypes[] = { "floppy drive", "HDD", "CD" };
	const char* currentDrive;
	if(bootDrive >= 0 && bootDrive < 0x80)
	{
		currentDrive = driveTypes[0];
	}
	else
	{
		currentDrive = driveTypes[1];

		bootDrive -= 0x80;
	}

	printf("Loaded from %s %c:\n", currentDrive, 'A' + bootDrive);
}

void _cdecl start(uint8_t bootDrive, void* e820_mmap)
{
	clrscr();

	printf("File: stage2.bin\n");

	printf("Stage2: Loaded, BootDrive ID: %d\n", bootDrive);

	printBootDriveInfo(bootDrive);

	prettyPrintE820MMAP(e820_mmap);

	if(!initialiseFAT(bootDrive))
	{
		printf("ERROR: Failed to initialise FAT driver\n");
		goto exit;
	}

	void* kernelEntry = nullptr;
	void* kernelMap = nullptr;

	if(!loadELF("kernel.elf", &kernelEntry, &kernelMap))
	{
		printf("ERROR: Failed to load kernel");
		goto exit;
	}

	initialisePages(getMemorySize(e820_mmap));

	KernelInfo kernelInfo;
	kernelInfo.bootDrive = bootDrive;
	kernelInfo.e820_mmap = e820_mmap;
	kernelInfo.kernelMap = kernelMap;

	KernelStart beginKernel = (KernelStart)kernelEntry;

	beginKernel(kernelInfo);

	exit:
	for(;;);
}