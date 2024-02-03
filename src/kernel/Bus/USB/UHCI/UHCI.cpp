#include "UHCI.hpp"

#include <std/stdio.h>
#include <Drivers/PIT.h>
#include <i686/x86.h>

#define GLOBAL_RESET_COUNT 5

#define UHCI_COMMAND 0x0
#define UHCI_STATUS  0x0
#define UHCI_INTR    0x4

namespace UHCI
{
    struct CapabilityRegister
    {
        uint8_t  size;
        uint8_t  reserved_u8;
        uint16_t interfaceVersion;
        uint32_t structuralParameters;
        uint32_t capabilityParameters;
        uint64_t HCSP_PORTROUTE;
    }_packed;

    struct OperationalRegister
    {
        uint16_t command;
        uint16_t status;
        uint16_t intr;
        uint32_t frameListBase; // low 8 bits have frame number info.
        uint16_t segementSelector4G;
        uint8_t  SOF;
        uint8_t  portsc1;
        uint8_t  portsc2;
    }_packed;

    bool init_uhci_controller(PCI::PCI_DEVICE* device)
    {
        // enable bus matering
        device->configWrite<uint8_t>(0x4, 0x5);

        // disable all interrupts
        device->outw(UHCI_INTR, 0x0);

        // reset controller
        for(int i = 0; i < GLOBAL_RESET_COUNT; i++)
        {
            device->outw(UHCI_COMMAND, 0x4);
            PIT_sleep(11);
            device->outw(UHCI_COMMAND, 0x0);
        }

        // check if cmd register is default
        if(device->inw(UHCI_COMMAND) != 0x0) return false;
        // check if status register is default
        if(device->inw(UHCI_COMMAND) != 0x0) return false;

    }
}