#include "PCI.hpp"

#include <std/stdio.h>
#include <i686/x86.h>

namespace PCI
{
    const uint16_t CONFIG_ADDRESS = 0xCF8;
    const uint16_t CONFIG_DATA    = 0xCFC;

    bool DeviceInfo::isValid()
    {
        return functions[0].vendorID != 0xFFFF;
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

    FunctionInfo getDeviceFunction(uint8_t bus, uint8_t device, uint8_t function)
    {
        FunctionInfo finfo;

        finfo.function = function;

        finfo.vendorID = configReadWord(bus, device, function, 0x00);

        if(finfo.vendorID != 0xFFFF)
        {
            finfo.deviceID = configReadWord(bus, device, function, 0x01);

            finfo.revisionID = configReadByte(bus, device, function, 0x08);
            finfo.progIF     = configReadByte(bus, device, function, 0x09);
            finfo.subClass   = configReadByte(bus, device, function, 0x0A);
            finfo.classCode  = configReadByte(bus, device, function, 0x0B);

            finfo.headerType = configReadByte(bus, device, function, 0x0E);
        }

        return finfo;
    }

    DeviceInfo getDevice(uint8_t bus, uint8_t device)
    {
        DeviceInfo dinfo;

        dinfo.bus = bus;
        dinfo.device_no = device;

        dinfo.functions[0] = getDeviceFunction(bus, device, 0);

        if((dinfo.functions[0].headerType & 0x80) > 0) dinfo.isMultiFunction = true;
        else dinfo.isMultiFunction = false;

        if(dinfo.isMultiFunction)
        {
            for(uint8_t i = 1; i < 8; i++)
            {
                dinfo.functions[i] = getDeviceFunction(bus, device, i);
            }
        }

        return dinfo;
    }

    void printDeviceInfoEntry(DeviceInfo& Dinfo)
    {
        FunctionInfo& f0 = Dinfo.functions[0];

        printf("\t%3u | %2u     | 0x%2x  | %3u      | %3u     | 0x%4x    | 0x%4x   \n", Dinfo.bus, Dinfo.device_no
                                            , f0.classCode, f0.subClass, f0.progIF, f0.vendorID, f0.deviceID);
    }

    void checkFunction(DeviceInfo& deviceInfo, uint8_t function)
    {
        uint8_t secondaryBus;

        FunctionInfo& f0 = deviceInfo.functions[0];
        
        if ((f0.classCode == 0x6) && (f0.subClass == 0x4))
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

        if((deviceInfo.functions[0].headerType & 0x80) > 0)
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

        DeviceInfo device0 = getDevice(0, 0);

        if (!device0.isMultiFunction)
        {
            // Single PCI host controller
            checkBus(0);
        }
        else
        {
            // Multiple PCI host controllers
            for (uint8_t function = 0; function < 8; function++)
            {
                if (device0.functions[function].vendorID != 0xFFFF) break;
                checkBus(function);
            }
        }
    }
}
