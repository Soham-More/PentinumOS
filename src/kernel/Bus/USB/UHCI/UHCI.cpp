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
#define UHCI_PCI_LEGACY  0xC0
#define UHCI_PORT_BASE   0x10

#define UHCI_TD 0b00
#define UHCI_QH 0b10
#define UHCI_Invalid  0b01
#define UHCI_Valid    0b00

#define UHCI_QControl     8
#define UHCI_QBulk        9
#define UHCI_QUEUE_COUNT 10

#define PORT_RESTART_TRIES 10

namespace USB
{
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

    void printWideString(char* string, uint8_t byteLength)
    {
        for(int i = 0; i < (byteLength / 2); i++)
        {
            putc(string[i * 2]);
        }
    }
    void copyWideStringToString(void* src, void* dst, size_t srcByteLength)
    {
        uint16_t* in = (uint16_t*)src;
        uint8_t* out = (uint8_t*)dst;

        for(int i = 0; i < (srcByteLength / 2); i++)
        {
            out[i] = in[i];
        }
        out[srcByteLength / 2] = 0;
    }

    void globalResetController(PCI::PCI_DEVICE* device)
    {
        // reset controller
        for(int i = 0; i < GLOBAL_RESET_COUNT; i++)
        {
            device->outw(UHCI_COMMAND, 0x4);
            PIT_sleep(15);
            device->outw(UHCI_COMMAND, 0x0);
        }
    }

    UHCIController::UHCIController(PCI::PCI_DEVICE* device) : uhciController(device) { }

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
            uint8_t status = (td[i].ctrlStatus >> 16) & 0xFF;

            if(status != 0)
            {
                if((status & (1 << 3)) == (1 << 3)) return 2;
                if((status & (1 << 7)) == (1 << 7)) return 3;

                return 1; // ERROR occured
            }
        }

        return 0;
    }
    uint8_t UHCIController::waitTillTransferComplete(uhci_td* td, size_t count)
    {
        uint16_t timeout = 1000;
        uint8_t status = 3;
        while(status == 3 && timeout > 0)
        {
            timeout -= 10;
            PIT_sleep(10);
            status = getTransferStatus(td, count);
        }

        return status;
    }

    void UHCIController::setupDevice(uint16_t portID)
    {
        uint16_t portsc = uhciController->inw(UHCI_PORT_BASE + portID);

        usb_device device;
        device.isLowSpeedDevice = (portsc & (1 << 8)) ? 1 : 0;
        device.address = 0;

        device_desc deviceDesc;

        char mf[STR_MAX_SIZE];
        char prod[STR_MAX_SIZE];
        char srn[STR_MAX_SIZE];

        device.portAddress = UHCI_PORT_BASE + portID;

        uint8_t requestType = USB_DEVICE_TO_HOST | USB_REQ_STRD | USB_REP_DEVICE;

        if(controlIn(device, &deviceDesc, requestType, REQ_GET_DESC, (1 << 8) | 0, 0, 18, 8))
        {
            uint16_t LANGID = 0x0409; // English(US)

            bool status = controlIn(device, mf, requestType, REQ_GET_DESC, (3 << 8) | deviceDesc.iManufacture, LANGID, STR_MAX_SIZE, deviceDesc.maxPacketSize);
            status &= controlIn(device, prod, requestType, REQ_GET_DESC, (3 << 8) | deviceDesc.iProduct, LANGID, STR_MAX_SIZE, deviceDesc.maxPacketSize);
            status &= controlIn(device, srn, requestType, REQ_GET_DESC, (3 << 8) | deviceDesc.iSerialNumber, LANGID, STR_MAX_SIZE, deviceDesc.maxPacketSize);

            if(!status) log_warn("[UHCI][DeviceSetup] Failed to retrieve device info.\n");
            else
            {
                device.manufactureName = (char*)std::malloc(mf[0] - 1); memset(device.manufactureName, 0, mf[0] - 1);
                device.productName = (char*)std::malloc(prod[0] - 1); memset(device.productName, 0, prod[0] - 1);
                device.serialNumber = (char*)std::malloc(srn[0] - 1); memset(device.serialNumber, 0, srn[0] - 1);

                copyWideStringToString((char*)mf + 2, device.manufactureName, mf[0] - 2);
                copyWideStringToString((char*)prod + 2, device.productName, prod[0] - 2);
                copyWideStringToString((char*)srn + 2, device.serialNumber, srn[0] - 2);
            }

            device.bcdDevice = deviceDesc.bcdDevice;
            device.configCount = deviceDesc.configCount;
            device.productID = deviceDesc.productID;
            device.protocolCode = deviceDesc.protocolCode;
            device.maxPacketSize = deviceDesc.maxPacketSize;
            device.subClass = deviceDesc.subClass;
            device.vendorID = deviceDesc.vendorID;

            device.address = 0;

            uint8_t setAddrReqType = USB_HOST_TO_DEVICE | USB_REQ_STRD | USB_REP_DEVICE;

            // reset the device again.
            resetPort(portID);

            if(controlOut(device, setAddrReqType, REQ_SET_ADDR, addressCount, 0, 0, device.maxPacketSize))
            {
                device.address = addressCount; addressCount++;

                connectedDevices.push_back(device);
            }
            else
            {
                log_warn("[UHCI][DeviceSetup] Failed to set USB address.\n");
            }
        }
        else
        {
            log_warn("[UHCI][DeviceSetup] Failed to get USB descriptor.\n");
        }
    }
    const std::vector<usb_device>& UHCIController::getAllDevices()
    {
        return this->connectedDevices;
    }

    // initialize the controller
    bool UHCIController::Init()
    {
        // TODO: return error if none of the BARs are I/O address

        // enable bus matering
        uhciController->configWrite<uint8_t>(0x4, 0b101);

        uint32_t addr = uhciController->configRead<uint32_t>(0x20);

        // disable USB all interrupts
        uhciController->outw(UHCI_INTR, 0x0);

        globalResetController(uhciController);

        // check if cmd register is default
        if(uhciController->inw(UHCI_COMMAND) != 0x0) return false;
        // check if status register is default
        if(uhciController->inw(UHCI_STATUS) != 0x20) return false;

        // what magic??
        uhciController->outw(UHCI_STATUS, 0x00FF);

        // store SOF
        uint8_t bSOF = uhciController->inb(UHCI_SOF);

        // reset
        uhciController->outw(UHCI_COMMAND, 0x2);

        // sleep for 42 ms
        PIT_sleep(45);

        if(uhciController->inb(UHCI_COMMAND) & 0x2) return false;

        uhciController->outb(UHCI_SOF, bSOF);

        // disable legacy keyboard/mouse support
        uhciController->configWrite<uint16_t>(UHCI_PCI_LEGACY, 0xAF00);

        return true;
    }
    // setup the controller
    void UHCIController::Setup()
    {
        // disable all interrupts
        uhciController->outw(UHCI_INTR, 0b1111);

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

        uint32_t fraddr = uhciController->inl(UHCI_FRADDR);
        uint32_t frnum = uhciController->inl(UHCI_FRNUM);

        // clear status register
        uhciController->outw(UHCI_STATUS, 0xFFFF);
        // set the USB command register.
        uhciController->outw(UHCI_COMMAND, (1 << 7) | (1 << 6) | (1 << 0));

        PIT_sleep(10);

        uint32_t cmdReg = uhciController->inw(UHCI_COMMAND);

        // address of 0 is reserved.
        addressCount = 1;

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
    bool UHCIController::resetDevice(usb_device& device)
    {
        return resetPort(device.portAddress);
    }

    bool UHCIController::controlIn(usb_device device, void* buffer, uint8_t requestType, uint8_t request, uint16_t value, uint16_t index, uint16_t length, uint8_t packetSize)
    {
        request_packet* rpacket = (request_packet*)std::mallocAligned(sizeof(request_packet), 16);
        rpacket->requestType = requestType;
        rpacket->request = request;
        rpacket->index = index;
        rpacket->value = value;
        rpacket->size = length;

        // return buffer
        uint8_t* retBuffer = (uint8_t*)std::malloc(length);
        memset(retBuffer, 0, length);

        uint16_t td_count = DivRoundUp(length, packetSize) + 2;
        uhci_td* td_in = (uhci_td*)std::mallocAligned(td_count * sizeof(uhci_td), 16);
        memset(td_in, 0, td_count * sizeof(uhci_td));

        uhci_queue_head* qh = (uhci_queue_head*)std::mallocAligned(sizeof(qh), 16);
        memset(qh, 0, sizeof(uhci_queue_head));
        qh->ptrVertical = sys::getPhysicalLocation(td_in);

        // the first TD describes the control packet
        td_in[0].linkPointer = sys::getPhysicalLocation(&td_in[1]);
        td_in[0].ctrlStatus = (device.isLowSpeedDevice ? (1 << 26) : 0) | (1 << 23);
        td_in[0].packetHeader = ((sizeof(request_packet) - 1) << 21) | (CTRL_ENDPOINT << 15) | (device.address << 8) | PACKET_SETUP; // DATA0
        td_in[0].bufferPointer = sys::getPhysicalLocation(rpacket);

        uint16_t sz = length;

        for(int i = 1; (i < td_count - 1); i++)
        {
            uint16_t tokenSize = sz < packetSize ? sz : packetSize;

            td_in[i].linkPointer = sys::getPhysicalLocation(&td_in[i + 1]);
            td_in[i].ctrlStatus = (device.isLowSpeedDevice ? (1 << 26) : 0) | (1 << 23);
            td_in[i].packetHeader = ((tokenSize - 1) << 21) | (CTRL_ENDPOINT << 15) | ((i & 1) ? (1 << 19) : 0) | (device.address << 8) | PACKET_IN;
            td_in[i].bufferPointer = sys::getPhysicalLocation(retBuffer) + (i - 1) * packetSize;

            sz -= tokenSize;
        }

        td_in[td_count-1].linkPointer = UHCI_Invalid;
        td_in[td_count-1].ctrlStatus = (device.isLowSpeedDevice ? (1 << 26) : 0) | (1 << 23);
        td_in[td_count-1].packetHeader = (0x7FF << 21) | (1 << 19) | (CTRL_ENDPOINT << 15) | (device.address << 8) | PACKET_OUT; // DATA1
        td_in[td_count-1].bufferPointer = nullptr;

        insertToQueue(qh, UHCI_QControl);

        uint8_t status = waitTillTransferComplete(td_in, td_count);

        removeFromQueue(qh, UHCI_QControl);

        memcpy(retBuffer, buffer, length);

        if(status != 0)
        {
            switch (status)
            {
            case 0x3:
                log_warn("[UHCI][ControlIn] USB device timed out. Info: \n");
                break;
            case 0x2:
                log_warn("[UHCI][ControlIn] USB device sent NAK. Info: \n");
                break;
            case 0x1:
                log_warn("[UHCI][ControlIn] ERROR USB device. Info: ");
                break;
            default:
                break;
            }
            log_warn("\tPort: %x\n", device.portAddress);
            log_warn("\tAddress: %x\n", device.address);
            log_warn("\tSpeed: %s\n", device.isLowSpeedDevice ? "Low Speed" : "Full Speed");
            log_warn("\tRequest: %x\n", request);
            log_warn("\tRequest Type: %x\n", requestType);
            log_warn("\tValue: %x\n", value);
            log_warn("\tIndex: %x\n", index);
            log_warn("\tMax Packet Size: %x\n", packetSize);
            log_warn("\tRequested Length: %x\n", length);
        }

        std::free(retBuffer);
        std::free(rpacket);
        std::free(td_in);
        std::free(qh);

        return status == 0;
    }
    bool UHCIController::controlOut(usb_device device, uint8_t requestType, uint8_t request, uint16_t value, uint16_t index, uint16_t length, uint8_t packetSize)
    {
        request_packet* rpacket = (request_packet*)std::mallocAligned(sizeof(request_packet), 16);
        rpacket->requestType = requestType;
        rpacket->request = request;
        rpacket->index = index;
        rpacket->value = value;
        rpacket->size = length;

        uhci_td* td_in = (uhci_td*)std::mallocAligned(2 * sizeof(uhci_td), 16);
        memset(td_in, 0, 2 * sizeof(uhci_td));

        uhci_queue_head* qh = (uhci_queue_head*)std::mallocAligned(sizeof(qh), 16);
        memset(qh, 0, sizeof(uhci_queue_head));
        qh->ptrVertical = sys::getPhysicalLocation(td_in);

        // the first TD describes the control packet
        td_in[0].linkPointer = sys::getPhysicalLocation(&td_in[1]);
        td_in[0].ctrlStatus = (device.isLowSpeedDevice ? (1 << 26) : 0) | (1 << 23);
        td_in[0].packetHeader = ((sizeof(request_packet) - 1) << 21) | (CTRL_ENDPOINT << 15) | (device.address << 8) | PACKET_SETUP; // DATA0
        td_in[0].bufferPointer = sys::getPhysicalLocation(rpacket);

        // last TD, status
        td_in[1].linkPointer = UHCI_Invalid;
        td_in[1].ctrlStatus = (device.isLowSpeedDevice ? (1 << 26) : 0) | (1 << 23);
        td_in[1].packetHeader = (0x7FF << 21) | DATA1 | (CTRL_ENDPOINT << 15) | (device.address << 8) | PACKET_IN; // DATA1
        td_in[1].bufferPointer = nullptr;

        insertToQueue(qh, UHCI_QControl);

        uint8_t status = waitTillTransferComplete(td_in, 2);

        removeFromQueue(qh, UHCI_QControl);

        if(status != 0)
        {
            switch (status)
            {
            case 0x3:
                log_warn("[UHCI][ControlOut] USB device timed out. Info: ");
                break;
            case 0x2:
                log_warn("[UHCI][ControlOut] USB device sent NAK. Info: ");
                break;
            case 0x1:
                log_warn("[UHCI][ControlOut] ERROR USB device. Info: ");
                break;
            default:
                break;
            }
            
            log_warn("\tPort: %x\n", device.portAddress);
            log_warn("\tAddress: %x\n", device.address);
            log_warn("\tSpeed: %s\n", device.isLowSpeedDevice ? "Low Speed" : "Full Speed");
            log_warn("\tRequest: %x\n", request);
            log_warn("\tRequest Type: %x\n", requestType);
            log_warn("\tValue: %x\n", value);
            log_warn("\tIndex: %x\n", index);
            log_warn("\tMax Packet Size: %x\n", packetSize);
            log_warn("\tRequested Length: %x\n", length);
        }

        std::free(rpacket);
        std::free(td_in);
        std::free(qh);

        return status == 0;
    }

    bool UHCIController::bulkIn(const usb_device& device, uint8_t endpoint, void* buffer, uint16_t size)
    {
        void* returnBuffer = std::malloc(size);

        uint16_t td_count = DivRoundUp(size, device.maxPacketSize);
        uhci_td* td_in = (uhci_td*)std::mallocAligned(td_count * sizeof(uhci_td), 16);
        memset(td_in, 0, td_count * sizeof(uhci_td));

        uhci_queue_head* qh = (uhci_queue_head*)std::mallocAligned(sizeof(qh), 16);
        memset(qh, 0, sizeof(uhci_queue_head));
        qh->ptrVertical = sys::getPhysicalLocation(td_in);
        qh->ptrHorizontal = UHCI_Invalid;

        uint16_t sz = size;

        for(int i = 0; i < td_count; i++)
        {
            uint16_t tokenSize = sz < device.maxPacketSize ? sz : device.maxPacketSize;

            td_in[i].linkPointer = sys::getPhysicalLocation(&td_in[i + 1]);
            if(i == td_count - 1) td_in[i].linkPointer = UHCI_Invalid;

            td_in[i].ctrlStatus = (device.isLowSpeedDevice ? (1 << 26) : 0) | (1 << 23);
            td_in[i].packetHeader = ((tokenSize - 1) << 21) | (endpoint << 15) | ((i & 1) ? (1 << 19) : 0) | (device.address << 8) | PACKET_IN;
            td_in[i].bufferPointer = sys::getPhysicalLocation(returnBuffer) + i * device.maxPacketSize;

            sz -= tokenSize;
        }

        insertToQueue(qh, UHCI_QControl);
        uint8_t status = waitTillTransferComplete(td_in, td_count);
        removeFromQueue(qh, UHCI_QControl);

        if(status != 0)
        {
            switch (status)
            {
            case 0x3:
                log_warn("[UHCI][ControlIn] USB device timed out. Info: \n");
                break;
            case 0x2:
                log_warn("[UHCI][ControlIn] USB device sent NAK. Info: \n");
                break;
            case 0x1:
                log_warn("[UHCI][ControlIn] ERROR USB device. Info: ");
                break;
            default:
                break;
            }
            log_warn("\tPort: %x\n", device.portAddress);
            log_warn("\tAddress: %x\n", device.address);
            log_warn("\tEndpoint: %x\n", endpoint);
            log_warn("\tSpeed: %s\n", device.isLowSpeedDevice ? "Low Speed" : "Full Speed");
            log_warn("\tMax Packet Size: %x\n", device.maxPacketSize);
            log_warn("\tSent Length: %x\n", size);
        }

        if(status == 0) memcpy(returnBuffer, buffer, size);

        std::free(returnBuffer);
        std::free(td_in);
        std::free(qh);

        return status == 0;
    }
    bool UHCIController::bulkOut(const usb_device& device, uint8_t endpoint, void* buffer, uint16_t size)
    {
        uint16_t td_count = DivRoundUp(size, device.maxPacketSize);
        uhci_td* td_in = (uhci_td*)std::mallocAligned(td_count * sizeof(uhci_td), 16);
        memset(td_in, 0, td_count * sizeof(uhci_td));

        uhci_queue_head* qh = (uhci_queue_head*)std::mallocAligned(sizeof(qh), 16);
        memset(qh, 0, sizeof(uhci_queue_head));
        qh->ptrVertical = sys::getPhysicalLocation(td_in);
        qh->ptrHorizontal = UHCI_Invalid;

        uint16_t sz = size;

        for(int i = 0; i < td_count; i++)
        {
            uint16_t tokenSize = sz < device.maxPacketSize ? sz : device.maxPacketSize;

            td_in[i].linkPointer = sys::getPhysicalLocation(&td_in[i + 1]);
            if(i == td_count - 1) td_in[i].linkPointer = UHCI_Invalid;

            td_in[i].ctrlStatus = (device.isLowSpeedDevice ? (1 << 26) : 0) | (1 << 23);
            td_in[i].packetHeader = ((tokenSize - 1) << 21) | (endpoint << 15) | ((i & 1) ? (1 << 19) : 0) | (device.address << 8) | PACKET_OUT;
            td_in[i].bufferPointer = sys::getPhysicalLocation(buffer) + i * device.maxPacketSize;

            sz -= tokenSize;
        }

        insertToQueue(qh, UHCI_QControl);
        uint8_t status = waitTillTransferComplete(td_in, td_count);
        removeFromQueue(qh, UHCI_QControl);

        if(status != 0)
        {
            switch (status)
            {
            case 0x3:
                log_warn("[UHCI][ControlIn] USB device timed out. Info: \n");
                break;
            case 0x2:
                log_warn("[UHCI][ControlIn] USB device sent NAK. Info: \n");
                break;
            case 0x1:
                log_warn("[UHCI][ControlIn] ERROR USB device. Info: ");
                break;
            default:
                break;
            }
            log_warn("\tPort: %x\n", device.portAddress);
            log_warn("\tAddress: %x\n", device.address);
            log_warn("\tEndpoint: %x\n", endpoint);
            log_warn("\tSpeed: %s\n", device.isLowSpeedDevice ? "Low Speed" : "Full Speed");
            log_warn("\tMax Packet Size: %x\n", device.maxPacketSize);
            log_warn("\tSent Length: %x\n", size);
        }

        std::free(td_in);
        std::free(qh);

        return status == 0;
    }

    bool UHCIController::controlIn(const usb_device& device, request_packet rpacket, void* buffer, uint16_t size)
    {
        return controlIn(device, buffer, rpacket.requestType, rpacket.request, rpacket.value, rpacket.index, size, device.maxPacketSize);
    }
    bool UHCIController::controlOut(const usb_device& device, request_packet rpacket, uint16_t size)
    {
        return controlOut(device, rpacket.requestType, rpacket.request, rpacket.value, rpacket.index, size, device.maxPacketSize);
    }
}