#include "EHCI.hpp"

#include <std/stdio.h>
#include <Drivers/PIT.h>

namespace EHCI
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
        uint32_t command;
        uint32_t status;
        uint32_t intr;
        uint32_t frameIndex;
        uint32_t segementSelector4G;
        uint32_t periodicListBaseAddress;
        uint32_t asyncListAddress;
    }_packed;

    void init_ehci_device(PCI::FunctionInfo device)
    {
        uint32_t BAR0 = reinterpret_cast<uint32_t>(PCI::initializeBAR(device, 0));

        volatile CapabilityRegister* capRegister = reinterpret_cast<CapabilityRegister*>(BAR0);
        volatile OperationalRegister* opRegister = reinterpret_cast<OperationalRegister*>(BAR0 + capRegister->size);

        // stop the usb transaction
        opRegister->command &= ~1;

        PIT_sleep(1);

        ;
    }
}