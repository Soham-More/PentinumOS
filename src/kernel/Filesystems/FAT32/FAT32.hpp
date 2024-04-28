#pragma once

#include <includes.h>
#include <Filesystems/File.hpp>
#include <System/Disk.hpp>
#include <std/stdmem.hpp>
#include <std/utils.hpp>

namespace fs
{
	class FAT32 : public FileSystem
	{
        private:
            struct FAT32_BS
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
                uint32_t        root_cluster;
                uint16_t        sectorFSInfo;
                uint16_t        backupBootSector;
                uint8_t         reserved[12];
                uint8_t         driveID;
                uint8_t         flagsNT;
                uint8_t         signature;
                uint8_t         volumeID[4];
                uint8_t         volumeLable[11];
                uint8_t         systemID[8];
            } _packed;
            struct DirectoryEntry
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
            } _packed;
            struct fileFAT32 : FILE
            {
                uint16_t sector_handle;           // memory allocated to this file
                uint32_t currentCluster;          // current cluster
                uint32_t clusterPosition;         // position wrt cluster begin
                uint32_t position;                // position in file
                DirectoryEntry directory;         // entry data in directory
            };
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

            sys::Disk* disk;
            uint32_t sectorSize;
            FAT32_BS bootSector;
            uint16_t bytes_per_cluster;
            uint32_t fs_data;					// first data sector
            uint32_t fs_fat;					// first sector of FAT
            uint32_t sc_data;					// data sector count
            uint32_t tc_cluster;				// total cluster count
            uint32_t tc_sectors;                // total sector count

            DirectoryEntry invalidEntry;
            DirectoryEntry root;

            void* m_freeSector;
            uint32_t* FAT32_table;
            fileFAT32* open_files;
            void* open_file_clusters;
            void* m_freeCluster;

            uint32_t currentFATSector = 0;
            uint16_t open_file_count = 0;
            uint8_t error_code = 0;

            uint32_t bytesToSectors(size_t bytes);
            uint32_t bytesToPages(size_t bytes);

            void * getFileCluster(uint16_t file_sector_id);
            uint32_t loadNextCluster(uint16_t sector_handle, uint32_t currentCluster);
            uint32_t getNextCluster(uint32_t currentCluster);
            bool loadCluster(uint16_t sector_handle, uint32_t cluster);
            bool loadToFreeCluster(uint32_t cluster);

            // check if file is closed.
            bool isFileClosed(fileFAT32 * file);
            
            // Checks if the increment in position valid
            bool isValidIncrement(fileFAT32 * file, uint32_t inc);

            // converts a string to FAT32 string.
            std::string toStringFAT32(const std::string& dir_name);

            // Searches a directory in a given directory
            // if currentDir is nullptr, it searches in root directory
            bool searchEntry(DirectoryEntry* currentDir, std::string& dir_name, DirectoryEntry* dir);
            DirectoryEntry getFileEntry(const std::string& path);
        public:
            static bool detect(sys::Disk* fsdisk, uint32_t partition_offset);

            bool initialise(sys::Disk* fsdisk, uint32_t partition_offset);
            bool search(const std::string& filename);

            // Open a file
            FILE* open(const std::string& filename);

            // Get a charecter from a file
            char getc(FILE* file);

            // get if current postion is EOF
            bool isEOF(FILE* file);

            // size for buffer larger than file will stop reading before buffer size limit is reached
            bool read(FILE* file, char* buffer, uint32_t size);

            // get file size
            uint32_t size(FILE* file);

            // sekk to position
            bool seek(FILE* file, uint32_t value, uint8_t seek_mode);

            // Close a opened file
            void close(FILE* file);

            void table_hexdump();
	};
}

