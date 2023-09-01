#include "FAT32.h"
#include "IO.h"
#include "memdefs.h"
#include "Memory.h"
#include "String.h"
#include "Math.h"

#define FAT32_CLUSTER_END UINT32_MAX
#define FAT32_CLUSTER_ERROR UINT32_MAX - 1

typedef struct FAT32_BS
{
    // FAT BPB
	uint8_t 		bootjmp[3];
	uint8_t 		oem_name[8];
	uint16_t 	    bytes_per_sector;
	uint8_t  		sectors_per_cluster;
	uint16_t		reserved_sector_count;
	uint8_t	 		table_count;
	uint16_t		root_entry_count;
	uint16_t		total_sectors_16;
	uint8_t			media_type;
	uint16_t		sectors_per_fat_16;
	uint16_t		sectors_per_track;
	uint16_t		head_side_count;
	uint32_t 		hidden_sector_count;
	uint32_t 		total_sectors_32;

    // FAT32 EBPB
    uint32_t        sectors_per_fat;
    uint16_t        flags;
    uint16_t        version;
    uint64_t        root_cluster;
    uint16_t        sectorFSInfo;
    uint16_t        backupBootSector;
    uint8_t         reserved[12];
    uint8_t         driveID;
    uint8_t         flagsNT;
    uint8_t         signature;
    uint8_t         volumeID[4];
    uint8_t         volumeLable[11];
    uint8_t         systemID[8];
} _packed FAT32_BS;

typedef struct FAT32
{
    DISK disk;
    FAT32_BS bootSector;
	uint16_t bytes_per_cluster;
	uint32_t fs_data;					// first data sector
	uint32_t fs_fat;					// first sector of FAT
	uint32_t sc_data;					// data sector count
	uint32_t tc_cluster;				// total cluster count
    uint32_t tc_sectors;                // total sector count
} FAT32;

FAT32 bootDrive;
DirectoryEntry invalidEntry;
DirectoryEntry root;

static void* m_freeSector = nullptr;
static uint32_t* FAT32_table = nullptr;
static FILE * open_files = nullptr;
static void * open_file_clusters = nullptr;
static void * m_freeCluster = nullptr;

uint32_t currentFATSector = 0;

// FAT32 specific private functions
void * getFileCluster(uint16_t file_sector_id)
{
	return (char *)open_file_clusters + file_sector_id * SECTOR_SIZE * bootDrive.bootSector.sectors_per_cluster;
}
uint32_t loadNextCluster(uint16_t sector_handle, uint16_t currentCluster)
{
	uint32_t fat_offset = currentCluster;
    uint32_t fat_sector = bootDrive.fs_fat + ((fat_offset * 4) / SECTOR_SIZE);
    uint32_t ent_offset = fat_offset % SECTOR_SIZE;
    
    // read the sector if it is not already loaded
    if(fat_sector != currentFATSector)
    {
        if(!disk_readSectors(bootDrive.disk, fat_sector, 1, FAT32_table))
        {
            return FAT32_CLUSTER_ERROR;
        }
    }
    
    //remember to ignore the high 4 bits.
    uint32_t table_value = FAT32_table[ent_offset];
    table_value &= 0x0FFFFFFF;

	// no more clusters
	if(table_value >= 0x0FFFFFF8) return FAT32_CLUSTER_END;

	uint16_t cluster_lba = ((table_value - 2) * bootDrive.bootSector.sectors_per_cluster) + bootDrive.fs_data;

	if(!disk_readSectors(bootDrive.disk, cluster_lba, bootDrive.bootSector.sectors_per_cluster, getFileCluster(sector_handle)))
	{
		return FAT32_CLUSTER_ERROR;
	}

	return table_value;
}
uint32_t getNextCluster(uint16_t currentCluster)
{
    uint32_t fat_offset = currentCluster;
    uint32_t fat_sector = bootDrive.fs_fat + ((fat_offset * 4) / SECTOR_SIZE);
    uint32_t ent_offset = fat_offset % SECTOR_SIZE;
    
    // read the sector if it is not already loaded
    if(fat_sector != currentFATSector)
    {
        if(!disk_readSectors(bootDrive.disk, fat_sector, 1, FAT32_table))
        {
            return FAT32_CLUSTER_ERROR;
        }

		currentFATSector = fat_sector;
    }
    
    //remember to ignore the high 4 bits.
    uint32_t table_value = FAT32_table[ent_offset];
    table_value &= 0x0FFFFFFF;

	// no more clusters
	if(table_value >= 0x0FFFFFF8) return FAT32_CLUSTER_END;

	return table_value;
}
bool loadCluster(uint16_t sector_handle, uint16_t cluster)
{
	uint16_t cluster_lba = ((cluster - 2) * bootDrive.bootSector.sectors_per_cluster) + bootDrive.fs_data;

	return disk_readSectors(bootDrive.disk, cluster_lba, bootDrive.bootSector.sectors_per_cluster, getFileCluster(sector_handle));
}
bool isFileClosed(FILE * file)
{
	return file->sector_handle == UINT16_MAX;
}

// Checks if the increment in position valid
bool isValidIncrement(FILE * file, uint32_t inc)
{
	return (file->clusterPosition + inc) < bootDrive.bytes_per_cluster && (file->position + inc) < file->directory.fileSize;
}

void add_padding_dir_name(char * dir_name)
{
	char new_dir_name[11] = "           ";

	// if this has a dot, split it
	uint16_t dot_pos = find_char(dir_name, '.');
	if(dot_pos != FIND_FAIL)
	{
		uint16_t i = 0;
		for(;i < dot_pos; i++)
		{
			new_dir_name[i] = toUpper(dir_name[i]);
		}

		i = dot_pos + 1;

		new_dir_name[10] = toUpper(dir_name[i + 2]);
		new_dir_name[ 9] = toUpper(dir_name[i + 1]);
		new_dir_name[ 8] = toUpper(dir_name[i + 0]);

		memcpy(new_dir_name, dir_name, 11);

		return;
	}

	for(uint16_t i = 0; i < 11 && dir_name[i] != null; i++)
	{
		new_dir_name[i] = toUpper(dir_name[i]);
	}

	memcpy(new_dir_name, dir_name, 11);
}

// Searches a directory in a given directory
// if currentDir is nullptr, it searches in root directory
bool search_directory(DirectoryEntry* currentDir, char * dir_name, DirectoryEntry* dir)
{
	if(currentDir->attributes != FAT_DIRECTORY)
	{
		return false;
	}

	uint16_t currentCluster = (currentDir->firstCluster_high << 16) + currentDir->firstCluster_low;

	while(currentCluster != FAT32_CLUSTER_END)
	{
		// load into m_freeCluster
		if(!loadCluster(FAT_MAX_OPEN_FILES, currentCluster))
		{
			// failed to load cluster
			printf("ERROR: Failed to read cluster while parsing directory %S\n", currentDir->filename, 11);
			return false;
		}

		DirectoryEntry* directories = m_freeCluster;

		for(uint16_t i = 0; i * sizeof(DirectoryEntry) < bootDrive.bytes_per_cluster; i++)
		{
			// End of current directory
			if(directories[i].filename[0] == null)
			{
				return false;
			}
			
			// Unused Directory, Skip Entry
			if(directories[i].filename[0] == (uint8_t)0xE5)
			{
				continue;
			}

			// if found, return true
			if(strcmp(dir_name, directories[i].filename, 11))
			{
				*dir = directories[i];
				return true;
			}
		}

		// if not found yet, check for another cluster
		currentCluster = getNextCluster(currentCluster);
	}

	// Filename not founud
	return false;
}
DirectoryEntry search_file_wsplit_path(SplitString* path_to_file)
{
	DirectoryEntry A = root; // start searching from root
	DirectoryEntry B;

	DirectoryEntry* currentEntry = &A;
	DirectoryEntry* nextEntry = &B;

	char* dir_name = nullptr;

	for(uint16_t i = 0; i < path_to_file->split_count; i++)
	{
		dir_name = getSplitString(path_to_file, i);
		add_padding_dir_name(dir_name);

		if(!search_directory(currentEntry, dir_name, nextEntry))
		{
			return invalidEntry;
		}

		swap(currentEntry, nextEntry, DirectoryEntry*);
	}

	return *currentEntry;
}
DirectoryEntry getFileDirectory(const char* path)
{
	SplitString splitPath;

	splitPath.split_count = count_char_occurrence(path, '/') + 1;
	splitPath.split_string_size = 11;
	splitPath.string_array = alloc(bytesToSectors(splitPath.split_count * 11));

	split_wrt_char(path, '/', &splitPath);

	DirectoryEntry fileDirectory = search_file_wsplit_path(&splitPath);

	popAllocMem(splitPath.split_count * 11);

	return fileDirectory;
}

// FAT API

bool FAT32_initialise(DISK bootdisk, uint32_t partition_offset)
{
	bootDrive.disk = bootdisk;

	m_freeSector = alloc(1);

	// load bootsector on active partition
	if(!disk_readSectors(bootDrive.disk, partition_offset, 1, m_freeSector))
	{
		return false;
	}

	memcpy(m_freeSector, &bootDrive.bootSector, sizeof(FAT32_BS));

	bootDrive.fs_data = partition_offset + bootDrive.bootSector.reserved_sector_count + (bootDrive.bootSector.table_count * bootDrive.bootSector.sectors_per_fat);
	bootDrive.fs_fat = partition_offset + bootDrive.bootSector.reserved_sector_count;
	bootDrive.sc_data = bootDrive.bootSector.total_sectors_16 - (bootDrive.bootSector.reserved_sector_count + (bootDrive.bootSector.table_count * bootDrive.bootSector.sectors_per_fat));
	bootDrive.tc_cluster = bootDrive.sc_data / bootDrive.bootSector.sectors_per_cluster;
	bootDrive.bytes_per_cluster = bootDrive.bootSector.bytes_per_sector * bootDrive.bootSector.sectors_per_cluster;

	invalidEntry.attributes = FAT_INVALID_ENTRY;

    // get root directory first cluster
    root.firstCluster_low = bootDrive.bootSector.root_cluster % (1 << 16);
    root.firstCluster_high = bootDrive.bootSector.root_cluster >> 16;

    root.attributes = FAT_DIRECTORY;

    // allocate only 1 sector, loading all sectors will make us run out of memory
	FAT32_table = (uint32_t *)alloc(1);

	currentFATSector = bootDrive.fs_fat;

	if(!disk_readSectors(bootDrive.disk, bootDrive.fs_fat, 1, (void *)FAT32_table))
	{
		printf("FAT Fail: Failed to load root FAT tables\n");
		return false;
	}

	open_files = alloc((FAT_MAX_OPEN_FILES * sizeof(FILE) + SECTOR_SIZE - 1) / SECTOR_SIZE);

	// NOTE: m_freeCluster must be allocated right after open_file_clusters alloc
	open_file_clusters = alloc(FAT_MAX_OPEN_FILES * bootDrive.bootSector.sectors_per_cluster);
	m_freeCluster = alloc(bootDrive.bootSector.sectors_per_cluster);

	return true;
}
bool FAT32_search(const char* filepath)
{
	return getFileDirectory(filepath).attributes != FAT_INVALID_ENTRY;
}

void FAT32_pretty_print_path(const char* filepath)
{
	SplitString splitPath;

	splitPath.split_count = count_char_occurrence(filepath, '/') + 1;
	splitPath.split_string_size = 11;
	splitPath.string_array = alloc(bytesToSectors(splitPath.split_count * 11));

	split_wrt_char(filepath, '/', &splitPath);

	printf("\nsub-directories: [size:%u] [sectors:%u]\n", splitPath.split_count, bytesToSectors(splitPath.split_count * 11));

	for(uint16_t i = 0; i < splitPath.split_count; i++)
	{
		printf("    %S\n", getSplitString(&splitPath, i), 11);
	}

	printf("\n");

	popAllocMem(bytesToSectors(splitPath.split_count * 11));
}

static uint16_t open_file_count = 0;
FILE * FAT32_open(const char* filename)
{
	uint16_t file_handle = open_file_count;

	// max files are open
	if(open_file_count == FAT_MAX_OPEN_FILES)
	{
		// there may be files that have been closed, so search for them
		// if the sector handle is uint16_t_MAX, the file has been closed
		bool foundClosedFile = false;
		
		for(uint16_t i = 0; i < FAT_MAX_OPEN_FILES; i++)
		{
			if(isFileClosed(open_files + i))
			{
				file_handle = i;
				foundClosedFile = true;
				break;
			}
		}

		// A closed file was not found
		if(!foundClosedFile)
		{
			printf("ERROR: Max files opened, could not open file %s\n", filename);
			return nullptr;
		}
	}

	FILE * currentFile = (open_files + file_handle);

	currentFile->directory = getFileDirectory(filename);

	if(currentFile->directory.attributes == FAT_INVALID_ENTRY)
	{
		printf("ERROR: Could not find file: %s\n", filename);
		return nullptr;
	}

	currentFile->sector_handle = file_handle;
	currentFile->position = 0;
	currentFile->currentCluster = (currentFile->directory.firstCluster_high << 16) + currentFile->directory.firstCluster_low;
	currentFile->clusterPosition = 0;
	
	if(!loadCluster(currentFile->sector_handle, currentFile->currentCluster))
	{
		return nullptr;
	}

	open_file_count++;

	return currentFile;
}

uint8_t error_code = 0;
char FAT32_getc(FILE * file)
{
	if(isFileClosed(file))
	{
		printf("ERROR: reading from a closed file!\n");
		return null;
	}

	file->position++;
	file->clusterPosition++;

	DirectoryEntry currentDirectory = file->directory;

	// end of file, return null
	if(file->position == currentDirectory.fileSize)
	{
		return null;
	}

	// End of this cluster
	if(file->clusterPosition > bootDrive.bytes_per_cluster)
	{
		uint16_t nextCluster = loadNextCluster(file->sector_handle, file->currentCluster);

		// this was last cluster, i.e end of file
		if(nextCluster == FAT32_CLUSTER_END)
		{
			return null;
		}

		// ERROR in reading next cluster
		if(nextCluster == FAT32_CLUSTER_ERROR)
		{
			printf("ERROR: could not load next cluster for file %S\n", currentDirectory.filename, 11);
			error_code = 1;
			return null;
		}

		// reset cluster position(and increment), and update current cluster
		file->clusterPosition = 1;
		file->currentCluster = nextCluster;
	}

	char read_value = *((char *)getFileCluster(file->sector_handle) + file->clusterPosition - 1);

	return read_value;
}
bool FAT32_read(FILE * file, char* buffer, uint32_t size)
{
	// If we are accesing a closed file, then give an error
	if(isFileClosed(file))
	{
		printf("ERROR: reading from a closed file!\n");
		return false;
	}

	// if the required data is within this cluster and it's size, then simply copy contents
	/*
	if(isValidIncrement(file, size))
	{
		memcpy((char *)getFileCluster(file->sector_handle) + file->clusterPosition, buffer, size);
		return true;
	}
	*/

	for(uint32_t i = 0; i < size; i++)
	{
		*(buffer + i) = FAT32_getc(file);

		// finished reading before buffer ran out
		if(FAT32_isEOF(file))
		{
			return true;
		}

		if(error_code != 0)
		{
			error_code = 0;
			return false;
		}
	}

	return true;
}
bool FAT32_isEOF(FILE* file)
{
	return file->position >= file->directory.fileSize;
}
uint32_t FAT32_fsize(FILE* file)
{
	return file->directory.fileSize;
}
bool FAT32_seek(FILE* file, uint32_t value, uint8_t seek_mode)
{
	if(seek_mode == FAT_SEEK_CURRENT)
	{
		// seek till after EOF
		if((file->position + value) >= file->directory.fileSize)
		{
			printf("Seek Error: Tried to seek beyond EOF\n");
			return false;
		}

		// seek is beyond the loaded cluster
		// load the corresponding cluster
		if((file->clusterPosition + value) >= bootDrive.bytes_per_cluster)
		{
			uint32_t clusterOffset = (file->clusterPosition + value) / bootDrive.bytes_per_cluster;
			uint32_t clusterSeek   = (file->clusterPosition + value) % bootDrive.bytes_per_cluster;

			uint32_t clusterID = file->currentCluster;

			for(uint32_t loadedClusters = 0; loadedClusters < clusterOffset; loadedClusters++)
			{
				clusterID = getNextCluster(clusterID);

				// will only happen if file is corrupted
				if(clusterID == FAT32_CLUSTER_END)
				{
					printf("Seek Error: Corrupted file %S", file->directory.filename, 11);
					return false;
				}

				// failed to read FAT
				if(clusterID == FAT32_CLUSTER_ERROR)
				{
					printf("Seek Error: Unknown Error while seeking in file %S", file->directory.filename, 11);
					return false;
				}
			}

			// load this cluster
			loadCluster(file->sector_handle, clusterID);

			// clusterID contains the cluster in which we seeked
			file->currentCluster = clusterID;
			file->clusterPosition = clusterSeek;
			file->position += value;

			return true;
		}

		// the seek is in this cluster
		file->clusterPosition += value;
		file->position += value;

		return true;
	}
	else if(seek_mode == FAT_SEEK_BEGINNING)
	{
		// seek till after EOF
		if(value >= file->directory.fileSize)
		{
			printf("Seek Error: Tried to seek beyond EOF\n");
			return false;
		}

		// seek is probably in a unloaded cluster
		uint32_t clusterOffset = value / bootDrive.bytes_per_cluster;
		uint32_t clusterSeek   = value % bootDrive.bytes_per_cluster;

		// start from first cluster
		uint32_t clusterID = (file->directory.firstCluster_high << 16) + file->directory.firstCluster_low;

		for(uint32_t loadedClusters = 0; loadedClusters < clusterOffset; loadedClusters++)
		{
			clusterID = getNextCluster(clusterID);

			// will only happen if file is corrupted
			if(clusterID == FAT32_CLUSTER_END)
			{
				printf("Seek Error: Corrupted file %S", file->directory.filename, 11);
				return false;
			}

			// failed to read FAT
			if(clusterID == FAT32_CLUSTER_ERROR)
			{
				printf("Seek Error: Unknown Error while seeking in file %S", file->directory.filename, 11);
				return false;
			}
		}

		// load this cluster if it's not already loaded
		if(file->currentCluster != clusterID) loadCluster(file->sector_handle, clusterID);

		// clusterID contains the cluster in which we seeked
		file->currentCluster = clusterID;
		file->clusterPosition = clusterSeek;
		file->position += value;

		return true;
	}

	// invalid seek mode
	printf("Seek Error: Invalid Seek Mode\n");
	return false;
}
void FAT32_close(FILE * file)
{
	file->sector_handle = UINT16_MAX;
}

void FAT32_table_hexdump()
{
	for(uint16_t i = 0; i < 64; i++)
	{
		if(i % 8 == 0)
		{
			putc('\n');
		}
		printf("%x ", *((char *)FAT32_table + i));
	}
}
