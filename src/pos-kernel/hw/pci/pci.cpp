#include "pci.hpp"

#include <arch/i686.h>
#include <cpu/paging.hpp>
#include <cpu/memory.hpp>

#include <std/std.hpp>
#include <std/ds.hpp>

namespace bus::pci
{
    const u16 CONFIG_ADDRESS = 0xCF8;
    const u16 CONFIG_DATA    = 0xCFC;

    struct device_info
    {
        u8 bus;
        u8 device_no;

        bool isMultiFunction;

        device functions[8];

        bool isValid() { return functions[0].vendorID != 0xFFFF; }
    };
    struct bar_info
    {
        void* address;
        size_t size;
        bar_type type;
    };
    enum class bar_type
    {
        MEMORY_MAPPED = 0,
        IO_MAPPED = 1
    };

    static size_t pci_device_count = 0;
    //static std::vector<device_info> pci_devices;

    device getDeviceFunction(u8 bus, u8 device, u8 function);
    device_info getDevice(u8 bus, u8 device);
    void checkFunction(vfs::vfs_t* gvfs, device_info& deviceInfo, u8 function);
    void checkDevice(vfs::vfs_t* gvfs, device_info& deviceInfo);
    void checkBus(vfs::vfs_t* gvfs, u8 bus);

    u32 config_read_u32(u8 bus_no, u8 device_no, u8 function, u8 register_offset)
    {
        u32 lbus = bus_no;
        u32 ldevice = device_no & 0x1F;
        u32 lfunc = function & 0x7;
        u32 loffset = register_offset & 0xFC;

        // (1 << 31): enable bit
        u32 configAddress = (1 << 31) | (lbus << 16) | (ldevice << 11) | (lfunc << 8) | (loffset);

        // send address
        x86_outl(CONFIG_ADDRESS, configAddress);

        // get register
        u32 configData = x86_inl(CONFIG_DATA);
        return configData;
    }
    u16 config_read_u16(u8 bus_no, u8 device_no, u8 function, u8 register_offset)
    {
        u8 dword_offset = (register_offset & 0x2) >> 1;

        u32 dword = config_read_u32(bus_no, device_no, function, register_offset);

        return (dword >> (dword_offset * 16)) & 0xFFFF;
    }
    u8 config_read_u8(u8 bus_no, u8 device_no, u8 function, u8 register_offset)
    {
        u8 dword_offset = (register_offset & 0x3);

        u32 dword = config_read_u32(bus_no, device_no, function, register_offset);

        return (dword >> (dword_offset * 8)) & 0xFF;
    }

    void config_write_u32(u8 bus_no, u8 device_no, u8 function, u8 register_offset, u32 value)
    {
        u32 lbus = bus_no;
        u32 ldevice = device_no & 0x1F;
        u32 lfunc = function & 0x7;
        u32 loffset = register_offset & 0xFC;

        // (1 << 31): enable bit
        u32 configAddress = (1 << 31) | (lbus << 16) | (ldevice << 11) | (lfunc << 8) | (loffset);

        // send address
        x86_outl(CONFIG_ADDRESS, configAddress);
        x86_outl(CONFIG_DATA, value);
    }
    void config_write_u16(u8 bus_no, u8 device_no, u8 function, u8 register_offset, u16 value)
    {
        u8 dword_offset = (register_offset & 0x2) >> 1;

        // preserve other bits
        u32 curr_value = config_read_u32(bus_no, device_no, function, register_offset);
        config_write_u32(bus_no, device_no, function, register_offset, (curr_value & ~(UINT16_MAX)) | value);
    }
    void config_write_u8 (u8 bus_no, u8 device_no, u8 function, u8 register_offset, u8 value)
    {
        u8 dword_offset = (register_offset & 0x2) >> 1;

        // preserve other bits
        u32 curr_value = config_read_u32(bus_no, device_no, function, register_offset);
        config_write_u16(bus_no, device_no, function, register_offset, (curr_value & ~(UINT8_MAX)) | value);
    }

    device getDeviceFunction(u8 bus, u8 device_no, u8 function)
    {
        device finfo;

        finfo.function = function;
        finfo.bus = bus;
        finfo.device_no = device_no;

        finfo.vendorID = config_read_u16(bus, device_no, function, 0x00);

        if(finfo.vendorID != 0xFFFF)
        {
            finfo.deviceID = config_read_u16(bus, device_no, function, 0x01);

            finfo.revisionID = config_read_u8(bus, device_no, function, 0x08);
            finfo.progIF     = config_read_u8(bus, device_no, function, 0x09);
            finfo.subClass   = config_read_u8(bus, device_no, function, 0x0A);
            finfo.classCode  = config_read_u8(bus, device_no, function, 0x0B);

            // mask multi function bit
            finfo.headerType = config_read_u8(bus, device_no, function, 0x0E) & (~0x80);
        }

        finfo.portBase = nullptr;

        return finfo;
    }
    device_info getDevice(u8 bus, u8 device)
    {
        device_info dinfo;

        dinfo.bus = bus;
        dinfo.device_no = device;

        dinfo.functions[0] = getDeviceFunction(bus, device, 0);

        u8 multiFunction = config_read_u8(bus, device, 0, 0x0E) & 0x80;

        if(multiFunction > 0) dinfo.isMultiFunction = true;
        else dinfo.isMultiFunction = false;

        if(dinfo.isMultiFunction)
        {
            for(u8 i = 1; i < 8; i++)
            {
                dinfo.functions[i] = getDeviceFunction(bus, device, i);
            }
        }

        return dinfo;
    }

    void pci_dev_read (vfs::node_t* node, size_t offset, void* buffer, size_t len)
    {
        if(len == 4)      *(u32*)buffer = reinterpret_cast<device*>(node->data)->inl((u16)offset);
        else if(len == 2) *(u16*)buffer = reinterpret_cast<device*>(node->data)->inw((u16)offset);
        else if(len == 1) *(u8* )buffer = reinterpret_cast<device*>(node->data)->inb((u16)offset);
        else log_warn("[PCI] invalid io request: (len:%u)", len);
    }
    i32 pci_dev_write(vfs::node_t* node, size_t offset, const void* buffer, size_t len)
    {
        if(len == 4)      reinterpret_cast<device*>(node->data)->outl((u16)offset, *(u32*)buffer);
        else if(len == 2) reinterpret_cast<device*>(node->data)->outw((u16)offset, *(u16*)buffer);
        else if(len == 1) reinterpret_cast<device*>(node->data)->outb((u16)offset, *(u8* )buffer);
        else {log_warn("[PCI] invalid io request: (len:%u)", len); return EINVOP; }

        return OP_SUCCESS;
    }

    void add_device_to_vfs(vfs::vfs_t* gvfs, device& _device)
    {
        std::string device_name = "dev" + std::utos(pci_device_count);
        pci_device_count++;

        vfs::node_t dnode = vfs::make_node(device_name.take(), false);
        dnode.flags |= vfs::NODE_DEVICE;
        dnode.driver_uid = UID_CLASS_USBCTRL;
        dnode.data = new device(_device);

        dnode.write = pci_dev_write;
        dnode.read  = pci_dev_read;

        vfs::add_dnode(gvfs, "/hw/pci", dnode);
    }

    void checkFunction(vfs::vfs_t* gvfs, device_info& deviceInfo, u8 function)
    {
        u8 secondaryBus;

        device& f0 = deviceInfo.functions[0];
        
        if ((f0.classCode == 0x6) && (f0.subClass == 0x4))
        {
            secondaryBus = config_read_u8(deviceInfo.bus, deviceInfo.device_no, function, 0x19);
            checkBus(gvfs, secondaryBus);
        }
    }
    void checkDevice(vfs::vfs_t* gvfs, device_info& deviceInfo)
    {
        if(!deviceInfo.isValid()) return;

        checkFunction(gvfs, deviceInfo, 0);

        // add device to list
        //pci_devices.push_back(deviceInfo);
        add_device_to_vfs(gvfs, deviceInfo.functions[0]);

        if(deviceInfo.isMultiFunction)
        {
            // multi function device
            for(u8 func = 1; func < 8; func++)
            {
                if(deviceInfo.functions[func].is_valid())
                {
                    checkFunction(gvfs, deviceInfo, func);
                }
            }
        }
    }
    void checkBus(vfs::vfs_t* gvfs, u8 bus)
    {
        for(u8 device = 0; device < 32; device++)
        {
            device_info Dinfo = getDevice(bus, device);

            if(Dinfo.isValid())
            {
                checkDevice(gvfs, Dinfo);
            }
        }
    }

    void enumerate_bus(vfs::vfs_t* gvfs)
    {
        device_info device0 = getDevice(0, 0);

        if (!device0.isMultiFunction)
        {
            // Single PCI host controller
            checkBus(gvfs, 0);
        }
        else
        {
            // Multiple PCI host controllers
            for (u8 function = 0; function < 8; function++)
            {
                if (device0.functions[function].vendorID != 0xFFFF) break;
                checkBus(gvfs, function);
            }
        }
    }

    void pretty_print_bus(vfs::vfs_t* gvfs)
    {
        vfs::node_t* pci_root = vfs::get_node(gvfs, "/hw/pci");
        if(IS_ERR_PTR(pci_root)) return;

        log_info("PCI Devices:\n");
        log_info("\tBUS | Device | Class | Subclass | Prog IF | Vendor ID | Device ID\n");

        vfs::node_t* child = pci_root->children_head;

        while(child != nullptr)
        {
            device& f0 = *reinterpret_cast<device*>(child->data);

            log_info("\t%3u | %2u     | 0x%2x  | %3u      | 0x%2x    | 0x%4x    | 0x%4x   \n", f0.bus, f0.device_no
                                            , f0.classCode, f0.subClass, f0.progIF, f0.vendorID, f0.deviceID);
            
            child = child->next;
        }
    }

    i32 init(vfs::vfs_t *gvfs)
    {
        i32 error_code = vfs::add_vnode(gvfs, "/hw/", vfs::make_node("pci", true));
        // propogate error
        if(error_code != OP_SUCCESS) { log_error("[PCI] Failed to make pci vnode in vfs!\ns"); return error_code;}

        enumerate_bus(gvfs);

        return OP_SUCCESS;
    }

    bar_info getDeviceBAR(device* pciDevice, u8 bar)
    {
        u8 offset = 0x10 + (bar * 4);
        device& function = *pciDevice;

        // preserve BAR Value
        u32 BAR = function.config_read<u32>(offset);

        u32 BAR_size = 0;

        // disable memory decode and IO decode
        u16 commandByte = function.config_read<u16>(0x04);
        commandByte &= ~(0x03);
        function.config_write(0x04, commandByte);

        // write all 1s
        function.config_write(offset, UINT32_MAX);

        u32 sizeMask = function.config_read<u32>(offset);

        // if it is 0, it is a memory mapped bar
        if((sizeMask & 0x01) == 0)
        {
            if((BAR & 0x6) == 0x00)
            {
                BAR_size = ~(sizeMask & ~((u32)0xF));
            }
            // else if it is 16-bit
            else if((BAR & 0x6) == 0x2)
            {
                BAR_size = ~(sizeMask & ~((u16)0xF));
            }
            // else if it is 64-bit
            // currently fails
            else if((BAR & 0x6) == 0x4)
            {
                // enable memory decode and IO decode
                commandByte = function.config_read<u16>(0x04);
                commandByte |= 0x03;
                function.config_write<u16>(0x04, commandByte);

                log_error("\n[PCI] Error: Detected 64-bit BAR Device(not supported): bus:%d, device_no:%f, functionid:%d\n", function.bus, function.device_no, function.function);
                return bar_info{nullptr, 0};
            }
        }
        // IO Mapped BAR
        else
        {
            // enable memory decode and IO decode
            commandByte = function.config_read<u16>(0x04);
            commandByte |= 0x03;
            function.config_write<u16>(0x04, commandByte);

            // restore bar settings
            function.config_write(offset, BAR & ~0x3);

            return bar_info{reinterpret_cast<void*>(BAR & ~0x3), ~(sizeMask & ~3) + 1, bar_type::IO_MAPPED};
        }

        // enable memory decode and IO decode
        commandByte = function.config_read<u16>(0x04);
        commandByte |= 0x03;
        function.config_write<u16>(0x04, commandByte);

        function.config_write(offset, BAR);

        return bar_info{reinterpret_cast<void*>(BAR & ~(0xF)), ~(sizeMask & ~0xF), bar_type::MEMORY_MAPPED};
    }

    bool device::is_valid()
    {
        return vendorID != 0xFFFF;
    }
    void* device::alloc_bar(u8 barID, bool isFrameBuffer)
    {
        u8 offset = 0x10 + (barID * 4);

        // preserve BAR Value
        u32 BAR = config_read<u32>(offset);

        u32 BAR_size = 0;

        // disable memory decode and IO decode
        u16 commandByte = config_read<u16>(0x04);
        commandByte &= ~(0x03);
        config_write(0x04, commandByte);

        // write all 1s
        config_write(offset, UINT32_MAX);

        u32 sizeMask = config_read<u32>(offset);

        // if it is 0, it is a memory mapped bar
        if((sizeMask & 0x01) == 0)
        {
            if((BAR & 0x6) == 0x00)
            {
                BAR_size = ~(sizeMask & ~((u32)0xF));
            }
            // else if it is 16-bit
            else if((BAR & 0x6) == 0x2)
            {
                BAR_size = ~(sizeMask & ~((u16)0xF));
            }
            // else if it is 64-bit
            // currently fails
            else if((BAR & 0x6) == 0x4)
            {
                // enable memory decode and IO decode
                commandByte = config_read<u16>(0x04);
                commandByte |= 0x03;
                config_write<u16>(0x04, commandByte);

                log_error("\n[PCI] Error: Detected 64-bit BAR Device(not supported): bus:%d, device_no:%f, functionid:%d\n", bus, device_no, function);
                return nullptr;
            }
        }
        // IO Mapped BAR
        else
        {
            // restore bar settings
            config_write(offset, BAR & ~0x3);

            return nullptr;
        }
        
        // now allocate it some memory
        u32 new_bar = reinterpret_cast<u32>(cpu::alloc_io_pages((BAR_size + 1) / PAGE_SIZE, (BAR_size + 1) / PAGE_SIZE));

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
                cpu::setFlagsPages(new_bar, cpu::PAGE_PRESENT | cpu::PAGE_WRITETHROUGH, (BAR_size + 1) / PAGE_SIZE);
            }
        }
        // mark as Uncachable
        else
        {
            cpu::setFlagsPages(new_bar, cpu::PAGE_PRESENT | cpu::PAGE_DISABLE_CACHING, (BAR_size  + 1) / PAGE_SIZE);
        }
        
        // restore bar settings
        new_bar |= BAR & 0xF;

        config_write(offset, new_bar);

        // enable memory decode and IO decode
        commandByte = config_read<u16>(0x04);
        commandByte |= 0x03;
        config_write<u16>(0x04, commandByte);

        u32 read_word = config_read<u32>(offset);

        return reinterpret_cast<void*>(new_bar & ~(0xF));
    }
    void device::outb(u16 register_offset, u8 value)
    {
        x86_outw(portBase + register_offset, value);
    }
    u8 device::inb(u16 register_offset)
    {
        return x86_inw(portBase + register_offset);
    }
    void device::outw(u16 register_offset, u16 value)
    {
        x86_outw(portBase + register_offset, value);
    }
    u16 device::inw(u16 register_offset)
    {
        u32 val = x86_inw(portBase + register_offset);
        return val;
    }
    void device::outl(u16 register_offset, u32 value)
    {
        x86_outl(portBase + register_offset, value);
    }
    u32 device::inl(u16 register_offset)
    {
        u32 val = x86_inl(portBase + register_offset);
        return val;
    }

    void device::setup()
    {
        // the device is already setup
        if(this->portBase != nullptr) return;

        for(size_t i = 0; i < 6; i++)
        {
            bar_info bInfo = getDeviceBAR(this, i);
            if(bInfo.type == bar_type::IO_MAPPED)
            {
                this->portBase = reinterpret_cast<u32>(bInfo.address);
            }
        }
    }

/*
    std::vector<device*> get_all_devices()
    {
        std::vector<device*> devices;

        for(size_t i = 0; i < pci_devices.size(); i++)
        {
            devices.push_back(&pci_devices[i].functions[0]);

            if(!pci_devices[i].isMultiFunction) continue;

            for(size_t j = 1; j < 8; j++)
            {
                devices.push_back(&pci_devices[i].functions[j]);
            }
        }

        return devices;
    }
*/
    /*
    device *get_device(u8 classCode, u8 subClass)
    {
        device* pciDevice = nullptr;

        for(size_t i = 0; i < pci_devices.size(); i++)
        {
            if(pci_devices[i].functions[0].classCode == classCode && pci_devices[i].functions[0].subClass == subClass)
            {
                pciDevice = &pci_devices[i].functions[0];
            }

            if(!pci_devices[i].isMultiFunction) continue;

            for(size_t j = 1; j < 8; j++)
            {
                if(pci_devices[i].functions[j].classCode == classCode && pci_devices[i].functions[j].subClass == subClass)
                {
                    pciDevice = &pci_devices[i].functions[j];
                }
            }
        }
        if(!pciDevice) return nullptr;

        for(size_t i = 0; i < 6; i++)
        {
            bar_info bInfo = getDeviceBAR(pciDevice, i);
            if(bInfo.type == bar_type::IO_MAPPED)
            {
                pciDevice->portBase = reinterpret_cast<u32>(bInfo.address);
            }
        }

        return pciDevice;
    }
    */
}

