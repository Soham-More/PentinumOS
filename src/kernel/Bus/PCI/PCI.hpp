#pragma once

#include <includes.h>

#define PCI_INIT_OK 0
#define PCI_INIT_NO_DRIVER 1

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

        template<class T>
        T configRead(uint8_t register_offset);

        template<class T>
        void configWrite(uint8_t register_offset, T value);

        void* allocBAR(uint8_t barID, bool isFrameBuffer = false);
    };
    
    void enumeratePCIBus();
    void prettyPrintDevices();

    PCI_DEVICE* getPCIDevice(uint8_t classCode, uint8_t subClass);
}
