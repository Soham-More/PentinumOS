#pragma once

#include <includes.h>

namespace PCI
{
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

    struct FunctionInfo
    {
        // which device?
        uint8_t bus;
        uint8_t device_no;

        uint8_t function;

        uint16_t vendorID;
        uint16_t deviceID;
        uint8_t  revisionID;

        uint8_t classCode;
        uint8_t subClass;

        uint8_t progIF;

        uint8_t headerType;

        bool isValid();

        // returns true if success
        bool self_test();

        // gets capability pointer
        uint8_t getCapabilityPointer();
    };
    struct DeviceInfo
    {
        uint8_t bus;
        uint8_t device_no;

        bool isMultiFunction;

        FunctionInfo functions[8];

        bool isValid();
    };

    struct GENERAL_HEADER
    {
        uint32_t BAR0;
        uint32_t BAR1;
        uint32_t BAR2;
        uint32_t BAR3;
        uint32_t BAR4;
        uint32_t BAR5;
        uint32_t cardbusCISPointer;
        uint16_t subSystemVendorID;
        uint16_t subSystemID;
        uint32_t expansionROMbase;
        uint8_t  capabilityPointer;
        uint8_t  reserved_u8;
        uint16_t reserved_u16;
        uint32_t reserved_u32;
        uint8_t  int_line;
        uint8_t  int_pin;
        uint8_t  minGrant;
        uint8_t  maxLatency;
    } _packed;

    uint32_t configReadRegister(uint8_t bus_no, uint8_t device_no, uint8_t function, uint8_t _register);

    uint32_t configReadDword(uint8_t bus_no, uint8_t device_no, uint8_t function, uint8_t register_offset);
    uint16_t configReadWord (uint8_t bus_no, uint8_t device_no, uint8_t function, uint8_t register_offset);
    uint8_t  configReadByte (uint8_t bus_no, uint8_t device_no, uint8_t function, uint8_t register_offset);

    uint32_t configReadDword(FunctionInfo& function, uint8_t register_offset);
    uint16_t configReadWord (FunctionInfo& function, uint8_t register_offset);
    uint8_t  configReadByte (FunctionInfo& function, uint8_t register_offset);

    void configWriteDword(uint8_t bus_no, uint8_t device_no, uint8_t function, uint8_t register_offset, uint32_t value);
    void configWriteWord (uint8_t bus_no, uint8_t device_no, uint8_t function, uint8_t register_offset, uint16_t value);
    void configWriteByte (uint8_t bus_no, uint8_t device_no, uint8_t function, uint8_t register_offset, uint8_t value);

    void configWriteDword(FunctionInfo& function, uint8_t register_offset, uint32_t value);
    void configWriteWord (FunctionInfo& function, uint8_t register_offset, uint16_t value);
    void configWriteByte (FunctionInfo& function, uint8_t register_offset, uint8_t  value);

    FunctionInfo getDeviceFunction(uint8_t bus, uint8_t device, uint8_t function);
    DeviceInfo getDevice(uint8_t bus, uint8_t device);

    void checkFunction(DeviceInfo& deviceInfo, uint8_t function);
    void checkDevice(DeviceInfo& deviceInfo);
    void checkBus(uint8_t bus);

    void enumeratePCIBus();
    void prettyPrintDevices();

    void initDevices();
}
