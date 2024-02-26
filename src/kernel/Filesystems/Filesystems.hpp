#pragma once

#include <includes.h>
#include <System/Disk.hpp>
#include <Filesystems/File.hpp>

namespace fs
{
    void initialise_filesystem();

    void register_disk(sys::Disk& disk);

    // Search a file
    bool search(const char* filename);

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
}

