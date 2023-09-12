#include "EHCI.hpp"

#include <std/stdio.h>

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
        uint32_t framListBaseAddress;
        uint32_t nextAsyncListAddress;
    }_packed;

    void init_ehci_device(PCI::FunctionInfo device)
    {
        void* memory_mapped_io = PCI::initializeBAR(device, 0);

        CapabilityRegister* capRegister = reinterpret_cast<CapabilityRegister*>(memory_mapped_io);
        OperationalRegister* opRegister = reinterpret_cast<OperationalRegister*>(memory_mapped_io);

        ;
    }
}