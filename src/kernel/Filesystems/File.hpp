#pragma once

#include <includes.h>
#include <std/utils.hpp>
#include <System/Disk.hpp>

#define MAX_OPEN_FILES 0x200

namespace fs
{
    enum SEEK_MODE
    {
        FAT_SEEK_BEGINNING = 0x00,
        FAT_SEEK_CURRENT   = 0x01,
    };

    struct FILE
    {
        size_t deviceID;
        size_t partitionID;
        size_t filessystemID;
    };

    class FileSystem
    {
        public:
            FileSystem(){;}
            bool initialise(sys::Disk* fsdisk, uint32_t partition_offset){;}
            bool search(const std::string& filename){;}

            // Open a file
            FILE* open(const std::string& filename){;};

            // Get a charecter from a file
            char getc(FILE* file){;}

            // get if current postion is EOF
            bool isEOF(FILE* file){;}

            // size for buffer larger than file will stop reading before buffer size limit is reached
            bool read(FILE* file, char* buffer, uint32_t size){;}

            // get file size
            uint32_t size(FILE* file){;}

            // sekk to position
            bool seek(FILE* file, uint32_t value, uint8_t seek_mode){;}

            // Close a opened file
            void close(FILE* file){;}
            ~FileSystem(){;}
    };
}

