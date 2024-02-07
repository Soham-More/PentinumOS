#pragma once

#include <includes.h>
#include <Bus/PCI/PCI.hpp>

// Naming rules:
//  types/structs: all small letters, seperated with _
//  classes: Camel Case
//  Macros: all CAPS
//  Macro functions: named like functions
//  Functions: namespace_camelCaseName()
//  variables: camelCase
//  

namespace USB
{
    struct uhci_td
    {
        uint32_t linkPointer;
        uint16_t actualLength;
        uint8_t  status;
        uint8_t  ctrl;
        uint32_t packetHeader;
        uint32_t bufferPointer;
        uint32_t r1;
        uint32_t r2;
        uint32_t r3;
        uint32_t r4;
    }_packed;

    struct uhci_device
    {
        bool isLowSpeedDevice;
        uint8_t address;
    };

    struct uhci_queue_head
    {
        uint32_t ptrHorizontal;
        uint32_t ptrVertical;

        uhci_queue_head* next;
        uhci_queue_head* vert;
    }_packed;

    class UHCIController
    {
        private:
            PCI::PCI_DEVICE* uhciController;

            uint16_t portCount;

            uint32_t* frameList;
            uhci_queue_head* uhciQueueHeads;

            bool isPortPresent(uint16_t portID);
            bool resetPort(uint16_t portID);
            void setupDevice(uint16_t portID);

            void insertToQueue(uhci_queue_head* qh, uint8_t queueID);
            void removeFromQueue(uhci_queue_head* qh, uint8_t queueID);

            uint8_t getTransferStatus(uhci_td* td, size_t count);

        public:
            UHCIController(PCI::PCI_DEVICE* device);

            bool Init();

            void Setup();

            bool sendControlIn(uhci_device device, void* buffer, uint8_t endpoint, uint8_t requestType, uint8_t request, uint16_t value, uint16_t index, uint16_t length, uint8_t packetSize);
    };
}
