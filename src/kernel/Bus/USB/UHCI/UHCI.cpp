#include "UHCI.hpp"

#include <std/stdio.h>
#include <Drivers/PIT.h>
#include <i686/x86.h>
#include <std/Heap/heap.hpp>

#define GLOBAL_RESET_COUNT 5

#define UHCI_COMMAND 0x0
#define UHCI_STATUS  0x2
#define UHCI_INTR    0x4
#define UHCI_SOF     0xC

#define UHCI_PCI_LEGACY  0xC0

namespace USB
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

    UHCIController::UHCIController(PCI::PCI_DEVICE* device) : uhciController(device) { }

    void globalResetController(PCI::PCI_DEVICE* device)
    {
        // reset controller
        for(int i = 0; i < GLOBAL_RESET_COUNT; i++)
        {
            device->outw(UHCI_COMMAND, 0x4);
            PIT_sleep(11);
            device->outw(UHCI_COMMAND, 0x0);
        }
    }

    // initialize the controller
    bool UHCIController::Init(PCI::PCI_DEVICE* device)
    {
        // TODO: return error if none of the BARs are I/O address

        // enable bus matering
        device->configWrite<uint8_t>(0x4, 0x5);

        // disable USB all interrupts
        device->outw(UHCI_INTR, 0x0);

        globalResetController(device);

        // check if cmd register is default
        if(device->inw(UHCI_COMMAND) != 0x0) return false;
        // check if status register is default
        if(device->inw(UHCI_COMMAND) != 0x0) return false;

        // what magic??
        device->outw(UHCI_STATUS, 0x00FF);

        // store SOF
        uint8_t bSOF = device->inb(UHCI_SOF);

        // reset
        device->outw(UHCI_COMMAND, 0x2);

        // sleep for 42 ms
        PIT_sleep(42);

        if(device->inb(UHCI_COMMAND) & 0x2) return false;

        device->outb(UHCI_SOF, bSOF);

        // disable legacy keyboard/mouse support
        device->configWrite(UHCI_PCI_LEGACY, 0xAF00);

        return true;
    }

    // setup the controller
    void UHCIController::Setup(PCI::PCI_DEVICE* device)
    {
        ;
    }
}