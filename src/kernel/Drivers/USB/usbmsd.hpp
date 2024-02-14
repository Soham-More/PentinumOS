#pragma once

#include <includes.h>

#include <Drivers/USB/usb.hpp>

namespace USB
{
    struct msd_device
    {
        usb_device device;
        interface_desc* interface;
        endpoint_desc* bulkIn;
        endpoint_desc* bulkOut;

        uint64_t blockCount;
        uint64_t blockSize;
        uint64_t size;
        bool use16Base;

        bool sendBulkOut(void* buffer, size_t size);
        bool sendBulkIn(void* buffer, size_t size);
    };

    msd_device init_msd_device(const usb_device& device);

    bool msd_reset(msd_device device);
    uint8_t msd_getMaxLUN(msd_device device);

    bool msd_read_sectors(msd_device device, uint32_t lba, size_t count, void* buffer);
}
