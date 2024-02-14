#pragma once

#include <Bus/USB/usbdefs.hpp>
#include <std/vector.hpp>

namespace USB
{
    class USBController
    {
        public:
            USBController() = default;

            virtual bool Init() = 0;
            virtual void Setup() = 0;

            virtual bool resetDevice(usb_device& device) = 0;
            virtual const std::vector<usb_device>& getAllDevices() = 0;

            virtual bool controlIn(const usb_device& device, request_packet rpacket, void* buffer, uint16_t size) = 0;
            virtual bool controlOut(const usb_device& device, request_packet rpacket, uint16_t size) = 0;

            virtual bool bulkIn(const usb_device& device, uint8_t endpoint, uint16_t maxPacketSize, void* buffer, uint16_t size) = 0;
            virtual bool bulkOut(const usb_device& device, uint8_t endpoint, uint16_t maxPacketSize, void* buffer, uint16_t size) = 0;

            ~USBController() = default;
    };
}

