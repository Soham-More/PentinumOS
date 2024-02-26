#include "Filesystems.hpp"

#include <std/IO.hpp>
#include <std/utils.hpp>

#include <Filesystems/FAT32/FAT32.hpp>

namespace fs
{
    static std::vector<fs::FileSystem> filesystems;
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
    }

    void initialise_filesystem()
    {
        // add filesystems
        filesystems.push_back(FAT32{});
    }

    void register_disk(sys::Disk& disk)
    {
        disks.push_back(&disk);
    }
}
