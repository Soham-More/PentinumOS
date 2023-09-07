#include "PCI.hpp"

#include <std/stdio.h>
#include <std/logger.h>
#include <i686/x86.h>
#include <std/vector.hpp>

namespace PCI
{
    const uint16_t CONFIG_ADDRESS = 0xCF8;
    const uint16_t CONFIG_DATA    = 0xCFC;

    static std::vector<DeviceInfo> pciDevices;

    bool FunctionInfo::isValid()
    {
        return vendorID != 0xFFFF;
    }

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

    uint32_t configReadDword(FunctionInfo& function, uint8_t register_offset)
    {
        return configReadDword(function.bus, function.device_no, function.function, register_offset);
    }
    uint16_t configReadWord(FunctionInfo& function, uint8_t register_offset)
    {
        return configReadWord(function.bus, function.device_no, function.function, register_offset);
    }
    uint8_t configReadByte(FunctionInfo& function, uint8_t register_offset)
    {
        return configReadByte(function.bus, function.device_no, function.function, register_offset);
    }

    void configWriteDword(uint8_t bus_no, uint8_t device_no, uint8_t function, uint8_t register_offset, uint32_t value)
    {
        uint32_t lbus = bus_no;
        uint32_t ldevice = device_no & 0x1F;
        uint32_t lfunc = function & 0x7;
        uint32_t loffset = register_offset & 0xFC;

        // (1 << 31): enable bit
        uint32_t configAddress = (1 << 31) | (lbus << 16) | (ldevice << 11) | (lfunc << 8) | (loffset);

        // send address
        x86_outl(CONFIG_ADDRESS, configAddress);
        x86_outl(CONFIG_ADDRESS, value);
    }
    void configWriteWord (uint8_t bus_no, uint8_t device_no, uint8_t function, uint8_t register_offset, uint16_t value)
    {
        uint8_t dword_offset = (register_offset & 0x2) >> 1;

        // preserve other bits
        uint32_t curr_value = configReadDword(bus_no, device_no, function, register_offset);
        configWriteWord(bus_no, device_no, function, register_offset, (curr_value & ~(UINT16_MAX)) | value);
    }
    void configWriteByte (uint8_t bus_no, uint8_t device_no, uint8_t function, uint8_t register_offset, uint8_t value)
    {
        uint8_t dword_offset = (register_offset & 0x2) >> 1;

        // preserve other bits
        uint32_t curr_value = configReadDword(bus_no, device_no, function, register_offset);
        configWriteWord(bus_no, device_no, function, register_offset, (curr_value & ~(UINT8_MAX)) | value);
    }

    void configWriteDword(FunctionInfo& function, uint8_t register_offset, uint32_t value)
    {
        configWriteDword(function.bus, function.device_no, function.function, register_offset, value);
    }
    void configWriteWord (FunctionInfo& function, uint8_t register_offset, uint16_t value)
    {
        configWriteWord(function.bus, function.device_no, function.function, register_offset, value);
    }
    void configWriteByte (FunctionInfo& function, uint8_t register_offset, uint8_t value)
    {
        configWriteByte(function.bus, function.device_no, function.function, register_offset, value);
    }

    FunctionInfo getDeviceFunction(uint8_t bus, uint8_t device, uint8_t function)
    {
        FunctionInfo finfo;

        finfo.function = function;
        finfo.bus = bus;
        finfo.device_no = device;

        finfo.vendorID = configReadWord(bus, device, function, 0x00);

        if(finfo.vendorID != 0xFFFF)
        {
            finfo.deviceID = configReadWord(bus, device, function, 0x01);

            finfo.revisionID = configReadByte(bus, device, function, 0x08);
            finfo.progIF     = configReadByte(bus, device, function, 0x09);
            finfo.subClass   = configReadByte(bus, device, function, 0x0A);
            finfo.classCode  = configReadByte(bus, device, function, 0x0B);

            // mask multi function bit
            finfo.headerType = configReadByte(bus, device, function, 0x0E) & (~0x80);
        }

        return finfo;
    }
    DeviceInfo getDevice(uint8_t bus, uint8_t device)
    {
        DeviceInfo dinfo;

        dinfo.bus = bus;
        dinfo.device_no = device;

        dinfo.functions[0] = getDeviceFunction(bus, device, 0);

        uint8_t multiFunction = configReadByte(bus, device, 0, 0x0E) & 0x80;

        if(multiFunction > 0) dinfo.isMultiFunction = true;
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

        // add device to list
        pciDevices.push_back(deviceInfo);

        if(deviceInfo.isMultiFunction)
        {
            // multi function device
            for(uint8_t func = 1; func < 8; func++)
            {
                if(deviceInfo.functions[func].isValid())
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

    void prettyPrintDevices()
    {
        log_info("PCI Devices:\n");
        log_info("\tBUS | Device | Class | Subclass | Prog IF | Vendor ID | Device ID\n");

        for(uint32_t i = 0; i < pciDevices.size(); i++)
        {
            FunctionInfo& f0 = pciDevices[i].functions[0];

            log_info("\t%3u | %2u     | 0x%2x  | %3u      | 0x%2x    | 0x%4x    | 0x%4x   \n", f0.bus, f0.device_no
                                            , f0.classCode, f0.subClass, f0.progIF, f0.vendorID, f0.deviceID);
        }
    }

    void initFunctionBAR(FunctionInfo& function, uint8_t bar)
    {
        uint8_t offset = (0x04 + bar) * 4;

        // preserve BAR Value
        uint32_t BAR = configReadDword(function, offset);

        uint32_t BAR_size = 0;

        // if it is 0, it is a memory mapped bar
        // ie 16-Byte aligned address
        if(BAR & 0x1 == 0)
        {
            // if it is 32-bit
            if(BAR & 0x6 == 0x00)
            {
                // write all 1s
                configWriteDword(function, offset, UINT32_MAX);

                uint32_t BAR_encoded_size = configReadDword(function, offset);

                BAR_size = ~(BAR_encoded_size & ~((uint32_t)0xF));
            }
            // else if it is 16-bit
            else if(BAR & 0x6 == 0x2)
            {
                // write all 1s
                configWriteDword(function, offset, UINT32_MAX);

                uint32_t BAR_encoded_size = configReadDword(function, offset);

                BAR_size = ~(BAR_encoded_size & ~((uint32_t)0xF));
            }
            // else if it is 64-bit
            // currently fails
            else if(BAR & 0x6 == 0x4)
            {
                log_error("\ERROR: not supported\n\tDetected 64-bit BAR Device: bus:%d, device_no:%f, functionid:%d\n", function.bus, function.device_no, function.function);
                return;
            }
        }

        ;
    }

    void initDevice(DeviceInfo& device)
    {
        // generic header
        if(device.functions[0].headerType = 0)
        {
            ;
        }
    }

    void initDevices()
    {
        ;
    }
}
