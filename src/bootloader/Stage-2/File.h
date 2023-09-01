#pragma once

#include "includes.h"

#define FAT_MAX_OPEN_FILES 16
#define FAT_SEARCH_FAIL UINT32_MAX

typedef struct
{
	uint8_t filename[11];
	uint8_t attributes;
	uint8_t reserved_winNT;
	uint8_t creationTime_ds;
	uint16_t creationTime;
	uint16_t creationDate;
	uint16_t lastAccessDate;
	uint16_t firstCluster_high; // useless for FAT12/16
	uint16_t lastModifyTime;
	uint16_t lastModifyDate;
	uint16_t firstCluster_low;
	uint32_t fileSize;
} _packed DirectoryEntry;

enum File_Attributes
{
    FAT_INVALID_ENTRY = 0x00,
    FAT_READ_ONLY     = 0x01,
    FAT_HIDDEN        = 0x02,
    FAT_SYSTEM        = 0x04,
    FAT_VOLUME_ID     = 0x08,
    FAT_DIRECTORY     = 0x10,
    FAT_ARCHIVE       = 0x20,
    FAT_LFN           = FAT_READ_ONLY | FAT_HIDDEN | FAT_SYSTEM | FAT_VOLUME_ID
};

enum SEEK_MODE
{
    FAT_SEEK_BEGINNING = 0x00,
    FAT_SEEK_CURRENT   = 0x01,
};

typedef struct FILE
{
    uint16_t sector_handle;           // memory allocated to this file
    uint32_t currentCluster;          // current cluster
    uint32_t clusterPosition;         // position wrt cluster begin
    uint32_t position;                // position in file
    DirectoryEntry directory;         // entry data in directory
} FILE;
