#pragma once

#include <includes.h>

#define PAGE_SIZE 4096

typedef struct _PagingInfo
{
	uint64_t memorySize;
	u32* pageDirectory;
	u32* pageTableArray;
}_packed PagingInfo;

typedef struct _KernelInfo
{
	u8 bootDrive;
	void* e820_mmap;
	void* kernelMap;
	PagingInfo* pagingInfo;
} _packed KernelInfo;

