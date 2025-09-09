#pragma once

#include <includes.h>

#include "../defs.hpp"
#include "../usb.hpp"

namespace drivers::usb
{
    struct msd_device
    {
        usb_device target;

        interface_desc* interface;
        endpoint_desc* bulkIn;
        endpoint_desc* bulkOut;

        u8 logicalUnitCount;

        uint64_t blockCount;
        uint64_t blockSize;
        uint64_t size;
        bool use16Base;

        char vendorSCSI[9];
        char productSCSI[17];
        char revisionSCSI[5];

        bool sendBulkOut(void* buffer, size_t size) const;
        bool sendBulkIn(void* buffer, size_t size) const;

        static bool detect(const usb_device& device);

        bool init(const usb_device& device);

        bool reset() const;
        bool fullReset() const;

        u32 get_sector_size();
        bool read_sectors(u32 lba, size_t count, void* buffer);

        //disk_driver as_disk_driver();
    };
}

