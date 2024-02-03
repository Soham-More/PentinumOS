#pragma once

#include <includes.h>
#include <Bus/PCI/PCI.hpp>

namespace UHCI
{
    bool init_uhci_controller(PCI::PCI_DEVICE* device);
}
