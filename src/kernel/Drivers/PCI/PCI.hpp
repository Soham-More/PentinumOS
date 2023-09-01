#pragma once

#include <includes.h>

namespace PCI
{
    uint32_t configReadDWORD(uint8_t bus_no, uint8_t device_no, uint8_t function, uint8_t register_offset);
}
