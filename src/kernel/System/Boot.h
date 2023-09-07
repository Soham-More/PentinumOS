#pragma once

#include <includes.h>

#define PAGE_SIZE 4096

typedef struct _PagingInfo
{
	uint64_t memorySize;
	uint32_t* pageDirectory;
	uint32_t* pageTableArray;
}_packed PagingInfo;

typedef struct _KernelInfo
{
	uint8_t bootDrive;
	void* e820_mmap;
	void* kernelMap;
	PagingInfo* pagingInfo;
} _packed KernelInfo;
