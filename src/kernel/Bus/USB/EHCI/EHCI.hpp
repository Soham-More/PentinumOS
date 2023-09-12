#pragma once

#include <includes.h>
#include <Bus/PCI/PCI.hpp>

namespace EHCI
{
    void init_ehci_device(PCI::FunctionInfo device);
}
