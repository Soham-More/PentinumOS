#include "PCI.hpp"

#include <std/stdio.h>
#include <std/logger.h>
#include <i686/x86.h>
#include <std/vector.hpp>
#include <std/memory.h>
#include <System/Paging.hpp>

namespace PCI
{
    const uint16_t CONFIG_ADDRESS = 0xCF8;
    const uint16_t CONFIG_DATA    = 0xCFC;

    struct DeviceInfo
    {
        uint8_t bus;
        uint8_t device_no;

        bool isMultiFunction;

        PCI_DEVICE functions[8];

        bool isValid() { return functions[0].vendorID != 0xFFFF; }
    };
    struct BARInfo
    {
        void* address;
        size_t size;
        BAR_Type type;
    };
    enum class BAR_Type
    {
        MEMORY_MAPPED = 0,
        IO_MAPPED = 1
    };

    static std::vector<DeviceInfo> pciDevices;

    template<> uint8_t PCI_DEVICE::configRead<uint8_t>(uint8_t register_offset);
    template<> uint16_t PCI_DEVICE::configRead<uint16_t>(uint8_t register_offset);
    template<> uint32_t PCI_DEVICE::configRead<uint32_t>(uint8_t register_offset);
    template<> void PCI_DEVICE::configWrite<uint8_t>(uint8_t register_offset, uint8_t value);
    template<> void PCI_DEVICE::configWrite<uint16_t>(uint8_t register_offset, uint16_t value);
    template<> void PCI_DEVICE::configWrite<uint32_t>(uint8_t register_offset, uint32_t value);

    PCI_DEVICE getDeviceFunction(uint8_t bus, uint8_t device, uint8_t function);
    DeviceInfo getDevice(uint8_t bus, uint8_t device);
    void checkFunction(DeviceInfo& deviceInfo, uint8_t function);
    void checkDevice(DeviceInfo& deviceInfo);
    void checkBus(uint8_t bus);

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
        x86_outl(CONFIG_DATA, value);
    }
    void configWriteWord (uint8_t bus_no, uint8_t device_no, uint8_t function, uint8_t register_offset, uint16_t value)
    {
        uint8_t dword_offset = (register_offset & 0x2) >> 1;

        // preserve other bits
        uint32_t curr_value = configReadDword(bus_no, device_no, function, register_offset);
        configWriteDword(bus_no, device_no, function, register_offset, (curr_value & ~(UINT16_MAX)) | value);
    }
    void configWriteByte (uint8_t bus_no, uint8_t device_no, uint8_t function, uint8_t register_offset, uint8_t value)
    {
        uint8_t dword_offset = (register_offset & 0x2) >> 1;

        // preserve other bits
        uint32_t curr_value = configReadDword(bus_no, device_no, function, register_offset);
        configWriteWord(bus_no, device_no, function, register_offset, (curr_value & ~(UINT8_MAX)) | value);
    }

    PCI_DEVICE getDeviceFunction(uint8_t bus, uint8_t device, uint8_t function)
    {
        PCI_DEVICE finfo;

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

        PCI_DEVICE& f0 = deviceInfo.functions[0];
        
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
            PCI_DEVICE& f0 = pciDevices[i].functions[0];

            log_info("\t%3u | %2u     | 0x%2x  | %3u      | 0x%2x    | 0x%4x    | 0x%4x   \n", f0.bus, f0.device_no
                                            , f0.classCode, f0.subClass, f0.progIF, f0.vendorID, f0.deviceID);
        }
    }

    BARInfo getDeviceBAR(PCI_DEVICE* pciDevice, uint8_t bar)
    {
        uint8_t offset = 0x10 + (bar * 4);
        PCI_DEVICE& function = *pciDevice;

        // preserve BAR Value
        uint32_t BAR = function.configRead<uint32_t>(offset);

        uint32_t BAR_size = 0;

        // disable memory decode and IO decode
        uint16_t commandByte = function.configRead<uint16_t>(0x04);
        commandByte &= ~(0x03);
        function.configWrite(0x04, commandByte);

        // write all 1s
        function.configWrite(offset, UINT32_MAX);

        uint32_t sizeMask = function.configRead<uint32_t>(offset);

        // if it is 0, it is a memory mapped bar
        if((sizeMask & 0x01) == 0)
        {
            if((BAR & 0x6) == 0x00)
            {
                BAR_size = ~(sizeMask & ~((uint32_t)0xF));
            }
            // else if it is 16-bit
            else if((BAR & 0x6) == 0x2)
            {
                BAR_size = ~(sizeMask & ~((uint16_t)0xF));
            }
            // else if it is 64-bit
            // currently fails
            else if((BAR & 0x6) == 0x4)
            {
                // enable memory decode and IO decode
                commandByte = function.configRead<uint16_t>(0x04);
                commandByte |= 0x03;
                function.configWrite<uint16_t>(0x04, commandByte);

                log_error("\nERROR: not supported\n\tDetected 64-bit BAR Device: bus:%d, device_no:%f, functionid:%d\n", function.bus, function.device_no, function.function);
                return BARInfo{nullptr, 0};
            }
        }
        // IO Mapped BAR
        else
        {
            // enable memory decode and IO decode
            commandByte = function.configRead<uint16_t>(0x04);
            commandByte |= 0x03;
            function.configWrite<uint16_t>(0x04, commandByte);

            // restore bar settings
            function.configWrite(offset, BAR & ~0x3);

            return BARInfo{reinterpret_cast<void*>(BAR & ~0x3), ~(sizeMask & ~3) + 1, BAR_Type::IO_MAPPED};
        }

        // enable memory decode and IO decode
        commandByte = function.configRead<uint16_t>(0x04);
        commandByte |= 0x03;
        function.configWrite<uint16_t>(0x04, commandByte);

        function.configWrite(offset, BAR);

        return BARInfo{reinterpret_cast<void*>(BAR & ~(0xF)), ~(sizeMask & ~0xF), BAR_Type::MEMORY_MAPPED};
    }

    template<> uint8_t PCI_DEVICE::configRead<uint8_t>(uint8_t register_offset)
    {
        return configReadByte(bus, device_no, function, register_offset);
    }
    template<> uint16_t PCI_DEVICE::configRead<uint16_t>(uint8_t register_offset)
    {
        return configReadWord(bus, device_no, function, register_offset);
    }
    template<> uint32_t PCI_DEVICE::configRead<uint32_t>(uint8_t register_offset)
    {
        return configReadDword(bus, device_no, function, register_offset);
    }

    template<> void PCI_DEVICE::configWrite<uint8_t>(uint8_t register_offset, uint8_t value)
    {
        configWriteByte(bus, device_no, function, register_offset, value);
    }
    template<> void PCI_DEVICE::configWrite<uint16_t>(uint8_t register_offset, uint16_t value)
    {
        configWriteWord(bus, device_no, function, register_offset, value);
    }
    template<> void PCI_DEVICE::configWrite<uint32_t>(uint8_t register_offset, uint32_t value)
    {
        configWriteDword(bus, device_no, function, register_offset, value);
    }

    bool PCI_DEVICE::isValid()
    {
        return vendorID != 0xFFFF;
    }
    void* PCI_DEVICE::allocBAR(uint8_t barID, bool isFrameBuffer)
    {
        uint8_t offset = 0x10 + (barID * 4);

        // preserve BAR Value
        uint32_t BAR = configRead<uint32_t>(offset);

        uint32_t BAR_size = 0;

        // disable memory decode and IO decode
        uint16_t commandByte = configRead<uint16_t>(0x04);
        commandByte &= ~(0x03);
        configWrite(0x04, commandByte);

        // write all 1s
        configWrite(offset, UINT32_MAX);

        uint32_t sizeMask = configRead<uint32_t>(offset);

        // if it is 0, it is a memory mapped bar
        if((sizeMask & 0x01) == 0)
        {
            if((BAR & 0x6) == 0x00)
            {
                BAR_size = ~(sizeMask & ~((uint32_t)0xF));
            }
            // else if it is 16-bit
            else if((BAR & 0x6) == 0x2)
            {
                BAR_size = ~(sizeMask & ~((uint16_t)0xF));
            }
            // else if it is 64-bit
            // currently fails
            else if((BAR & 0x6) == 0x4)
            {
                // enable memory decode and IO decode
                commandByte = configRead<uint16_t>(0x04);
                commandByte |= 0x03;
                configWrite<uint16_t>(0x04, commandByte);

                log_error("\ERROR: not supported\n\tDetected 64-bit BAR Device: bus:%d, device_no:%f, functionid:%d\n", bus, device_no, function);
                return nullptr;
            }
        }
        // IO Mapped BAR
        else
        {
            // restore bar settings
            configWrite(offset, BAR & ~0x3);

            return nullptr;
        }
        
        // now allocate it some memory
        uint32_t new_bar = reinterpret_cast<uint32_t>(alloc_io_space_pages((BAR_size + 1) / PAGE_SIZE, (BAR_size + 1) / PAGE_SIZE));

        // BAR is prefetchable
        if((BAR & (1 << 3)) > 0)
        {
            if(isFrameBuffer)
            {
                // mark as Write Comb
                // TODO
            }
            else
            {
                // then it is write through
                sys::setFlagsPages(new_bar, sys::PAGE_PRESENT | sys::PAGE_WRITETHROUGH, (BAR_size + 1) / PAGE_SIZE);
            }
        }
        // mark as Uncachable
        else
        {
            sys::setFlagsPages(new_bar, sys::PAGE_PRESENT | sys::PAGE_DISABLE_CACHING, (BAR_size  + 1) / PAGE_SIZE);
        }
        
        // restore bar settings
        new_bar |= BAR & 0xF;

        configWrite(offset, new_bar);

        // enable memory decode and IO decode
        commandByte = configRead<uint16_t>(0x04);
        commandByte |= 0x03;
        configWrite<uint16_t>(0x04, commandByte);

        uint32_t read_word = configRead<uint32_t>(offset);

        return reinterpret_cast<void*>(new_bar & ~(0xF));
    }
    void PCI_DEVICE::outw(uint16_t register_offset, uint16_t value)
    {
        x86_outw(portBase + register_offset, value);
    }
    uint16_t PCI_DEVICE::inw(uint16_t register_offset)
    {
        return x86_inw(register_offset);
    }
    PCI_DEVICE* getPCIDevice(uint8_t classCode, uint8_t subClass)
    {
        PCI_DEVICE* pciDevice = nullptr;

        for(size_t i = 0; i < pciDevices.size(); i++)
        {
            if(pciDevices[i].functions[0].classCode == classCode && pciDevices[i].functions[0].subClass == subClass)
            {
                pciDevice = &pciDevices[i].functions[0];
            }

            if(!pciDevices[i].isMultiFunction) continue;

            for(size_t j = 1; j < 8; j++)
            {
                if(pciDevices[i].functions[j].classCode == classCode && pciDevices[i].functions[j].subClass == subClass)
                {
                    pciDevice = &pciDevices[i].functions[j];
                }
            }
        }
        if(!pciDevice) return nullptr;

        for(size_t i = 0; i < 6; i++)
        {
            BARInfo bInfo = getDeviceBAR(pciDevice, i);
            if(bInfo.type == BAR_Type::IO_MAPPED)
            {
                pciDevice->portBase = reinterpret_cast<uint32_t>(bInfo.address);
            }
        }

        return pciDevice;
    }
}
