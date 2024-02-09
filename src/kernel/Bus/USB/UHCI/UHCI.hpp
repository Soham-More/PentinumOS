#pragma once

#include <includes.h>
#include <Bus/PCI/PCI.hpp>
#include <std/vector.hpp>
#include <USB/usbdefs.hpp>

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
        uint32_t ctrlStatus;
        uint32_t packetHeader;
        uint32_t bufferPointer;
        uint32_t resv[4];
    }_packed;

    struct uhci_device
    {
        bool     isLowSpeedDevice;
        uint16_t portAddress;
        uint8_t  address;
        uint8_t  subClass;
        uint8_t  protocolCode;
        uint8_t  maxPacketSize;
        uint16_t vendorID;
        uint16_t productID;
        uint16_t bcdDevice;
        uint8_t  configCount;
        char* manufactureName;
        char* productName;
        char* serialNumber;
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
            uint8_t  addressCount;

            uint32_t* frameList;
            uhci_queue_head* uhciQueueHeads;

            std::vector<uhci_device> connectedDevices;

            bool isPortPresent(uint16_t portID);
            bool resetPort(uint16_t portID);
            void setupDevice(uint16_t portID);

            void insertToQueue(uhci_queue_head* qh, uint8_t queueID);
            void removeFromQueue(uhci_queue_head* qh, uint8_t queueID);

            uint8_t getTransferStatus(uhci_td* td, size_t count);
            uint8_t waitTillTransferComplete(uhci_td* td, size_t count);

            bool controlIn(uhci_device device, void* buffer, uint8_t endpoint, uint8_t requestType, uint8_t request, uint16_t value, uint16_t index, uint16_t length, uint8_t packetSize);
            bool controlOut(uhci_device device, uint8_t endpoint, uint8_t requestType, uint8_t request, uint16_t value, uint16_t index, uint16_t length, uint8_t packetSize);

        public:
            UHCIController(PCI::PCI_DEVICE* device);

            bool Init();
            void Setup();

            bool resetDevice(uhci_device& device);
            const std::vector<uhci_device>& getAllDevices();

            bool controlIn(uhci_device& device, request_packet rpacket, uint8_t endpoint, void* buffer, uint16_t size);
            bool controlOut(uhci_device& device, request_packet rpacket, uint8_t endpoint, uint16_t size);

            bool bulkIn(uhci_device& device, uint8_t endpoint, void* buffer, uint16_t size);
            bool bulkOut(uhci_device& device, uint8_t endpoint, void* buffer, uint16_t size);

            //bool BulkIn(uhci_device device, void* buffer, uint8_t endpoint, uint8_t requestType, uint8_t request, uint16_t value, uint16_t index, uint16_t length, uint8_t packetSize);
            //bool BulkOut(uhci_device device, uint8_t endpoint, uint8_t requestType, uint8_t request, uint16_t value, uint16_t index, uint16_t length, uint8_t packetSize);
    };
}
