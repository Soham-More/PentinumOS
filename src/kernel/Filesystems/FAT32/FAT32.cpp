#include "FAT32.hpp"

#include <std/stdmem.hpp>
#include <std/IO.hpp>

#define FAT32_CLUSTER_END UINT32_MAX
#define FAT32_CLUSTER_ERROR UINT32_MAX - 1

namespace fs
{
	uint32_t FAT32::bytesToSectors(size_t bytes)
	{
		return DivRoundUp(bytes, sectorSize);
	}
	uint32_t FAT32::bytesToPages(size_t bytes)
	{
		return DivRoundUp(bytes, PAGE_SIZE);
	}

	// FAT32 specific private functions
	void* FAT32::getFileCluster(uint16_t file_sector_id)
	{
		if(file_sector_id == MAX_OPEN_FILES)
		{
			return m_freeCluster;
		}

		return (char *)open_file_clusters + file_sector_id * sectorSize * bootSector.sectors_per_cluster;
	}
	uint32_t FAT32::loadNextCluster(uint16_t sector_handle, uint32_t currentCluster)
	{
		uint32_t fat_offset = currentCluster * 4;
		uint32_t fat_sector = fs_fat + (fat_offset / sectorSize);
		uint32_t ent_offset = (fat_offset % sectorSize) / 4;
		
		// read the sector if it is not already loaded
		if(fat_sector != currentFATSector)
		{
			if(!disk->read_sectors(fat_sector, 1, FAT32_table))
			{
				return FAT32_CLUSTER_ERROR;
			}
		}
		
		// ignore the high 4 bits.
		uint32_t table_value = FAT32_table[ent_offset];
		table_value &= 0x0FFFFFFF;

		// no more clusters
		if(table_value >= 0x0FFFFFF8) return FAT32_CLUSTER_END;

		uint32_t cluster_lba = ((table_value - 2) * bootSector.sectors_per_cluster) + fs_data;

		if(!disk->read_sectors(cluster_lba, bootSector.sectors_per_cluster, getFileCluster(sector_handle)))
		{
			return FAT32_CLUSTER_ERROR;
		}

		return table_value;
	}
	uint32_t FAT32::getNextCluster(uint32_t currentCluster)
	{
		uint32_t fat_offset = currentCluster * 4;
		uint32_t fat_sector = fs_fat + (fat_offset / sectorSize);
		uint32_t ent_offset = (fat_offset % sectorSize) / 4;
		
		// read the sector if it is not already loaded
		if(fat_sector != currentFATSector)
		{
			if(!disk->read_sectors(fat_sector, 1, FAT32_table))
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
	bool FAT32::loadCluster(uint16_t sector_handle, uint32_t cluster)
	{
		uint32_t cluster_lba = ((cluster - 2) * bootSector.sectors_per_cluster) + fs_data;

		return disk->read_sectors(cluster_lba, bootSector.sectors_per_cluster, getFileCluster(sector_handle));
	}
	bool FAT32::loadToFreeCluster(uint32_t cluster)
	{
		uint32_t cluster_lba = ((cluster - 2) * bootSector.sectors_per_cluster) + fs_data;

		return disk->read_sectors(cluster_lba, bootSector.sectors_per_cluster, m_freeCluster);
	}
	
	bool FAT32::isFileClosed(fileFAT32 * file)
	{
		return file->sector_handle == UINT16_MAX;
	}

	// Checks if the increment in position valid
	bool FAT32::isValidIncrement(fileFAT32 * file, uint32_t inc)
	{
		return (file->clusterPosition + inc) < bytes_per_cluster && (file->position + inc) < file->directory.fileSize;
	}

	std::string FAT32::toStringFAT32(const std::string& dir_name)
	{
		std::vector<std::string> filenames;
		dir_name.split('.', filenames);

		if(filenames.size() == 2)
		{
			std::string newFilename = filenames[0];
			newFilename.to_upper();
			newFilename.ljust(' ', 8);
			
			filenames[1].to_upper();
			newFilename += filenames[1];

			return newFilename;
		}

		filenames[0].to_upper();
		return filenames[0];
	}

	// Searches a directory in a given directory
	// if currentDir is nullptr, it searches in root directory
	bool FAT32::searchEntry(DirectoryEntry* currentDir, std::string& dir_name, DirectoryEntry* dir)
	{
		if(currentDir->attributes != FAT_DIRECTORY) return false;

		uint16_t currentCluster = (currentDir->firstCluster_high << 16) + currentDir->firstCluster_low;

		while(currentCluster != FAT32_CLUSTER_END)
		{
			// load into m_freeCluster
			if(!loadToFreeCluster(currentCluster))
			{
				// failed to load cluster
				log_error("[FileSystem][FAT32] Failed to read cluster while parsing directory:\n\t %S\n", currentDir->filename, 11);
				return false;
			}

			DirectoryEntry* directories = (DirectoryEntry*)m_freeCluster;

			for(uint16_t i = 0; i * sizeof(DirectoryEntry) < bytes_per_cluster; i++)
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
				if(strcmp(dir_name.c_str(), (char*)directories[i].filename, 11))
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
	FAT32::DirectoryEntry FAT32::getFileEntry(const std::string& path)
	{
		std::vector<std::string> directoryStructure;
		path.split('/', directoryStructure);

		DirectoryEntry A = root; // start searching from root
		DirectoryEntry B;

		DirectoryEntry* currentEntry = &A;
		DirectoryEntry* nextEntry = &B;

		for(uint16_t i = 0; i < directoryStructure.size(); i++)
		{
			std::string& dir_name = directoryStructure[i];
			dir_name = toStringFAT32(dir_name);

			if(!searchEntry(currentEntry, dir_name, nextEntry))
			{
				return invalidEntry;
			}

			// swap
			DirectoryEntry* tmp = currentEntry;
			currentEntry = nextEntry;
			nextEntry = tmp;
		}

		return *currentEntry;
	}

	bool FAT32::detect(sys::Disk* fsdisk, uint32_t partition_offset)
	{
		void* m_freeSector = alloc_pages(1);
		FAT32_BS bootSector;

		// load bootsector on active partition
		if(!fsdisk->read_sectors(partition_offset, 1, m_freeSector))
		{
			return false;
		}

		memcpy(m_freeSector, &bootSector, sizeof(FAT32_BS));

		if(bootSector.signature == 0x28 || bootSector.signature == 0x29) return true;

		return false;
	}

	// FAT API
	bool FAT32::initialise(sys::Disk* fsdisk, uint32_t partition_offset)
	{
		disk = fsdisk;

		m_freeSector = alloc_pages(1);

		// load bootsector on active partition
		if(!disk->read_sectors(partition_offset, 1, m_freeSector))
		{
			return false;
		}

		memcpy(m_freeSector, &bootSector, sizeof(FAT32_BS));

		fs_data = partition_offset + bootSector.reserved_sector_count + (bootSector.table_count * bootSector.sectors_per_fat);
		fs_fat = partition_offset + bootSector.reserved_sector_count;
		sc_data = bootSector.total_sectors_32 - (bootSector.reserved_sector_count + (bootSector.table_count * bootSector.sectors_per_fat));
		tc_cluster = sc_data / bootSector.sectors_per_cluster;
		bytes_per_cluster = bootSector.bytes_per_sector * bootSector.sectors_per_cluster;

		invalidEntry.attributes = FAT_INVALID_ENTRY;

		// get root directory first cluster
		root.firstCluster_low = bootSector.root_cluster % (1 << 16);
		root.firstCluster_high = bootSector.root_cluster >> 16;

		root.attributes = FAT_DIRECTORY;

		// allocate 1 page
		FAT32_table = (uint32_t *)alloc_pages(1);

		currentFATSector = fs_fat;

		if(!disk->read_sectors(fs_fat, 1, (void *)FAT32_table))
		{
			log_error("[FS][FAT32] Failed to load root FAT32 tables\n");
			return false;
		}

		open_files = (fileFAT32*)alloc_pages(bytesToPages(MAX_OPEN_FILES * sizeof(fileFAT32)));

		// NOTE: m_freeCluster must be allocated right after open_file_clusters alloc
		size_t bytes_per_cluster = bootSector.sectors_per_cluster * bootSector.bytes_per_sector;

		open_file_clusters = alloc_pages(bytesToPages(MAX_OPEN_FILES * bytes_per_cluster));
		m_freeCluster = alloc_pages(bytesToPages(bytes_per_cluster));

		return true;
	}
	bool FAT32::search(const std::string& filepath)
	{
		return getFileEntry(filepath).attributes != FAT_INVALID_ENTRY;
	}

	FILE* FAT32::open(const std::string& filename)
	{
		uint16_t file_handle = open_file_count;

		// max files are open
		if(open_file_count == MAX_OPEN_FILES)
		{
			// there may be files that have been closed, so search for them
			// if the sector handle is uint16_t_MAX, the file has been closed
			bool foundClosedFile = false;
			
			for(uint16_t i = 0; i < MAX_OPEN_FILES; i++)
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
				log_error("[FS][FAT32] Max files opened, could not open file %s\n", filename);
				return nullptr;
			}
		}

		fileFAT32* currentFile = (open_files + file_handle);

		currentFile->directory = getFileEntry(filename);

		if(currentFile->directory.attributes == FAT_INVALID_ENTRY)
		{
			log_error("[FS][FAT32] Could not find file: %s\n", filename.c_str());
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

	char FAT32::getc(FILE* _file)
	{
		fileFAT32* file = reinterpret_cast<fileFAT32*>(_file);

		if(isFileClosed(file))
		{
			log_error("[FS][FAT32] Reading from a closed file!\n");
			return null;
		}

		file->position++;
		file->clusterPosition++;

		DirectoryEntry currentDirectory = file->directory;

		// end of file, return null
		if(file->position > currentDirectory.fileSize)
		{
			return null;
		}

		// End of this cluster
		if(file->clusterPosition > bytes_per_cluster)
		{
			//printf("\nLoading next cluster\n");
			uint16_t nextCluster = loadNextCluster(file->sector_handle, file->currentCluster);

			// this was last cluster, i.e end of file
			if(nextCluster == FAT32_CLUSTER_END)
			{
				return null;
			}

			// ERROR in reading next cluster
			if(nextCluster == FAT32_CLUSTER_ERROR)
			{
				log_error("[FS][FAT32] Could not load next cluster for file %S\n", currentDirectory.filename, 11);
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
	bool FAT32::read(FILE * _file, char* buffer, uint32_t size)
	{
		fileFAT32* file = reinterpret_cast<fileFAT32*>(_file);

		// If we are accesing a closed file, then give an error
		if(isFileClosed(file))
		{
			log_error("[FS][FAT32] Reading from a closed file!\n");
			return false;
		}

		for(uint32_t i = 0; i < size; i++)
		{
			*(buffer + i) = getc(file);

			// finished reading before buffer ran out
			if(isEOF(file))
			{
				//printf("\nEOF 0x%x\n", i);
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
	bool FAT32::isEOF(FILE* _file)
	{
		fileFAT32* file = reinterpret_cast<fileFAT32*>(_file);
		return file->position > file->directory.fileSize;
	}
	uint32_t FAT32::size(FILE* _file)
	{
		fileFAT32* file = reinterpret_cast<fileFAT32*>(_file);
		return file->directory.fileSize;
	}
	bool FAT32::seek(FILE* _file, uint32_t value, uint8_t seek_mode)
	{
		fileFAT32* file = reinterpret_cast<fileFAT32*>(_file);

		if(seek_mode == FAT_SEEK_CURRENT)
		{
			// seek till after EOF
			if((file->position + value) >= file->directory.fileSize)
			{
				log_warn("[FS][FAT32] Tried to seek beyond EOF\n");
				return false;
			}

			// seek is beyond the loaded cluster
			// load the corresponding cluster
			if((file->clusterPosition + value) >= bytes_per_cluster)
			{
				uint32_t clusterOffset = (file->clusterPosition + value) / bytes_per_cluster;
				uint32_t clusterSeek   = (file->clusterPosition + value) % bytes_per_cluster;

				uint32_t clusterID = file->currentCluster;

				for(uint32_t loadedClusters = 0; loadedClusters < clusterOffset; loadedClusters++)
				{
					clusterID = getNextCluster(clusterID);

					// will only happen if file is corrupted
					if(clusterID == FAT32_CLUSTER_END)
					{
						log_error("[FS][FAT32] Corrupted file %S", file->directory.filename, 11);
						return false;
					}

					// failed to read FAT
					if(clusterID == FAT32_CLUSTER_ERROR)
					{
						printf("[FS][FAT32] Unknown Error while seeking in file %S", file->directory.filename, 11);
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
				log_warn("[FS][FAT32] Tried to seek beyond EOF\n");
				return false;
			}

			// seek is probably in a unloaded cluster
			uint32_t clusterOffset = value / bytes_per_cluster;
			uint32_t clusterSeek   = value % bytes_per_cluster;

			// start from first cluster
			uint32_t clusterID = (file->directory.firstCluster_high << 16) + file->directory.firstCluster_low;

			for(uint32_t loadedClusters = 0; loadedClusters < clusterOffset; loadedClusters++)
			{
				clusterID = getNextCluster(clusterID);

				// will only happen if file is corrupted
				if(clusterID == FAT32_CLUSTER_END)
				{
					log_error("[FS][FAT32] Corrupted file %S", file->directory.filename, 11);
					return false;
				}

				// failed to read FAT
				if(clusterID == FAT32_CLUSTER_ERROR)
				{
					log_error("[FS][FAT32] Unknown Error while seeking in file %S", file->directory.filename, 11);
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
		log_warn("[FS][FAT32] Invalid Seek Mode!\n");
		return false;
	}
	void FAT32::close(FILE * _file)
	{
		fileFAT32* file = reinterpret_cast<fileFAT32*>(_file);
		file->sector_handle = UINT16_MAX;
	}

	void FAT32::table_hexdump()
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
}