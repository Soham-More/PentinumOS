#include "UHCI.hpp"

#include <std/stdio.h>
#include <Drivers/PIT.h>
#include <i686/x86.h>
#include <std/Heap/heap.hpp>
#include <std/memory.h>
#include <System/Paging.hpp>
#include <std/logger.h>
#include <std/math.h>

#define GLOBAL_RESET_COUNT 5

#define UHCI_COMMAND 0x0
#define UHCI_STATUS  0x2
#define UHCI_INTR    0x4
#define UHCI_SOF     0xC
#define UHCI_FRNUM   0x6
#define UHCI_FRADDR  0x8

#define UHCI_TD 0b00
#define UHCI_QH 0b00

#define UHCI_Invalid  0b01
#define UHCI_Valid    0b00

#define UHCI_QControl     8
#define UHCI_QUEUE_COUNT 10

#define UHCI_PCI_LEGACY  0xC0

#define UHCI_PORT_BASE   0x10

// min wait time after reset
#define USB_TDRST       10
#define USB_TRSTRCY     10 

#define PORT_RESTART_TRIES 10

#define CTRL_ENDPOINT   0

#define PACKET_SETUP    0b11010000 + 0b0010
#define PACKET_IN       0b10010000 + 0b0110
#define PACKET_OUT      0b00010000 + 0b1110

#define REQ_GET_DESC 6

namespace USB
{
    struct request_packet
    {
        uint8_t  requestType;
        uint8_t  request;
        uint16_t value;
        uint16_t index;
        uint16_t size;
    }_packed;

    struct device_desc
    {
        uint8_t  length;
        uint8_t  descType;
        uint16_t bcdUSB;
        uint8_t  deviceClass;
        uint8_t  subClass;
        uint8_t  protocolCode;
        uint8_t  maxPacketSize;
        uint16_t vendorID;
        uint16_t productID;
        uint16_t bcdDevice;
        uint8_t  iManufacture;
        uint8_t  iProduct;
        uint8_t  iSerialNumber;
        uint8_t  configCount;
    }_packed;

    UHCIController::UHCIController(PCI::PCI_DEVICE* device) : uhciController(device) { }

    void globalResetController(PCI::PCI_DEVICE* device)
    {
        // reset controller
        for(int i = 0; i < GLOBAL_RESET_COUNT; i++)
        {
            device->outw(UHCI_COMMAND, 0x4);
            PIT_sleep(11);
            device->outw(UHCI_COMMAND, 0x0);
        }
    }

    // returns if a USB port is present
    bool UHCIController::isPortPresent(uint16_t portID)
    {
        // 7th bit must be on
        if((uhciController->inw(UHCI_PORT_BASE + portID) & 0x80) == 0) return false;

        // try to clear it, if it becomes off, this is not a USB port
        uhciController->outw(UHCI_PORT_BASE + portID, uhciController->inw(UHCI_PORT_BASE + portID) & ~0x80);
        if((uhciController->inw(UHCI_PORT_BASE + portID) & 0x80) == 0) return false;

        // try R/WC
        uhciController->outw(UHCI_PORT_BASE + portID, uhciController->inw(UHCI_PORT_BASE + portID) | 0x80);
        if((uhciController->inw(UHCI_PORT_BASE + portID) & 0x80) == 0) return false;

        // wtf??????
        uhciController->outw(UHCI_PORT_BASE + portID, uhciController->inw(UHCI_PORT_BASE + portID) | 0x0A);

        uint32_t response = uhciController->inw(UHCI_PORT_BASE + portID);

        if((uhciController->inw(UHCI_PORT_BASE + portID) & 0x0A) != 0) return false;

        return true;
    }

    // returns if a device is connected to USB port, and resets the device
    bool UHCIController::resetPort(uint16_t portID)
    {
        // set reset bit
        uhciController->outw(UHCI_PORT_BASE + portID, uhciController->inw(UHCI_PORT_BASE + portID) | (1 << 9));
        PIT_sleep(USB_TDRST);
        // clear reset bit
        uhciController->outw(UHCI_PORT_BASE + portID, uhciController->inw(UHCI_PORT_BASE + portID) & ~(1 << 9));

        for(int i = 0; i < PORT_RESTART_TRIES; i++)
        {
            PIT_sleep(USB_TRSTRCY);

            // no devices connected
            if((uhciController->inw(UHCI_PORT_BASE + portID) & 1) == 0) return false;

            // if connect/enable change
            if(uhciController->inw(UHCI_PORT_BASE + portID) & 0b1010)
            {
                uhciController->outw(UHCI_PORT_BASE + portID, uhciController->inw(UHCI_PORT_BASE + portID) & 0x124E);
                continue;
            }

            // check the enable bit
            if(uhciController->inw(UHCI_PORT_BASE) & (1 << 2))
            {
                // enabled
                return true;
            }

            // set enabled
            uhciController->outw(UHCI_PORT_BASE + portID, uhciController->inw(UHCI_PORT_BASE + portID) | (1 << 2));
        }

        return false;
    }

    // insert QH to queue indicated by queueID
    void UHCIController::insertToQueue(uhci_queue_head* qh, uint8_t queueID)
    {
        // first time
        if((uhciQueueHeads[queueID].ptrVertical & UHCI_Invalid) == UHCI_Invalid)
        {
            uhciQueueHeads[queueID].ptrVertical = sys::getPhysicalLocation(qh) | UHCI_QH;
            uhciQueueHeads[queueID].vert = qh;
            qh->next = nullptr;
            qh->vert = &uhciQueueHeads[queueID];
            qh->ptrHorizontal = UHCI_Invalid;
        }
        else
        {
            uhci_queue_head* lastQH = uhciQueueHeads[queueID].vert;

            // get the last queue head.
            while(lastQH != nullptr) lastQH = lastQH->next;

            lastQH->next = qh;
            lastQH->ptrHorizontal = sys::getPhysicalLocation(qh) | UHCI_QH;

            qh->next = nullptr;
            qh->vert = nullptr;
            qh->ptrHorizontal = UHCI_Invalid;
        }
    }

    // remove queue head from queue
    void UHCIController::removeFromQueue(uhci_queue_head* qh, uint8_t queueID)
    {
        // if vert points to parent queue header, then change queue header pointer.
        if(qh->vert != nullptr)
        {
            qh->vert->ptrVertical = qh->ptrHorizontal;
            qh->vert->vert = qh->next;

            qh->next = nullptr;
            qh->vert = nullptr;
        }
        else
        {
            uhci_queue_head* prevQH = uhciQueueHeads[queueID].vert;

            // get QH before qh
            while(prevQH->next != qh) prevQH = prevQH->next;

            prevQH->next = qh->next;
            prevQH->ptrHorizontal = qh->ptrHorizontal;

            qh->next = nullptr;
            qh->vert = nullptr;
        }
    }

    uint8_t UHCIController::getTransferStatus(uhci_td* td, size_t count)
    {
        for(int i = 0; i < count; i++)
        {
            if(td[i].status != 0)
            {
                if((td[i].status & (1 << 3) == (1 << 3))) return 2;
                if((td[i].status & (1 << 7) == (1 << 7))) return 3;
            }
        }

        return 0;
    }

    void UHCIController::setupDevice(uint16_t portID)
    {
        uint16_t portsc = uhciController->inw(UHCI_PORT_BASE + portID);

        uhci_device device;
        device.isLowSpeedDevice = (portsc & (1 << 8)) ? 1 : 0;
        device.address = 0;

        device_desc deviceDesc;

        bool status = sendControlIn(device, &deviceDesc, 0, 0x80, REQ_GET_DESC, (1 << 8) | 0, 0, 18, 8);

        ;
    }

    // initialize the controller
    bool UHCIController::Init()
    {
        // TODO: return error if none of the BARs are I/O address

        // enable bus matering
        uhciController->configWrite<uint8_t>(0x4, 0x5);

        // disable USB all interrupts
        uhciController->outw(UHCI_INTR, 0x0);

        globalResetController(uhciController);

        // check if cmd register is default
        if(uhciController->inw(UHCI_COMMAND) != 0x0) return false;
        // check if status register is default
        if(uhciController->inw(UHCI_COMMAND) != 0x0) return false;

        // what magic??
        uhciController->outw(UHCI_STATUS, 0x00FF);

        // store SOF
        uint8_t bSOF = uhciController->inb(UHCI_SOF);

        // reset
        uhciController->outw(UHCI_COMMAND, 0x2);

        // sleep for 42 ms
        PIT_sleep(42);

        if(uhciController->inb(UHCI_COMMAND) & 0x2) return false;

        uhciController->outb(UHCI_SOF, bSOF);

        // disable legacy keyboard/mouse support
        uhciController->configWrite(UHCI_PCI_LEGACY, 0xAF00);

        return true;
    }

    // setup the controller
    void UHCIController::Setup()
    {
        // disable all interrupts
        uhciController->outw(UHCI_INTR, 0b0000);

        // reset frame num register to 0
        uhciController->outw(UHCI_FRNUM, 0);

        this->frameList = (uint32_t*)std::mallocAligned(1024 * sizeof(uint32_t), 4096);
        ptr_t frameListPhys = sys::getPhysicalLocation(this->frameList);

        memset(this->frameList, 0, 1024 * sizeof(uint32_t));

        // allocate Queue Heads
        this->uhciQueueHeads = (uhci_queue_head*)std::mallocAligned(UHCI_QUEUE_COUNT * sizeof(uhci_queue_head), 16);
        ptr_t queueHeadPhy = sys::getPhysicalLocation(this->uhciQueueHeads);

        memset(this->uhciQueueHeads, 0, UHCI_QUEUE_COUNT * sizeof(uhci_queue_head));

        // No TDs, this driver does not implement isochronous transfers

        for(size_t i = 0; i < UHCI_QUEUE_COUNT - 1; i++)
        {
            // point to next QH
            uhciQueueHeads[i].ptrHorizontal = queueHeadPhy + ((i + 1) * sizeof(uhci_queue_head));
            // mark as QH pointer
            uhciQueueHeads[i].ptrHorizontal |= UHCI_QH;
            // invalidate the vertical pointer
            uhciQueueHeads[i].ptrVertical = UHCI_Invalid;
        }
        uhciQueueHeads[UHCI_QUEUE_COUNT - 1].ptrHorizontal = UHCI_Invalid;
        uhciQueueHeads[UHCI_QUEUE_COUNT - 1].ptrVertical = UHCI_Invalid;

        // setup the schedule, give 50% time to bulk and control.
        for(int i = 0; i < 1024; i++)
        {
            int queueStart = 7;
            if((i + 1) % 2 == 0) queueStart--;
            if((i + 1) % 4 == 0) queueStart--;
            if((i + 1) % 8 == 0) queueStart--;
            if((i + 1) % 16 == 0) queueStart--;
            if((i + 1) % 32 == 0) queueStart--;
            if((i + 1) % 64 == 0) queueStart--;
            if((i + 1) % 128 == 0) queueStart--;

            this->frameList[i] = (queueHeadPhy + queueStart * sizeof(uhci_queue_head)) | UHCI_QH;
        }

        // set frame list pointer
        uhciController->outl(UHCI_FRADDR, frameListPhys);

        // clear status register
        uhciController->outw(UHCI_STATUS, 0xFFFF);
        // set the USB command register.
        uhciController->outl(UHCI_STATUS, (1 << 7) | (1 << 6) | (1 << 0));

        uint16_t port = 0;
        while(isPortPresent(port))
        {
            portCount++;

            if(resetPort(port))
            {
                log_info("[UHCI] Found USB 1.0 Device at port %x\n", port);

                setupDevice(port);
            }

            // try next port
            port += 2;
        }
    }

    bool UHCIController::sendControlIn(uhci_device device, void* buffer, uint8_t endpoint, uint8_t requestType, uint8_t request, uint16_t value, uint16_t index, uint16_t length, uint8_t packetSize)
    {
        request_packet* rpacket = (request_packet*)std::mallocAligned(sizeof(request_packet), 16);
        rpacket->requestType = requestType;
        rpacket->request = request;
        rpacket->index = index;
        rpacket->value = value;
        rpacket->size = length;

        uint16_t td_count = DivRoundUp(length, packetSize) + 2;
        uhci_td* td_in = (uhci_td*)std::mallocAligned(td_count * sizeof(uhci_td), 16);
        memset(td_in, 0, td_count * sizeof(uhci_td));

        uhci_queue_head* qh = (uhci_queue_head*)std::mallocAligned(sizeof(qh), 16);
        memset(qh, 0, sizeof(uhci_queue_head));
        qh->ptrVertical = sys::getPhysicalLocation(td_in);

        // the first TD describes the control packet
        td_in[0].linkPointer = sys::getPhysicalLocation(&td_in[1]);
        td_in[0].ctrl = (device.isLowSpeedDevice ? (1 << 2) : 0);
        td_in[0].status = (1 << 7);
        td_in[0].packetHeader = ((sizeof(rpacket) - 1) << 21) | (CTRL_ENDPOINT << 15) | (device.address << 8) | PACKET_SETUP; // DATA0
        td_in[0].bufferPointer = sys::getPhysicalLocation(rpacket);

        uint16_t sz = length;

        for(int i = 1; (i < td_count - 1) && sz > 0; i++)
        {
            uint16_t tokenSize = sz < packetSize ? sz : packetSize;

            td_in[i].linkPointer = sys::getPhysicalLocation(&td_in[i + 1]);
            td_in[i].ctrl = (device.isLowSpeedDevice ? (1 << 2) : 0);
            td_in[i].status = (1 << 7);
            td_in[i].packetHeader = ((tokenSize - 1) << 21) | (CTRL_ENDPOINT << 15) | ((i & 1) << 19) | (device.address << 8) | PACKET_IN;
            td_in[i].bufferPointer = sys::getPhysicalLocation(buffer) + i * packetSize;

            sz -= tokenSize;
        }

        td_in[td_count-1].linkPointer = UHCI_Invalid;
        td_in[td_count-1].ctrl = (device.isLowSpeedDevice ? (1 << 2) : 0);
        td_in[td_count-1].status = (1 << 7);
        td_in[td_count-1].packetHeader = (0x7FF << 21) | (CTRL_ENDPOINT << 15) | (device.address << 8) | PACKET_OUT; // DATA1
        td_in[td_count-1].bufferPointer = nullptr;

        insertToQueue(qh, UHCI_QControl);

        uint16_t timeout = 1000;
        uint8_t status = 3;
        while(status == 3 && timeout > 0)
        {
            timeout -= 10;
            PIT_sleep(10);
            status = getTransferStatus(td_in, td_count);
        }

        removeFromQueue(qh, UHCI_QControl);

        std::free(rpacket);
        std::free(td_in);
        std::free(qh);

        return status == 0;
    }
}