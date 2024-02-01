#include "UHCI.hpp"

#include <std/stdio.h>
#include <Drivers/PIT.h>

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

    void init_uhci_device(PCI::PCI_DEVICE* device)
    {
        ;
    }
}