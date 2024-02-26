#pragma once

#include <includes.h>
#include <std/utils.hpp>

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
            virtual bool initialise(sys::Disk* fsdisk, uint32_t partition_offset) = 0;
            virtual bool search(const char* filename) = 0;

            // Open a file
            virtual FILE* open(const std::string& filename) = 0;

            // Get a charecter from a file
            virtual char getc(FILE* file) = 0;

            // get if current postion is EOF
            virtual bool isEOF(FILE* file) = 0;

            // size for buffer larger than file will stop reading before buffer size limit is reached
            virtual bool read(FILE* file, char* buffer, uint32_t size) = 0;

            // get file size
            virtual uint32_t size(FILE* file) = 0;

            // sekk to position
            virtual bool seek(FILE* file, uint32_t value, uint8_t seek_mode) = 0;

            // Close a opened file
            virtual void close(FILE* file) = 0;
    };
}

