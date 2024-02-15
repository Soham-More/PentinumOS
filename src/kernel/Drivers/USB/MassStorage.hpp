#pragma once

#include <includes.h>
#include <Drivers/USB/usb.hpp>
#include <System/Disk.hpp>

namespace USB
{
    struct msd_device : public usb_device, public sys::Disk
    {
        interface_desc* interface;
        endpoint_desc* bulkIn;
        endpoint_desc* bulkOut;

        uint8_t logicalUnitCount;

        uint64_t blockCount;
        uint64_t blockSize;
        uint64_t size;
        bool use16Base;

        char vendorSCSI[9];
        char productSCSI[17];
        char revisionSCSI[5];

        bool sendBulkOut(void* buffer, size_t size) const;
        bool sendBulkIn(void* buffer, size_t size) const;

        bool init(const usb_device& device);

        bool reset() const;
        bool fullReset() const;

        uint32_t get_sector_size();
        bool read_sectors(uint32_t lba, size_t count, void* buffer);
    };
}
