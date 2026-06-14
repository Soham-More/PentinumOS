#pragma once

#include <includes.h>
#include <memory/priv.h>

typedef struct E820_MMAP_ENTRY
{
	u64 baseAddress;
	u64 regionSize;
	u32 regionType;
	u32 ACPI3_eattrib_bf;
} _packed E820_MMAP_ENTRY;

typedef struct KernelMapEntry
{
    u32 sectionBegin;
    u32 sectionSize;
}_packed KernelMapEntry;

typedef struct KernelMap
{
    u32 entryCount;
    KernelMapEntry* entries;
}_packed KernelMap;

typedef struct _PagingInfo
{
	u64  memorySize;
	u32* pageDirectory;
	u32* pageTableArray;
}_packed PagingInfo;

typedef struct _KernelInfo
{
	u8 bootDrive;
	void* e820_mmap;
	KernelMap* kernelMap;
	PagingInfo* pagingInfo;
} _packed KernelInfo;

// returns enum page_flags_t type
u32 hdf_mmap_entry_type(E820_MMAP_ENTRY* entry);

