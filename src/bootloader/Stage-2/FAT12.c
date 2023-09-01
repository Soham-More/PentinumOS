#include "FAT12.h"
#include "IO.h"
#include "memdefs.h"
#include "Memory.h"
#include "String.h"
#include "Math.h"

#define FAT12_CLUSTER_END UINT16_MAX
#define FAT12_CLUSTER_ERROR UINT16_MAX - 1

typedef struct FAT12_BS
{
	uint8_t 		bootjmp[3];
	uint8_t 		oem_name[8];
	uint16_t 	    bytes_per_sector;
	uint8_t  		sectors_per_cluster;
	uint16_t		reserved_sector_count;
	uint8_t	 		table_count;
	uint16_t		root_entry_count;
	uint16_t		total_sectors_16;
	uint8_t			media_type;
	uint16_t		table_size_16;
	uint16_t		sectors_per_track;
	uint16_t		head_side_count;
	uint32_t 		hidden_sector_count;
	uint32_t 		total_sectors_32;

	//extended fat12 and fat16 stuff
	uint8_t		bios_drive_num;
	uint8_t		reserved1;
	uint8_t		boot_signature;
	uint32_t	volume_id;
	uint8_t		volume_label[11];
	uint8_t		fat_type_label[8];
} _packed FAT12_BS;

typedef struct FAT12
{
    DISK disk;
    FAT12_BS bootSector;
	uint16_t root_dir_size;
	uint16_t bytes_per_cluster;
	uint16_t fs_data;					// first data sector
	uint16_t fs_fat;					// first sector of FAT
	uint16_t fs_root_dir; 			// first sector of root dir
	uint16_t sc_data;					// data sector count
	uint16_t tc_cluster;				// total cluster count
} FAT12;

FAT12 bootDrive12;
DirectoryEntry invalidEntry12;

static void* m_freeSector12 = nullptr;
static DirectoryEntry* root_directories = nullptr;
static uint16_t* FAT12_table = nullptr;
static FILE * open_files12 = nullptr;
static void * open_file_clusters12 = nullptr;
static void * m_freeCluster12 = nullptr;

// FAT12 specific private functions

void * getFileCluster12(uint16_t file_sector_id)
{
	return (char *)open_file_clusters12 + file_sector_id * SECTOR_SIZE * bootDrive12.bootSector.sectors_per_cluster;
}
uint16_t loadNextCluster12(uint16_t sector_handle, uint16_t currentCluster)
{
	uint16_t fat_offset = currentCluster + (currentCluster / 2);// multiply by 1.5
	uint16_t fat_sector = (fat_offset / bootDrive12.bootSector.bytes_per_sector) * bootDrive12.bootSector.bytes_per_sector;
	uint16_t ent_offset = fat_offset % bootDrive12.bootSector.bytes_per_sector;
	
	uint16_t table_value = *(uint16_t *)((char *)FAT12_table + fat_sector + ent_offset);
	
	if(currentCluster & 0x0001)
	table_value = table_value >> 4;
	else
	table_value = table_value & 0x0FFF;

	// no more clusters
	if(table_value >= 0xFF8)
	{
		return FAT12_CLUSTER_END;
	}

	uint16_t cluster_lba = ((table_value - 2) * bootDrive12.bootSector.sectors_per_cluster) + bootDrive12.fs_data;

	if(!disk_readSectors(bootDrive12.disk, cluster_lba, bootDrive12.bootSector.sectors_per_cluster, getFileCluster12(sector_handle)))
	{
		return FAT12_CLUSTER_ERROR;
	}

	return table_value;
}
uint16_t getNextCluster12(uint16_t currentCluster)
{
	uint16_t fat_offset = currentCluster + (currentCluster / 2);// multiply by 1.5
	uint16_t fat_sector = (fat_offset / bootDrive12.bootSector.bytes_per_sector) * bootDrive12.bootSector.bytes_per_sector;
	uint16_t ent_offset = fat_offset % bootDrive12.bootSector.bytes_per_sector;
	
	uint16_t table_value = *(uint16_t *)((char *)FAT12_table + fat_sector + ent_offset);

	if(currentCluster & 0x0001) table_value = table_value >> 4;
	else 						table_value = table_value & 0x0FFF;

	// no more clusters
	if(table_value >= 0xFF8) return FAT12_CLUSTER_END;

	return table_value;
}
bool loadCluster12(uint16_t sector_handle, uint16_t cluster)
{
	uint16_t cluster_lba = ((cluster - 2) * bootDrive12.bootSector.sectors_per_cluster) + bootDrive12.fs_data;

	return disk_readSectors(bootDrive12.disk, cluster_lba, bootDrive12.bootSector.sectors_per_cluster, getFileCluster12(sector_handle));
}
bool isFileClosed12(FILE * file)
{
	return file->sector_handle == UINT16_MAX;
}

// Checks if the increment in position valid
bool isValidIncrement12(FILE * file, uint32_t inc)
{
	return (file->clusterPosition + inc) < bootDrive12.bytes_per_cluster && (file->position + inc) < file->directory.fileSize;
}

void add_padding_dir_name12(char * dir_name)
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
bool search_directory12(DirectoryEntry* currentDir, char * dir_name, DirectoryEntry* dir)
{
	if(currentDir == nullptr)
	{
		DirectoryEntry * directories = root_directories;

		for(uint16_t i = 0; i * sizeof(DirectoryEntry) < bootDrive12.bytes_per_cluster; i++)
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

		// Entry not found
		return false;
	}

	if(currentDir->attributes != FAT_DIRECTORY)
	{
		return false;
	}

	uint16_t currentCluster = currentDir->firstCluster_low;

	while(currentCluster != FAT12_CLUSTER_END)
	{
		// load into m_freeCluster12
		if(!loadCluster12(FAT_MAX_OPEN_FILES, currentCluster))
		{
			// failed to load cluster
			printf("ERROR: Failed to read cluster while parsing directory %S\n", currentDir->filename, 11);
			return false;
		}

		DirectoryEntry* directories = m_freeCluster12;

		for(uint16_t i = 0; i * sizeof(DirectoryEntry) < bootDrive12.bytes_per_cluster; i++)
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

			//printf("Seeing Directory: %S\n", directories[i].filename, 11);

			// if found, return true
			if(strcmp(dir_name, directories[i].filename, 11))
			{
				*dir = directories[i];
				return true;
			}
		}

		// if not found yet, check for another cluster
		currentCluster = getNextCluster12(currentCluster);
	}

	// Filename not founud
	return false;
}
DirectoryEntry search_file_wsplit_path12(SplitString* path_to_file)
{
	DirectoryEntry A;
	DirectoryEntry B;

	DirectoryEntry* currentEntry = &A;
	DirectoryEntry* nextEntry = &B;

	char * dir_name = getSplitString(path_to_file, 0);
	add_padding_dir_name12(dir_name);

	if(!search_directory12(nullptr, dir_name, currentEntry))
	{
		return invalidEntry12;
	}

	for(uint16_t i = 1; i < path_to_file->split_count; i++)
	{
		dir_name = getSplitString(path_to_file, i);
		add_padding_dir_name12(dir_name);

		if(!search_directory12(currentEntry, dir_name, nextEntry))
		{
			return invalidEntry12;
		}

		swap(currentEntry, nextEntry, DirectoryEntry*);
	}

	return *currentEntry;
}
DirectoryEntry getFileDirectory12(const char* path)
{
	SplitString splitPath;

	splitPath.split_count = count_char_occurrence(path, '/') + 1;
	splitPath.split_string_size = 11;
	splitPath.string_array = alloc(bytesToSectors(splitPath.split_count * 11));

	split_wrt_char(path, '/', &splitPath);

	DirectoryEntry fileDirectory = search_file_wsplit_path12(&splitPath);

	popAllocMem(splitPath.split_count * 11);

	return fileDirectory;
}

// FAT API

bool FAT12_initialise(DISK bootdisk, uint32_t partition_offset)
{
	bootDrive12.disk = bootdisk;

	// load bootsector on active partition
	if(!disk_readSectors(bootDrive12.disk, partition_offset, 1, m_freeSector12))
	{
		return false;
	}

	memcpy(m_freeSector12, &bootDrive12.bootSector, sizeof(FAT12_BS));

	bootDrive12.root_dir_size = ((bootDrive12.bootSector.root_entry_count * 32) + (bootDrive12.bootSector.bytes_per_sector - 1)) / bootDrive12.bootSector.bytes_per_sector;
	bootDrive12.fs_data = partition_offset + bootDrive12.bootSector.reserved_sector_count + (bootDrive12.bootSector.table_count * bootDrive12.bootSector.table_size_16) + bootDrive12.root_dir_size;
	bootDrive12.fs_fat = partition_offset + bootDrive12.bootSector.reserved_sector_count;
	bootDrive12.sc_data = bootDrive12.bootSector.total_sectors_16 - (bootDrive12.bootSector.reserved_sector_count + (bootDrive12.bootSector.table_count * bootDrive12.bootSector.table_size_16) + bootDrive12.root_dir_size);
	bootDrive12.tc_cluster = bootDrive12.sc_data / bootDrive12.bootSector.sectors_per_cluster;
	bootDrive12.fs_root_dir = partition_offset + bootDrive12.fs_data - bootDrive12.root_dir_size;
	bootDrive12.bytes_per_cluster = bootDrive12.bootSector.bytes_per_sector * bootDrive12.bootSector.sectors_per_cluster;

	invalidEntry12.attributes = FAT_INVALID_ENTRY;

	root_directories = (DirectoryEntry *)alloc(bootDrive12.root_dir_size);
	FAT12_table = (uint16_t *)alloc(bootDrive12.bootSector.table_count * bootDrive12.bootSector.table_size_16);

	if(!disk_readSectors(bootDrive12.disk, bootDrive12.fs_fat, bootDrive12.bootSector.table_count * bootDrive12.bootSector.table_size_16, (void *)FAT12_table))
	{
		printf("FAT Fail: Failed to load root FAT tables\n");
		return false;
	}

	if(!disk_readSectors(bootDrive12.disk, bootDrive12.fs_root_dir, bootDrive12.root_dir_size, (void *)root_directories))
	{
		printf("FAT Fail: Failed to load root directories\n");
		return false;
	}

	open_files12 = alloc((FAT_MAX_OPEN_FILES * sizeof(FILE) + SECTOR_SIZE - 1) / SECTOR_SIZE);

	// NOTE: m_freeCluster12 must be allocated right after open_file_clusters12 alloc
	open_file_clusters12 = alloc(FAT_MAX_OPEN_FILES * bootDrive12.bootSector.sectors_per_cluster);
	m_freeCluster12 = alloc(bootDrive12.bootSector.sectors_per_cluster);

	return true;
}
bool FAT12_search(const char* filepath)
{
	return getFileDirectory12(filepath).attributes != FAT_INVALID_ENTRY;
}

void FAT12_pretty_print_path(const char* filepath)
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

static uint16_t open_file_count12 = 0;
FILE * FAT12_open(const char* filename)
{
	uint16_t file_handle = open_file_count12;

	// max files are open
	if(open_file_count12 == FAT_MAX_OPEN_FILES)
	{
		// there may be files that have been closed, so search for them
		// if the sector handle is uint16_t_MAX, the file has been closed
		bool foundClosedFile = false;
		
		for(uint16_t i = 0; i < FAT_MAX_OPEN_FILES; i++)
		{
			if(isFileClosed12(open_files12 + i))
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

	FILE * currentFile = (open_files12 + file_handle);

	currentFile->directory = getFileDirectory12(filename);

	if(currentFile->directory.attributes == FAT_INVALID_ENTRY)
	{
		printf("ERROR: Could not find file: %s\n", filename);
		return nullptr;
	}

	currentFile->sector_handle = file_handle;
	currentFile->position = 0;
	currentFile->currentCluster = currentFile->directory.firstCluster_low;
	currentFile->clusterPosition = 0;
	
	if(!loadCluster12(currentFile->sector_handle, currentFile->currentCluster))
	{
		return nullptr;
	}

	open_file_count12++;

	return currentFile;
}

uint8_t error_code12 = 0;
char FAT12_getc(FILE * file)
{
	if(isFileClosed12(file))
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
	if(file->clusterPosition > bootDrive12.bytes_per_cluster)
	{
		uint16_t nextCluster = loadNextCluster12(file->sector_handle, file->currentCluster);

		// this was last cluster, i.e end of file
		if(nextCluster == FAT12_CLUSTER_END)
		{
			return null;
		}

		// ERROR in reading next cluster
		if(nextCluster == FAT12_CLUSTER_ERROR)
		{
			printf("ERROR: could not load next cluster for file %S\n", currentDirectory.filename, 11);
			error_code12 = 1;
			return null;
		}

		// reset cluster position(and increment), and update current cluster
		file->clusterPosition = 1;
		file->currentCluster = nextCluster;
	}

	char read_value = *((char *)getFileCluster12(file->sector_handle) + file->clusterPosition - 1);

	return read_value;
}
bool FAT12_read(FILE * file, char* buffer, uint32_t size)
{
	// If we are accesing a closed file, then give an error
	if(isFileClosed12(file))
	{
		printf("ERROR: reading from a closed file!\n");
		return false;
	}

	// if the required data is within this cluster and it's size, then simply copy contents
	if(isValidIncrement12(file, size))
	{
		memcpy((char *)getFileCluster12(file->sector_handle) + file->clusterPosition, buffer, size);
	}

	for(uint32_t i = 0; i < size; i++)
	{
		*(buffer + i) = FAT12_getc(file);

		// finished reading before buffer ran out
		if(FAT12_isEOF(file))
		{
			return true;
		}

		if(error_code12 != 0)
		{
			error_code12 = 0;
			return false;
		}
	}

	return true;
}
bool FAT12_isEOF(FILE* file)
{
	return file->position >= file->directory.fileSize;
}
uint32_t FAT12_fsize(FILE* file)
{
	return file->directory.fileSize;
}

bool FAT12_seek(FILE* file, uint32_t value, uint8_t seek_mode)
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
		if((file->clusterPosition + value) >= bootDrive12.bytes_per_cluster)
		{
			uint32_t clusterOffset = (file->clusterPosition + value) / bootDrive12.bytes_per_cluster;
			uint32_t clusterSeek   = (file->clusterPosition + value) % bootDrive12.bytes_per_cluster;

			uint32_t clusterID = file->currentCluster;

			for(uint32_t loadedClusters = 0; loadedClusters < clusterOffset; loadedClusters++)
			{
				clusterID = getNextCluster12(clusterID);

				// will only happen if file is corrupted
				if(clusterID == FAT12_CLUSTER_END)
				{
					printf("Seek Error: Corrupted file %S", file->directory.filename, 11);
					return false;
				}

				// failed to read FAT
				if(clusterID == FAT12_CLUSTER_ERROR)
				{
					printf("Seek Error: Unknown Error while seeking in file %S", file->directory.filename, 11);
					return false;
				}
			}

			// load this cluster
			loadCluster12(file->sector_handle, clusterID);

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
		uint32_t clusterOffset = value / bootDrive12.bytes_per_cluster;
		uint32_t clusterSeek   = value % bootDrive12.bytes_per_cluster;

		// start from first cluster
		uint32_t clusterID = (file->directory.firstCluster_high << 16) + file->directory.firstCluster_low;

		for(uint32_t loadedClusters = 0; loadedClusters < clusterOffset; loadedClusters++)
		{
			clusterID = getNextCluster12(clusterID);

			// will only happen if file is corrupted
			if(clusterID == FAT12_CLUSTER_END)
			{
				printf("Seek Error: Corrupted file %S", file->directory.filename, 11);
				return false;
			}

			// failed to read FAT
			if(clusterID == FAT12_CLUSTER_ERROR)
			{
				printf("Seek Error: Unknown Error while seeking in file %S", file->directory.filename, 11);
				return false;
			}
		}

		// load this cluster if it's not already loaded
		if(file->currentCluster != clusterID) loadCluster12(file->sector_handle, clusterID);

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

void FAT12_close(FILE * file)
{
	file->sector_handle = UINT16_MAX;
}

void FAT12_table_hexdump()
{
	for(uint16_t i = 0; i < 64; i++)
	{
		if(i % 8 == 0)
		{
			putc('\n');
		}
		printf("%x ", *((char *)FAT12_table + i));
	}
}
