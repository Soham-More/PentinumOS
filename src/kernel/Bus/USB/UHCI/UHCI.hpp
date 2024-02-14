#pragma once

#include <includes.h>
#include <Bus/PCI/PCI.hpp>
#include <Bus/USB/USBController.hpp>

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

    struct uhci_queue_head
    {
        uint32_t ptrHorizontal;
        uint32_t ptrVertical;

        uhci_queue_head* next;
        uhci_queue_head* vert;
    }_packed;

    class UHCIController : public USBController
    {
        private:
            PCI::PCI_DEVICE* uhciController;

            uint16_t portCount;
            uint8_t  addressCount;

            uint32_t* frameList;
            uhci_queue_head* uhciQueueHeads;

            std::vector<usb_device> connectedDevices;

            bool isPortPresent(uint16_t portID);
            bool resetPort(uint16_t portID);
            void setupDevice(uint16_t portID);

            void insertToQueue(uhci_queue_head* qh, uint8_t queueID);
            void removeFromQueue(uhci_queue_head* qh, uint8_t queueID);

            uint8_t getTransferStatus(uhci_td* td, size_t count);
            uint8_t waitTillTransferComplete(uhci_td* td, size_t count);

            bool controlIn(usb_device device, void* buffer, uint8_t requestType, uint8_t request, uint16_t value, uint16_t index, uint16_t length, uint8_t packetSize);
            bool controlOut(usb_device device, uint8_t requestType, uint8_t request, uint16_t value, uint16_t index, uint16_t length, uint8_t packetSize);

        public:
            UHCIController(PCI::PCI_DEVICE* device);

            bool Init();
            void Setup();

            bool resetDevice(usb_device& device);
            const std::vector<usb_device>& getAllDevices();

            bool controlIn(const usb_device& device, request_packet rpacket, void* buffer, uint16_t size);
            bool controlOut(const usb_device& device, request_packet rpacket, uint16_t size);

            bool bulkIn(const usb_device& device, uint8_t endpoint, uint16_t maxPacketSize, void* buffer, uint16_t size);
            bool bulkOut(const usb_device& device, uint8_t endpoint, uint16_t maxPacketSize, void* buffer, uint16_t size);
    };
}
