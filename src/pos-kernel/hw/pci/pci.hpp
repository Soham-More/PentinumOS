#pragma once

#include <includes.h>

#include <std/std.hpp>
#include <std/ds.hpp>

#include <vfs/vfs.hpp>

namespace bus::pci
{
    struct bar_info;
    enum class bar_type;

    enum class Command : u16
    {
        IO_SPACE                = 0x001,
        MEMORY_SPACE            = 0x002,
        BUS_MASTER              = 0x004,
        SPECIAL_CYCLES          = 0x008,
        MW_INV_ENABLE           = 0x010,
        VGA_PALLETE_SNOOP       = 0x020,
        PARITY_ERROR_RESPONSE   = 0x040,
        SERR_ENABLE             = 0x080,
        FAST_BACKTOBACK_ENABLE  = 0x100,
        INT_DISABLE             = 0x200,
    };
    enum class Status : u16
    {
        INT_STATUS      = 0x08,
        CAPABILITY_LIST = 0x10,
        CAPABLE_66MHz   = 0x20,

        CAPABLE_FAST_BACKTOBACK  = 0x40,
        MASTER_DATA_PARITY_ERROR = 0x80,
        
        SIG_TARGET_ABORT = 0x400,
        REV_TARGET_ABORT = 0x800,
        REV_MASTER_ABORT = 0x1000,
        SIG_SYSTEM_ERROR = 0x2000,
        
        DETECTED_PARITY_ERROR = 0x4000,
    };

    u32 config_read_u32(u8 bus_no, u8 device_no, u8 function, u8 register_offset);
    void config_write_u32(u8 bus_no, u8 device_no, u8 function, u8 register_offset, u32 value);

    struct device
    {
        u8 bus;
        u8 device_no;
        u8 function;

        u16 vendorID;
        u16 deviceID;
        u8  revisionID;

        u8 classCode;
        u8 subClass;
        u8 progIF;

        u32 portBase;
        u32 size;

        u8 headerType;

        bar_type type;

        bool is_valid();

        // WARNING: use only with unsigned ints.
        template<class T>
        T config_read(u8 register_offset)
        {
            u8 offset = register_offset & (0x4 - sizeof(T));

            u32 dword = config_read_u32(bus, device_no, function, register_offset);

            return (dword >> (offset * 8)) & (T)-1;
        }

        // WARNING: use only with unsigned ints.
        template<class T>
        void config_write(u8 register_offset, T value)
        {
            // preserve other bits
            u32 curr_value = config_read_u32(bus, device_no, function, register_offset);
            config_write_u32(bus, device_no, function, register_offset, (curr_value & ~((T)-1)) | value);
        }

        void* alloc_bar(u8 barID, bool isFrameBuffer = false);

        void outb(u16 register_offset, u8 value);
        u8 inb(u16 register_offset);

        void outw(u16 register_offset, u16 value);
        u16 inw(u16 register_offset);
        
        void outl(u16 register_offset, u32 value);
        u32 inl(u16 register_offset);

        // setup the device
        void setup();
    };

    
    void enumerate_bus(vfs::vfs_t* gvfs);
    
    void pretty_print_bus(vfs::vfs_t* gvfs);
    
    i32 init(vfs::vfs_t* gvfs);

    //std::vector<device*> get_all_devices();
    //device* get_device(u8 classCode, u8 subClass);
}

