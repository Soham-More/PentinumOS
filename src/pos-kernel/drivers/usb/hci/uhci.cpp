#include "uhci.hpp"

#include <c/math.h>
#include <io/io.h>
#include <hw/pit/PIT.h>
#include <arch/x86.h>
#include <std/std.hpp>
#include <cpu/paging.hpp>

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

namespace drivers::usb
{
    using namespace bus;

    struct device_desc
    {
        u8  length;
        u8  descType;
        u16 bcdUSB;
        u8  deviceClass;
        u8  subClass;
        u8  protocolCode;
        u8  maxPacketSize;
        u16 vendorID;
        u16 productID;
        u16 bcdDevice;
        u8  iManufacture;
        u8  iProduct;
        u8  iSerialNumber;
        u8  configCount;
    }_packed;

    void printWideString(char* string, u8 byteLength)
    {
        for(int i = 0; i < (byteLength / 2); i++)
        {
            putc(string[i * 2]);
        }
    }
    void copyWideStringToString(void* src, void* dst, size_t srcByteLength)
    {
        u16* in = (u16*)src;
        u8* out = (u8*)dst;

        for(int i = 0; i < (srcByteLength / 2); i++)
        {
            out[i] = in[i];
        }
        out[srcByteLength / 2] = 0;
    }

    void globalResetController(pci::device* device)
    {
        // reset controller
        for(int i = 0; i < GLOBAL_RESET_COUNT; i++)
        {
            device->outw(UHCI_COMMAND, 0x4);
            PIT_sleep(15);
            device->outw(UHCI_COMMAND, 0x0);
        }
    }

    uhci_controller::uhci_controller(pci::device* device) : uhciController(device), device_count(0) { }

    // returns if a USB port is present
    bool uhci_controller::isPortPresent(u16 portID)
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

        u32 response = uhciController->inw(UHCI_PORT_BASE + portID);

        if((uhciController->inw(UHCI_PORT_BASE + portID) & 0x0A) != 0) return false;

        return true;
    }
    // returns if a device is connected to USB port, and resets the device
    bool uhci_controller::resetPort(u16 portID)
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
    void uhci_controller::insertToQueue(uhci_queue_head* qh, u8 queueID)
    {
        // first time
        if((uhciQueueHeads[queueID].ptrVertical & UHCI_Invalid) == UHCI_Invalid)
        {
            uhciQueueHeads[queueID].ptrVertical = cpu::getPhysicalLocation(qh) | UHCI_QH;
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
            lastQH->ptrHorizontal = cpu::getPhysicalLocation(qh) | UHCI_QH;

            qh->next = nullptr;
            qh->vert = nullptr;
            qh->ptrHorizontal = UHCI_Invalid;
        }
    }

    // remove queue head from queue
    void uhci_controller::removeFromQueue(uhci_queue_head* qh, u8 queueID)
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

    u8 uhci_controller::getTransferStatus(volatile uhci_td* td, size_t count)
    {
        for(int i = 0; i < count; i++)
        {
            u8 status = (td[i].ctrlStatus >> 16) & 0xFF;

            if(status != 0)
            {
                if((status & (1 << 3)) == (1 << 3)) return 2;
                if((status & (1 << 7)) == (1 << 7)) return 3;

                return 1; // ERROR occured
            }
        }

        return 0;
    }
    u8 uhci_controller::waitTillTransferComplete(uhci_td* td, size_t count)
    {
        u16 timeout = 1000;
        u8 status = 3;
        while(status == 3 && timeout > 0)
        {
            timeout -= 10;
            PIT_sleep(10);
            status = getTransferStatus(td, count);
        }

        return status;
    }

    void uhci_controller::setupDevice(vfs::vfs_t* gvfs, vfs::node_t* controller_node, u16 portID)
    {
        u16 portsc = uhciController->inw(UHCI_PORT_BASE + portID);

        usb_device device;
        device.isLowSpeedDevice = (portsc & (1 << 8)) ? 1 : 0;
        device.address = 0;

        device_desc deviceDesc;

        char mf[STR_MAX_SIZE];
        char prod[STR_MAX_SIZE];
        char srn[STR_MAX_SIZE];

        device.portAddress = UHCI_PORT_BASE + portID;

        u8 requestType = USB_DEVICE_TO_HOST | USB_REQ_STRD | USB_REP_DEVICE;

        if(controlIn(device, &deviceDesc, requestType, REQ_GET_DESC, (1 << 8) | 0, 0, 18, 8))
        {
            u16 LANGID = 0x0409; // English(US)

            bool status = controlIn(device, mf, requestType, REQ_GET_DESC, (3 << 8) | deviceDesc.iManufacture, LANGID, STR_MAX_SIZE, deviceDesc.maxPacketSize);
            status &= controlIn(device, prod, requestType, REQ_GET_DESC, (3 << 8) | deviceDesc.iProduct, LANGID, STR_MAX_SIZE, deviceDesc.maxPacketSize);
            status &= controlIn(device, srn, requestType, REQ_GET_DESC, (3 << 8) | deviceDesc.iSerialNumber, LANGID, STR_MAX_SIZE, deviceDesc.maxPacketSize);

            if(!status) log_warn("[UHCI][DeviceSetup] Failed to retrieve device info.\n");
            else
            {
                device.manufactureName = std::string((u16*)(mf + 2), mf[0] - 1);
                device.productName     = std::string((u16*)(prod + 2), prod[0] - 1);
                device.serialNumber    = std::string((u16*)(srn + 2), srn[0] - 1);
            }

            device.bcdDevice = deviceDesc.bcdDevice;
            device.configCount = deviceDesc.configCount;
            device.productID = deviceDesc.productID;
            device.protocolCode = deviceDesc.protocolCode;
            device.maxPacketSize = deviceDesc.maxPacketSize;
            device.subClass = deviceDesc.subClass;
            device.vendorID = deviceDesc.vendorID;

            // initialized by usb protocol layer
            // device.controller = nullptr;

            device.address = 0;

            u8 setAddrReqType = USB_HOST_TO_DEVICE | USB_REQ_STRD | USB_REP_DEVICE;

            // reset the device again.
            resetPort(portID);

            std::string ctrl_path = "/dev/" + std::string(controller_node->name);

            if(controlOut(device, setAddrReqType, REQ_SET_ADDR, addressCount, 0, 0, device.maxPacketSize))
            {
                device.address = addressCount; addressCount++;

                std::string dev_name = "usb" + std::utos(device_count);
                vfs::node_t node = vfs::make_node(dev_name.take(), false);

                node.flags |= vfs::NODE_DEVICE;
                node.data = new usb_device(device);
                node.size = device.maxPacketSize;
                node.write = vfs::ignore_write;
                node.read = vfs::ignore_read;
                node.driver_uid = UID_CLASS_USB;

                vfs::add_dnode(gvfs, ctrl_path.c_str(), node);

                device_count++;

                //connectedDevices.push_back(device);
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
    /*
    std::vector<usb_device>& uhci_controller::getAllDevices()
    {
        return this->connectedDevices;
    }
    */

    // initialize the controller
    bool uhci_controller::Init()
    {
        // TODO: return error if none of the BARs are I/O address

        // enable bus matering
        uhciController->config_write<u8>(0x4, 0b101);

        u32 addr = uhciController->config_read<u32>(0x20);

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
        u8 bSOF = uhciController->inb(UHCI_SOF);

        // reset
        uhciController->outw(UHCI_COMMAND, 0x2);

        // sleep for 42 ms
        PIT_sleep(45);

        if(uhciController->inb(UHCI_COMMAND) & 0x2) return false;

        uhciController->outb(UHCI_SOF, bSOF);

        // disable legacy keyboard/mouse support
        uhciController->config_write<u16>(UHCI_PCI_LEGACY, 0xAF00);

        return true;
    }
    // setup the controller
    void uhci_controller::Setup(vfs::vfs_t* gvfs, vfs::node_t* controller_node)
    {
        // disable all interrupts
        uhciController->outw(UHCI_INTR, 0b1111);

        // reset frame num register to 0
        uhciController->outw(UHCI_FRNUM, 0);

        this->frameList = (u32*)std::malloc_aligned(1024 * sizeof(u32), 4096);
        ptr_t frameListPhys = cpu::getPhysicalLocation(this->frameList);

        std::memset(this->frameList, 0, 1024 * sizeof(u32));

        // allocate Queue Heads
        this->uhciQueueHeads = (uhci_queue_head*)std::malloc_aligned(UHCI_QUEUE_COUNT * sizeof(uhci_queue_head), 16);
        ptr_t queueHeadPhy = cpu::getPhysicalLocation(this->uhciQueueHeads);

        std::memset(this->uhciQueueHeads, 0, UHCI_QUEUE_COUNT * sizeof(uhci_queue_head));

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

        u32 fraddr = uhciController->inl(UHCI_FRADDR);
        u32 frnum = uhciController->inl(UHCI_FRNUM);

        // clear status register
        uhciController->outw(UHCI_STATUS, 0xFFFF);
        // set the USB command register.
        uhciController->outw(UHCI_COMMAND, (1 << 7) | (1 << 6) | (1 << 0));

        PIT_sleep(10);

        u32 cmdReg = uhciController->inw(UHCI_COMMAND);

        // address of 0 is reserved.
        addressCount = 1;

        u16 port = 0;
        while(isPortPresent(port))
        {
            portCount++;

            if(resetPort(port))
            {
                //log_info("[UHCI] Found USB 1.0 Device at port %x\n", port);

                setupDevice(gvfs, controller_node, port);
            }

            // try next port
            port += 2;
        }
    }
    bool uhci_controller::resetDevice(usb_device& device)
    {
        return resetPort(device.portAddress);
    }

    bool uhci_controller::controlIn(usb_device device, void* buffer, u8 requestType, u8 request, u16 value, u16 index, u16 length, u8 packetSize)
    {
        request_packet* rpacket = (request_packet*)std::malloc_aligned(sizeof(request_packet), 16);
        rpacket->requestType = requestType;
        rpacket->request = request;
        rpacket->index = index;
        rpacket->value = value;
        rpacket->size = length;

        // return buffer
        u8* retBuffer = (u8*)std::malloc(length);
        std::memset(retBuffer, 0, length);

        u16 td_count = DivRoundUp(length, packetSize) + 2;
        uhci_td* td_in = (uhci_td*)std::malloc_aligned(td_count * sizeof(uhci_td), 16);
        std::memset(td_in, 0, td_count * sizeof(uhci_td));

        uhci_queue_head* qh = (uhci_queue_head*)std::malloc_aligned(sizeof(qh), 16);
        std::memset(qh, 0, sizeof(uhci_queue_head));
        qh->ptrVertical = cpu::getPhysicalLocation(td_in);

        // the first TD describes the control packet
        td_in[0].linkPointer = cpu::getPhysicalLocation(&td_in[1]);
        td_in[0].ctrlStatus = (device.isLowSpeedDevice ? (1 << 26) : 0) | (1 << 23);
        td_in[0].packetHeader = ((sizeof(request_packet) - 1) << 21) | (CTRL_ENDPOINT << 15) | (device.address << 8) | PACKET_SETUP; // DATA0
        td_in[0].bufferPointer = cpu::getPhysicalLocation(rpacket);

        u16 sz = length;

        for(int i = 1; (i < td_count - 1); i++)
        {
            u16 tokenSize = sz < packetSize ? sz : packetSize;

            td_in[i].linkPointer = cpu::getPhysicalLocation(&td_in[i + 1]);
            td_in[i].ctrlStatus = (device.isLowSpeedDevice ? (1 << 26) : 0) | (1 << 23);
            td_in[i].packetHeader = ((tokenSize - 1) << 21) | (CTRL_ENDPOINT << 15) | ((i & 1) ? (1 << 19) : 0) | (device.address << 8) | PACKET_IN;
            td_in[i].bufferPointer = cpu::getPhysicalLocation(retBuffer) + (i - 1) * packetSize;

            sz -= tokenSize;
        }

        td_in[td_count-1].linkPointer = UHCI_Invalid;
        td_in[td_count-1].ctrlStatus = (device.isLowSpeedDevice ? (1 << 26) : 0) | (1 << 23);
        td_in[td_count-1].packetHeader = (0x7FF << 21) | (1 << 19) | (CTRL_ENDPOINT << 15) | (device.address << 8) | PACKET_OUT; // DATA1
        td_in[td_count-1].bufferPointer = nullptr;

        insertToQueue(qh, UHCI_QControl);

        u8 status = waitTillTransferComplete(td_in, td_count);

        removeFromQueue(qh, UHCI_QControl);

        std::memcpy(retBuffer, buffer, length);

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
    bool uhci_controller::controlOut(usb_device device, u8 requestType, u8 request, u16 value, u16 index, u16 length, u8 packetSize)
    {
        request_packet* rpacket = (request_packet*)std::malloc_aligned(sizeof(request_packet), 16);
        rpacket->requestType = requestType;
        rpacket->request = request;
        rpacket->index = index;
        rpacket->value = value;
        rpacket->size = length;

        uhci_td* td_in = (uhci_td*)std::malloc_aligned(2 * sizeof(uhci_td), 16);
        std::memset(td_in, 0, 2 * sizeof(uhci_td));

        uhci_queue_head* qh = (uhci_queue_head*)std::malloc_aligned(sizeof(qh), 16);
        std::memset(qh, 0, sizeof(uhci_queue_head));
        qh->ptrVertical = cpu::getPhysicalLocation(td_in);

        // the first TD describes the control packet
        td_in[0].linkPointer = cpu::getPhysicalLocation(&td_in[1]);
        td_in[0].ctrlStatus = (device.isLowSpeedDevice ? (1 << 26) : 0) | (1 << 23);
        td_in[0].packetHeader = ((sizeof(request_packet) - 1) << 21) | (CTRL_ENDPOINT << 15) | (device.address << 8) | PACKET_SETUP; // DATA0
        td_in[0].bufferPointer = cpu::getPhysicalLocation(rpacket);

        // last TD, status
        td_in[1].linkPointer = UHCI_Invalid;
        td_in[1].ctrlStatus = (device.isLowSpeedDevice ? (1 << 26) : 0) | (1 << 23);
        td_in[1].packetHeader = (0x7FF << 21) | DATA1 | (CTRL_ENDPOINT << 15) | (device.address << 8) | PACKET_IN; // DATA1
        td_in[1].bufferPointer = nullptr;

        insertToQueue(qh, UHCI_QControl);

        u8 status = waitTillTransferComplete(td_in, 2);

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

    bool uhci_controller::bulkIn(const usb_device& device, u8 endpoint, u16 maxPacketSize, void* buffer, u16 size)
    {
        void* returnBuffer = std::malloc(size);

        u16 td_count = DivRoundUp(size, maxPacketSize);
        uhci_td* td_in = (uhci_td*)std::malloc_aligned(td_count * sizeof(uhci_td), 16);
        std::memset(td_in, 0, td_count * sizeof(uhci_td));

        uhci_queue_head* qh = (uhci_queue_head*)std::malloc_aligned(sizeof(qh), 16);
        std::memset(qh, 0, sizeof(uhci_queue_head));
        qh->ptrVertical = cpu::getPhysicalLocation(td_in);
        qh->ptrHorizontal = UHCI_Invalid;

        u16 sz = size;

        for(int i = 0; i < td_count; i++)
        {
            u16 tokenSize = sz < maxPacketSize ? sz : maxPacketSize;

            td_in[i].linkPointer = cpu::getPhysicalLocation(&td_in[i + 1]);
            if(i == td_count - 1) td_in[i].linkPointer = UHCI_Invalid;

            td_in[i].ctrlStatus = (device.isLowSpeedDevice ? (1 << 26) : 0) | (1 << 23);
            td_in[i].packetHeader = ((tokenSize - 1) << 21) | (endpoint << 15) | ((i & 1) ? (1 << 19) : 0) | (device.address << 8) | PACKET_IN;
            td_in[i].bufferPointer = cpu::getPhysicalLocation(returnBuffer) + i * maxPacketSize;

            sz -= tokenSize;
        }

        insertToQueue(qh, UHCI_QControl);
        u8 status = waitTillTransferComplete(td_in, td_count);
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
                log_warn("[UHCI][ControlIn] ERROR USB device. Info: \n");
                break;
            default:
                break;
            }
            log_warn("\tPort: %x\n", device.portAddress);
            log_warn("\tAddress: %x\n", device.address);
            log_warn("\tEndpoint: %x\n", endpoint);
            log_warn("\tSpeed: %s\n", device.isLowSpeedDevice ? "Low Speed" : "Full Speed");
            log_warn("\tMax Packet Size: %x\n", maxPacketSize);
            log_warn("\tSent Length: %x\n", size);
        }

        if(status == 0) std::memcpy(returnBuffer, buffer, size);

        std::free(returnBuffer);
        std::free(td_in);
        std::free(qh);

        return status == 0;
    }
    bool uhci_controller::bulkOut(const usb_device& device, u8 endpoint, u16 maxPacketSize, void* buffer, u16 size)
    {
        u16 td_count = DivRoundUp(size, maxPacketSize);
        uhci_td* td_in = (uhci_td*)std::malloc_aligned(td_count * sizeof(uhci_td), 16);
        std::memset(td_in, 0, td_count * sizeof(uhci_td));

        uhci_queue_head* qh = (uhci_queue_head*)std::malloc_aligned(sizeof(qh), 16);
        std::memset(qh, 0, sizeof(uhci_queue_head));
        qh->ptrVertical = cpu::getPhysicalLocation(td_in);
        qh->ptrHorizontal = UHCI_Invalid;

        void* buffer_out = std::malloc_aligned(size, 16);
        std::memcpy(buffer, buffer_out, size);

        u16 sz = size;

        for(int i = 0; i < td_count; i++)
        {
            u16 tokenSize = sz < maxPacketSize ? sz : maxPacketSize;

            td_in[i].linkPointer = cpu::getPhysicalLocation(&td_in[i + 1]);
            if(i == td_count - 1) td_in[i].linkPointer = UHCI_Invalid;

            td_in[i].ctrlStatus = (device.isLowSpeedDevice ? (1 << 26) : 0) | (1 << 23);
            td_in[i].packetHeader = ((tokenSize - 1) << 21) | (endpoint << 15) | ((i & 1) ? (1 << 19) : 0) | (device.address << 8) | PACKET_OUT;
            td_in[i].bufferPointer = cpu::getPhysicalLocation(buffer_out) + i * maxPacketSize;

            sz -= tokenSize;
        }

        insertToQueue(qh, UHCI_QControl);
        u8 status = waitTillTransferComplete(td_in, td_count);
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
            log_warn("\tMax Packet Size: %x\n", maxPacketSize);
            log_warn("\tSent Length: %x\n", size);
        }

        std::free(buffer_out);
        std::free(qh);
        std::free(td_in);

        return status == 0;
    }

    bool uhci_controller::controlIn(const usb_device& device, request_packet rpacket, void* buffer, u16 size)
    {
        return controlIn(device, buffer, rpacket.requestType, rpacket.request, rpacket.value, rpacket.index, size, device.maxPacketSize);
    }
    bool uhci_controller::controlOut(const usb_device& device, request_packet rpacket, u16 size)
    {
        return controlOut(device, rpacket.requestType, rpacket.request, rpacket.value, rpacket.index, size, device.maxPacketSize);
    }

    namespace uhci
    {
        bool reset_device(void* instance_data, usb_device& device)
        {
            return reinterpret_cast<uhci_controller*>(instance_data)->resetDevice(device);
        }

        bool controlIn(void* instance_data, const usb_device& device, request_packet rpacket, void* buffer, u16 size)
        {
            return reinterpret_cast<uhci_controller*>(instance_data)->controlIn(device, rpacket, buffer, size);
        }
        bool controlOut(void* instance_data, const usb_device& device, request_packet rpacket, u16 size)
        {
            return reinterpret_cast<uhci_controller*>(instance_data)->controlOut(device, rpacket, size);
        }

        bool bulkIn(void* instance_data, const usb_device& device, u8 endpoint, u16 maxPacketSize, void* buffer, u16 size)
        {
            return reinterpret_cast<uhci_controller*>(instance_data)->bulkIn(device, endpoint, maxPacketSize, buffer, size);
        }
        bool bulkOut(void* instance_data, const usb_device& device, u8 endpoint, u16 maxPacketSize, void* buffer, u16 size)
        {
            return reinterpret_cast<uhci_controller*>(instance_data)->bulkOut(device, endpoint, maxPacketSize, buffer, size);
        }
    }

    usb_controller uhci_controller::as_generic_controller()
    {
        usb_controller controller;
        controller.instance_data = (void*)this;

        controller.reset_device = uhci::reset_device;
        controller.controlIn = uhci::controlIn;
        controller.controlOut = uhci::controlOut;
        controller.bulkIn = uhci::bulkIn;
        controller.bulkOut = uhci::bulkOut;

        return controller;
    }

    struct uhci_kdriver
    {
        vfs::node_t* pci_node;
        std::vector<vfs::node_t*> vfs_nodes;
        std::vector<uhci_controller*> controllers;
    };

    const char* uhci_kdriver_error_desc(u32 error)
    {
        const char* error_array[] = {
            "No Failiure",
            "Event parsing failed[this driver does not handle the given event]",
            "Critical Failiure",
            "Failed to initialize uhci controller detected on pci bus"
        };

        if(error >= sizeof(error_array)/sizeof(char*)) return nullptr;

        return error_array[error];
    }
    u32 uhci_kdriver_init(kernel_driver* driver)
    {
        uhci_kdriver uchi_driver;
        uchi_driver.pci_node = vfs::get_node(driver->gvfs, "/hw/pci");

        if(IS_ERR_PTR(uchi_driver.pci_node)) return DRIVER_EXEC_FAIL;

        // just initialize the driver
        driver->data = new uhci_kdriver(uchi_driver);

        return DRIVER_SUCCESS;
    }
    u32 uhci_kdriver_process_event(kernel_driver* driver, vfs::event_t event)
    {
        if(event.flags != vfs::EVENT_DEVICE_ADD) return DRIVER_PARSE_FAIL;
        if(event.trigger_node == nullptr) return DRIVER_PARSE_FAIL;

        vfs::node_t* dnode = event.trigger_node;
        uhci_kdriver& self = *reinterpret_cast<uhci_kdriver*>(driver->data);

        // not a pci device
        if(dnode->parent != self.pci_node) return DRIVER_PARSE_FAIL;
        bus::pci::device* pci_device = reinterpret_cast<bus::pci::device*>(dnode->data);
        
        // check if this is a new uhci controller
        if(pci_device->classCode != 0x0C || pci_device->subClass != 0x03 || pci_device->progIF != 0x00) return DRIVER_PARSE_FAIL;

        // it's a controller, setup the device
        pci_device->setup();

        uhci_controller* controller = new usb::uhci_controller(pci_device);
        if(!controller->Init()) return 0x3;

        // make a new node for the uhci controller
        std::string name = "uhci" + std::utos(self.controllers.size());
        vfs::node_t node = vfs::make_node(name.copy().take(), true);

        // set the controller
        node.data = new usb_controller(controller->as_generic_controller());

        vfs::add_vnode(driver->gvfs, "/dev/", node);

        name = "/dev/" + name;
        vfs::node_t* ctrl_node = vfs::get_node(driver->gvfs, name.c_str());

        // setup the controller and detect new devices
        controller->Setup(driver->gvfs, ctrl_node);

        // finially add to list of controllers
        self.controllers.push_back(controller);
        self.vfs_nodes.push_back(ctrl_node);

        return DRIVER_SUCCESS;
    }

    kernel_driver get_uhci_driver(vfs::vfs_t* gvfs)
    {
        return kernel_driver{
            // driver name and desciption
            .name = "dvr_uhci",
            .desc = "USB 1.0 UHCI Driver",

            // the filesystem
            .gvfs = gvfs,
            .data = nullptr,

            // some driver functions
            .get_error_desc = uhci_kdriver_error_desc,
            .init = uhci_kdriver_init,
            .process_event = uhci_kdriver_process_event,

            // the class of the driver
            .uid_class = UID_CLASS_USBCTRL
        };
    }
}
