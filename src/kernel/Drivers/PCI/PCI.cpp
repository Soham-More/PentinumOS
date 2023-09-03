#include "PCI.hpp"

#include <std/stdio.h>
#include <i686/x86.h>

namespace PCI
{
    const uint16_t CONFIG_ADDRESS = 0xCF8;
    const uint16_t CONFIG_DATA    = 0xCFC;
/*
    struct Device
    {
        uint16_t vendorID;
        uint16_t deviceID;
        uint16_t command;
        uint16_t status;
        uint8_t  revisionID;
        uint8_t  progIF;
        uint8_t  subClass;
        uint8_t  classCode;
        uint8_t  cacheLineSize;
        uint8_t  latencyTimer;
        uint8_t  headerType;
        uint8_t  BIST;
    } _packed;
*/
    bool DeviceInfo::isValid()
    {
        return vendorID != 0xFFFF;
    }

    uint32_t configReadRegister(uint8_t bus_no, uint8_t device_no, uint8_t function, uint8_t _register)
    {
        return configReadDword(bus_no, device_no, function, _register << 2);
    }

    uint32_t configReadDword(uint8_t bus_no, uint8_t device_no, uint8_t function, uint8_t register_offset)
    {
        uint32_t lbus = bus_no;
        uint32_t ldevice = device_no & 0x1F;
        uint32_t lfunc = function & 0x7;
        uint32_t loffset = register_offset & 0xFC;

        // (1 << 31): enable bit
        uint32_t configAddress = (1 << 31) | (lbus << 16) | (ldevice << 11) | (lfunc << 8) | (loffset);

        // send address
        x86_outl(CONFIG_ADDRESS, configAddress);

        // get register
        uint32_t configData = x86_inl(CONFIG_DATA);
        return configData;
    }

    uint16_t configReadWord(uint8_t bus_no, uint8_t device_no, uint8_t function, uint8_t register_offset)
    {
        uint8_t dword_offset = (register_offset & 0x2) >> 1;

        uint32_t dword = configReadDword(bus_no, device_no, function, register_offset);

        return (dword >> (dword_offset * 16)) & 0xFFFF;
    }

    uint8_t configReadByte(uint8_t bus_no, uint8_t device_no, uint8_t function, uint8_t register_offset)
    {
        uint8_t dword_offset = (register_offset & 0x3);

        uint32_t dword = configReadDword(bus_no, device_no, function, register_offset);

        return (dword >> (dword_offset * 8)) & 0xFF;
    }

    DeviceInfo getDevice(uint8_t bus, uint8_t device)
    {
        DeviceInfo dinfo;

        dinfo.bus = bus;
        dinfo.device_no = device;

        dinfo.vendorID = configReadWord(bus, device, 0, 0x00);

        if(dinfo.vendorID != 0xFFFF)
        {
            dinfo.deviceID = configReadWord(bus, device, 0, 0x01);

            dinfo.revisionID = configReadByte(bus, device, 0, 0x08);
            dinfo.progIF     = configReadByte(bus, device, 0, 0x09);
            dinfo.subClass   = configReadByte(bus, device, 0, 0x0A);
            dinfo.classCode  = configReadByte(bus, device, 0, 0x0B);

            dinfo.headerType = configReadByte(bus, device, 0, 0x0E);
        }

        return dinfo;
    }

    void printDeviceInfoEntry(DeviceInfo& Dinfo)
    {
        printf("\t%3u | %2u     | 0x%2x  | %3u      | %3u     | 0x%4x    | 0x%4x   \n", Dinfo.bus, Dinfo.device_no
                                            , Dinfo.classCode, Dinfo.subClass, Dinfo.progIF, Dinfo.vendorID, Dinfo.deviceID);
    }

    void checkFunction(DeviceInfo& deviceInfo, uint8_t function)
    {
        uint8_t secondaryBus;
        
        if ((deviceInfo.classCode == 0x6) && (deviceInfo.subClass == 0x4))
        {
            secondaryBus = configReadByte(deviceInfo.bus, deviceInfo.device_no, function, 0x19);
            checkBus(secondaryBus);
        }
    }

    void checkDevice(DeviceInfo& deviceInfo)
    {
        if(!deviceInfo.isValid()) return;

        checkFunction(deviceInfo, 0);

        printDeviceInfoEntry(deviceInfo);

        if((deviceInfo.headerType & 0x80) != 0)
        {
            // multi function device
            for(uint8_t func = 1; func < 8; func++)
            {
                if(configReadWord(deviceInfo.bus, deviceInfo.device_no, func, 0) != 0xFFFF)
                {
                    checkFunction(deviceInfo, func);
                }
            }
        }
    }

    void checkBus(uint8_t bus)
    {
        for(uint8_t device = 0; device < 32; device++)
        {
            DeviceInfo Dinfo = getDevice(bus, device);

            if(Dinfo.isValid())
            {
                checkDevice(Dinfo);
            }
        }
    }

    void enumeratePCIBus()
    {
        printf("PCI Buses:\n");
        printf("\tBUS | Device | Class | Subclass | Prog IF | Vendor ID | Device ID\n");

        uint8_t headerType = configReadWord(0, 0, 0, 0x0E);

        if ((headerType & 0x80) == 0)
        {
            // Single PCI host controller
            checkBus(0);
        }
        else
        {
            // Multiple PCI host controllers
            for (uint8_t function = 0; function < 8; function++)
            {
                if (configReadWord(0, 0, function, 0) != 0xFFFF) break;
                checkBus(function);
            }
        }
    }
}
