#include "PCI.hpp"

#include <i686/x86.h>

namespace PCI
{
    const uint16_t CONFIG_ADDRESS = 0xCF8;
    const uint16_t CONFIG_DATA    = 0xCFC;

    uint32_t configReadDWORD(uint8_t bus_no, uint8_t device_no, uint8_t function, uint8_t register_offset)
    {
        uint32_t lbus = bus_no;
        uint32_t ldevice = device_no & 0x1F;
        uint32_t lfunc = function & 0x7;
        uint32_t loffset = register_offset & 0xFC;

        // (1 << 31): enable bit
        uint32_t configAddress = (1 << 31) | (lbus << 16) | ((ldevice & 0xF) << 11) | (lfunc << 8) | (loffset);

        // send address
        x86_outl(CONFIG_ADDRESS, configAddress);

        // get register
        uint32_t configData = x86_inl(CONFIG_DATA);
        return configData;
    }
}
