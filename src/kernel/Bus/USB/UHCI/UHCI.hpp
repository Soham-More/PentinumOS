#pragma once

#include <includes.h>
#include <Bus/PCI/PCI.hpp>

namespace USB
{
    class UHCIController
    {
        private:
            PCI::PCI_DEVICE* uhciController;

        public:
            UHCIController(PCI::PCI_DEVICE* device);

            bool Init(PCI::PCI_DEVICE* device);

            void Setup(PCI::PCI_DEVICE* device);
    };
}
