#include "Filesystems.hpp"

#include <std/IO.hpp>
#include <std/utils.hpp>

#include <Filesystems/FAT32/FAT32.hpp>

namespace fs
{
    static std::vector<fs::FileSystem*> filesystems;
    static std::vector<sys::Disk*> disks;

    struct FilePathInfo
    {
        size_t diskID;
        size_t partitionID;
        std::string filepath;
    };

    FilePathInfo getFilePathInfo(const std::string& filepath)
    {
        FilePathInfo filePathInfo;

        std::vector<std::string> path;

        filepath.split('/', path);

        if(path[0].size() != 5) return FilePathInfo{0, 0, ""};

        // check if it starts with a valid device name
        if(path[0][0] != 's' || path[0][1] != 'd' || path[0][3] != 'p') return FilePathInfo{0, 0, ""};

        // get disk ID
        filePathInfo.diskID = path[0][2] - 'a';
        filePathInfo.partitionID = path[0][4] - '0';

        filePathInfo.filepath = filepath.filter(6);

        return filePathInfo;
    }

    void detectFS(sys::Disk& disk, size_t partitionID)
    {
        if(FAT32::detect(&disk, disk.disk_partitions[partitionID].lba))
        {
            FAT32* newFAT32 = (FAT32*)std::malloc(sizeof(FAT32));

            *newFAT32 = FAT32{};

            filesystems.push_back(newFAT32);

            disk.disk_partitions[partitionID].fsID = filesystems.size() - 1;

            newFAT32->initialise(&disk, disk.disk_partitions[partitionID].lba);
        }
        else
        {
            return;
        }

        

        //filesystems.back()->initialise(&disk, disk.disk_partitions[partitionID].lba);
    }

    void register_disk(sys::Disk& disk)
    {
        disks.push_back(&disk);

        for(size_t i = 0; i < disk.partition_count; i++)
        {
            detectFS(disk, i);
        }
    }

    // Search a file
    bool search(const std::string& filename)
    {
        FilePathInfo info = getFilePathInfo(filename);

        if(info.filepath.empty()) return false;

        uint8_t fsID = disks[info.diskID]->disk_partitions[info.partitionID].fsID;

        return filesystems[fsID]->search(info.filepath);
    }

    // Open a file
    FILE* open(const std::string& filename)
    {
        FilePathInfo info = getFilePathInfo(filename);

        if(info.filepath.empty()) return nullptr;

        uint8_t fsID = disks[info.diskID]->disk_partitions[info.partitionID].fsID;

        FILE* file = filesystems[fsID]->open(info.filepath);

        file->filessystemID = fsID;
        file->deviceID = info.diskID;
        file->partitionID = info.partitionID;

        return file;
    }

    // Get a charecter from a file
    char getc(FILE* file)
    {
        if(file == nullptr) return null;
        return filesystems[file->filessystemID]->getc(file);
    }

    // get if current postion is EOF
    bool isEOF(FILE* file)
    {
        if(file == nullptr) return true;
        return filesystems[file->filessystemID]->isEOF(file);
    }

    // size for buffer larger than file will stop reading before buffer size limit is reached
    bool read(FILE* file, char* buffer, uint32_t size)
    {
        if(file == nullptr) return false;
        return filesystems[file->filessystemID]->read(file, buffer, size);
    }

    // get file size
    uint32_t size(FILE* file)
    {
        if(file == nullptr) return 0;
        return filesystems[file->filessystemID]->size(file);
    }

    // sekk to position
    bool seek(FILE* file, uint32_t value, uint8_t seek_mode)
    {
        if(file == nullptr) return false;
        return filesystems[file->filessystemID]->seek(file, value, seek_mode);
    }

    // Close a opened file
    void close(FILE* file)
    {
        if(file == nullptr) return;
        return filesystems[file->filessystemID]->close(file);
    }
}
