#pragma once

#include <includes.h>

namespace PCI
{
    struct BARInfo;
    enum class BAR_Type;

    enum class Command : uint16_t
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
    enum class Status : uint16_t
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

    uint32_t configReadDword(uint8_t bus_no, uint8_t device_no, uint8_t function, uint8_t register_offset);
    void configWriteDword(uint8_t bus_no, uint8_t device_no, uint8_t function, uint8_t register_offset, uint32_t value);

    struct PCI_DEVICE
    {
        uint8_t bus;
        uint8_t device_no;
        uint8_t function;

        uint16_t vendorID;
        uint16_t deviceID;
        uint8_t  revisionID;

        uint8_t classCode;
        uint8_t subClass;
        uint8_t progIF;

        uint32_t portBase;
        uint32_t size;

        uint8_t headerType;

        BAR_Type type;

        bool isValid();

        // WARNING: use only with unsigned ints.
        template<class T>
        T configRead(uint8_t register_offset)
        {
            uint8_t offset = register_offset & (0x4 - sizeof(T));

            uint32_t dword = configReadDword(bus, device_no, function, register_offset);

            return (dword >> (offset * 8)) & (T)-1;
        }

        // WARNING: use only with unsigned ints.
        template<class T>
        void configWrite(uint8_t register_offset, T value)
        {
            // preserve other bits
            uint32_t curr_value = configReadDword(bus, device_no, function, register_offset);
            configWriteDword(bus, device_no, function, register_offset, (curr_value & ~((T)-1)) | value);
        }

        void* allocBAR(uint8_t barID, bool isFrameBuffer = false);

        void outb(uint16_t register_offset, uint8_t value);
        uint8_t inb(uint16_t register_offset);

        void outw(uint16_t register_offset, uint16_t value);
        uint16_t inw(uint16_t register_offset);
        
        void outl(uint16_t register_offset, uint32_t value);
        uint32_t inl(uint16_t register_offset);
    };

    void enumeratePCIBus();
    void prettyPrintDevices();

    PCI_DEVICE* getPCIDevice(uint8_t classCode, uint8_t subClass);
}
