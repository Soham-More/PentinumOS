#pragma once

#include <includes.h>
#include "../defs.hpp"
#include "../../driver_defs.hpp"

#include <std/std.hpp>
#include <std/ds.hpp>
#include <hw/pci/pci.hpp>

namespace drivers::usb
{
    struct uhci_td
    {
        u32 linkPointer;
        u32 ctrlStatus;
        u32 packetHeader;
        u32 bufferPointer;
        u32 resv[4];
    }_packed;

    struct uhci_queue_head
    {
        u32 ptrHorizontal;
        u32 ptrVertical;

        uhci_queue_head* next;
        uhci_queue_head* vert;
    }_packed;

    class uhci_controller
    {
        private:
            bus::pci::device* uhciController;

            u32 device_count;

            u16 portCount;
            u8  addressCount;

            u32* frameList;
            uhci_queue_head* uhciQueueHeads;

            //std::vector<usb_device> connectedDevices;

            bool isPortPresent(u16 portID);
            bool resetPort(u16 portID);
            void setupDevice(vfs::vfs_t* gvfs, vfs::node_t* controller_node, u16 portID);

            void insertToQueue(uhci_queue_head* qh, u8 queueID);
            void removeFromQueue(uhci_queue_head* qh, u8 queueID);

            u8 getTransferStatus(volatile uhci_td* td, size_t count);
            u8 waitTillTransferComplete(uhci_td* td, size_t count);

            bool controlIn(usb_device device, void* buffer, u8 requestType, u8 request, u16 value, u16 index, u16 length, u8 packetSize);
            bool controlOut(usb_device device, u8 requestType, u8 request, u16 value, u16 index, u16 length, u8 packetSize);

        public:
            uhci_controller(bus::pci::device* device);

            bool Init();
            void Setup(vfs::vfs_t* gvfs, vfs::node_t* controller_node);

            bool resetDevice(usb_device& device);
            //std::vector<usb_device>& getAllDevices();

            bool controlIn(const usb_device& device, request_packet rpacket, void* buffer, u16 size);
            bool controlOut(const usb_device& device, request_packet rpacket, u16 size);

            bool bulkIn(const usb_device& device, u8 endpoint, u16 maxPacketSize, void* buffer, u16 size);
            bool bulkOut(const usb_device& device, u8 endpoint, u16 maxPacketSize, void* buffer, u16 size);

            // uhci_controller should be alive for
            // as long as usb_controller is.
            usb_controller as_generic_controller();
    };

    kernel_driver get_uhci_driver(vfs::vfs_t* gvfs);
}
