#pragma once

#include <includes.h>
#include <Bus/PCI/PCI.hpp>

namespace UHCI
{
    void init_uhci_device(PCI::PCI_DEVICE* device);
}
