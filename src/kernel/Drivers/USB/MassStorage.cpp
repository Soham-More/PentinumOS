#include "MassStorage.hpp"

#include <std/IO.hpp>

#define SCSI_TEST_UNIT_READY        0x00
#define SCSI_REQUEST_SENSE          0x03
#define SCSI_INQUIRY                0x12
#define SCSI_READ_FORMAT_CAPACITIES 0x23
#define SCSI_READ_CAPACITY_10       0x25
#define SCSI_READ_CAPACITY_16       0x9E
#define SCSI_READ_10                0x28
#define SCSI_WRITE_10               0x2A
#define SCSI_READ_16                0x88
#define SCSI_WRITE_16               0x8A
//#define SCSI_READ_12                0xA8
//#define SCSI_WRITE_12               0xAA

#define CBW_SIGNATURE 0x43425355
#define CSW_SIGNATURE 0x53425355

namespace USB
{
    struct CommandBlockWrapper
    {
        uint32_t signature;     // USBC in hexadecimal, acting as magic number
        uint32_t tag;           // Signature
        uint32_t transferLen;   // Number of bytes to transfer excluding size of CBW
        uint8_t flags;          // 7: 0=Out 1=In, 6:0=Reserved
        uint8_t lun;            // 7:4 Reserved, 3:0 Logical Unit Number
        uint8_t cmdLen;         // Length of command in next field [1-16]
        uint8_t command[16];    // Command Data
    } _packed;
    struct CommandStatusWrapper
    {
        uint32_t signature;     // CSW Magic number
        uint32_t tag;           // Signature, same as CBW
        uint32_t dataResidue;   // Difference in data actually read/written
        uint8_t status;         // Status Byte
    } _packed;
    struct InquiryReturnBlock
    {
        uint8_t peripheralDeviceType : 5;
        uint8_t peripheralQualifier : 3;
        uint8_t resv1 : 7;
        uint8_t RMB : 1;
        uint8_t version;
        uint8_t responseDataFormat : 4;
        uint8_t HiSup : 1;
        uint8_t NormACA : 1;
        uint8_t resv2 : 2;
        uint8_t additionalLength;
        uint8_t prot : 1;
        uint8_t resv3 : 2;
        uint8_t _3PC : 1;
        uint8_t TPGS : 2;
        uint8_t ACC : 1;
        uint8_t SCCS : 1;
        uint8_t addr16 : 1;
        uint8_t resv4 : 3;
        uint8_t multiP : 1;
        uint8_t VS1 : 1;
        uint8_t ENCServ : 1;
        uint8_t resv5 : 1;
        uint8_t VS2 : 1;
        uint8_t cmndQue : 1;
        uint8_t resv6 : 2;
        uint8_t SYNC : 1;
        uint8_t wbus16 : 1;
        uint8_t resv7 : 2;
        char vendorInformation[8];
        char productIdentification[16];
        char productRevisionLevel[4];
    } _packed;
    struct CapacityListHeader
    {
        uint8_t reserved[3];
        uint8_t listLength;
    } _packed;
    struct CapacityDescriptor
    {
        uint32_t numberOfBlocks;
        uint8_t descriptorCode : 2;
        uint8_t reserved : 6;
        uint32_t blockLength : 24;
    } _packed;
    struct Capacity10Block
    {
        uint32_t logicalBlockAddress;
        uint32_t blockLength;
    } _packed;
    struct Capacity16Block
    {
        uint64_t logicalBlockAddress;
        uint32_t blockLength;
        uint8_t unused[20];
    } _packed;
    struct RequestSenseBlock
    {
        uint8_t errorCode : 7;
        uint8_t valid : 1;
        uint8_t resv1;
        uint8_t senseKey : 4;
        uint8_t resv2 : 1;
        uint8_t ILI : 1;
        uint8_t EOM : 1;
        uint8_t fileMark : 1;
        uint8_t information[4];
        uint8_t additionalLength;
        uint8_t resv3[4];
        uint8_t ASC;
        uint8_t ASCQ;
        uint8_t FRUC;
        uint8_t specific[3];
    } _packed;

    CommandBlockWrapper SCSIprepareCommandBlock(uint8_t command, int length, uint64_t lba = 0, int sectors = 0)
    {
        CommandBlockWrapper cmd;
        memset(&cmd, 0, sizeof(CommandBlockWrapper));

        //Default parameters for each request
        cmd.signature = CBW_SIGNATURE;
        cmd.tag = 0xDEAD;
        cmd.transferLen = length;
        cmd.command[0] = command;

        switch(command)
        {
            case SCSI_TEST_UNIT_READY:
                cmd.flags = 0x0;
                cmd.cmdLen = 0x6;
                break;
            case SCSI_REQUEST_SENSE:
                cmd.flags = 0x80;
                cmd.cmdLen = 0x6;
                cmd.command[4] = 18;
                break;
            case SCSI_INQUIRY:
                cmd.flags = 0x80;
                cmd.cmdLen = 0x6;
                cmd.command[4] = 36;
                break;
            case SCSI_READ_FORMAT_CAPACITIES:
                cmd.flags = 0x80;
                cmd.cmdLen = 10;
                cmd.command[8] = 0xFC;
                break;
            case SCSI_READ_CAPACITY_10:
                cmd.flags = 0x80;
                cmd.cmdLen = 10;
                break;
            case SCSI_READ_CAPACITY_16:
                cmd.flags = 0x80;
                cmd.cmdLen = 16;
                cmd.command[13] = 32;
                break;
            case SCSI_READ_10:
                cmd.flags = 0x80;
                cmd.cmdLen = 10;
                cmd.command[2] = (lba >> 24) & 0xFF;
                cmd.command[3] = (lba >> 16) & 0xFF;
                cmd.command[4] = (lba >> 8) & 0xFF;
                cmd.command[5] = (lba >> 0) & 0xFF;
                cmd.command[7] = (sectors >> 8) & 0xFF;
                cmd.command[8] = (sectors >> 0) & 0xFF;
                break;
            case SCSI_WRITE_10:
                cmd.flags = 0x0;
                cmd.cmdLen = 10;
                cmd.command[2] = (lba >> 24) & 0xFF;
                cmd.command[3] = (lba >> 16) & 0xFF;
                cmd.command[4] = (lba >> 8) & 0xFF;
                cmd.command[5] = (lba >> 0) & 0xFF;
                cmd.command[7] = (sectors >> 8) & 0xFF;
                cmd.command[8] = (sectors >> 0) & 0xFF;
                break;
            case SCSI_READ_16:
                cmd.flags = 0x80;
                cmd.cmdLen = 16;
                cmd.command[2] = (lba >> 56) & 0xFF;
                cmd.command[3] = (lba >> 48) & 0xFF;
                cmd.command[4] = (lba >> 40) & 0xFF;
                cmd.command[5] = (lba >> 32) & 0xFF;
                cmd.command[6] = (lba >> 24) & 0xFF;
                cmd.command[7] = (lba >> 16) & 0xFF;
                cmd.command[8] = (lba >> 8) & 0xFF;
                cmd.command[9] = (lba >> 0) & 0xFF;
                cmd.command[10] = (sectors >> 24) & 0xFF;
                cmd.command[11] = (sectors >> 16) & 0xFF;
                cmd.command[12] = (sectors >> 8) & 0xFF;
                cmd.command[13] = (sectors >> 0) & 0xFF;
                break;
            case SCSI_WRITE_16:
                cmd.flags = 0x0;
                cmd.cmdLen = 16;
                cmd.command[2] = (lba >> 56) & 0xFF;
                cmd.command[3] = (lba >> 48) & 0xFF;
                cmd.command[4] = (lba >> 40) & 0xFF;
                cmd.command[5] = (lba >> 32) & 0xFF;
                cmd.command[6] = (lba >> 24) & 0xFF;
                cmd.command[7] = (lba >> 16) & 0xFF;
                cmd.command[8] = (lba >> 8) & 0xFF;
                cmd.command[9] = (lba >> 0) & 0xFF;
                cmd.command[10] = (sectors >> 24) & 0xFF;
                cmd.command[11] = (sectors >> 16) & 0xFF;
                cmd.command[12] = (sectors >> 8) & 0xFF;
                cmd.command[13] = (sectors >> 0) & 0xFF;
                break;         
            default:
                log_error("[USB Mass Storage Driver] Unkown Command %b\n", command);
                break;
        }

        return cmd;
    }
    bool SCSIRequest(const msd_device& device, CommandBlockWrapper* request, uint8_t* dataPointer, int dataLength)
    {
        // Send request to device
        if(!device.sendBulkOut(request, sizeof(CommandBlockWrapper)))
        {
            log_error("[USB Mass Storage Driver] Error Sending command %b to bulk out endpoint\n", request->command[0]);
            
            // Clear HALT for the OUT-Endpoint
            if(!clearFeature(device, USB_REP_ENDPOINT, 0, device.bulkOut->endpointAddress & 0xF))
            {
                log_error("[USB Mass Storage Driver] Clear feature (HALT) Failed for Bulk-Out!\n");
                return false;
            }
        }

        // If this is a data command, recieve the data
        if(dataLength > 0) 
        {
            if(request->flags == 0x80)
            {
                if(!device.sendBulkIn(dataPointer, dataLength))
                {
                    log_error("[USB Mass Storage Driver] Error receiving data after command %b from bulk endpoint, len=%d\n", request->command[0], dataLength);
                    
                    // Clear HALT feature for the IN-Endpoint
                    if(!clearFeature(device, USB_REP_ENDPOINT, 0, device.bulkIn->endpointAddress & 0xF))
                    {
                        log_error("[USB Mass Storage Driver] Clear feature (HALT) Failed for Bulk-In!\n");
                        return false;
                    }
                }
            }
            else
            {
                if(!device.sendBulkOut(dataPointer, dataLength)) {
                    log_error("[USB Mass Storage Driver] Error sending data after command %b to bulk endpoint, len=%d\n", request->command[0], dataLength);
                    
                    // Clear HALT feature for the OUT-Endpoint
                    if(!clearFeature(device, USB_REP_ENDPOINT, 0, device.bulkOut->endpointAddress & 0xF))
                    {
                        log_error("[USB Mass Storage Driver] Clear feature (HALT) Failed for Bulk-Out!\n");
                        return false;
                    }
                }
            }
        }

        // Receive status descriptor from device.
        // Also when receiving data failed this is required
        CommandStatusWrapper status; memset(&status, 0, sizeof(CommandStatusWrapper));

        if(!device.sendBulkIn(&status, sizeof(CommandStatusWrapper))) {
            log_error("[USB Mass Storage Driver] Error reading Command Status Wrapper from bulk in endpoint\n");

            // Clear HALT feature for the IN-Endpoint
            if(!clearFeature(device, USB_REP_ENDPOINT, 0, device.bulkIn->endpointAddress & 0xF))
            {
                log_error("[USB Mass Storage Driver] Clear feature (HALT) Failed for Bulk-In!\n");
                return false;
            }
        }

        if(status.signature != CSW_SIGNATURE) {
            device.fullReset();
            return false;
        }

        if((status.status == 1) && (request->command[0] != SCSI_REQUEST_SENSE))
        {
            // Command did not succeed so Request the Sense data
            CommandBlockWrapper requestSenseCMD = SCSIprepareCommandBlock(SCSI_REQUEST_SENSE, sizeof(RequestSenseBlock));
            RequestSenseBlock requestSenseRet;

            if(SCSIRequest(device, &requestSenseCMD, (uint8_t*)&requestSenseRet, sizeof(RequestSenseBlock)))
                log_info("[USB Mass Storage Driver] Request Sense after ReadCap: Error=%b Valid=%d Additional=%d Key=%d ASC=%b ASCQ=%b\n", requestSenseRet.errorCode, requestSenseRet.valid, requestSenseRet.additionalLength, requestSenseRet.senseKey, requestSenseRet.ASC, requestSenseRet.ASCQ);
            else
                log_error("[USB Mass Storage Driver] Could not request sense after read capacity failure\n");

            return false;
        }

        if((status.status == 2) && (request->command[0] != SCSI_REQUEST_SENSE))
        {
            device.fullReset();
            return false;
        }

        if((status.status == 0) && (status.tag == request->tag))
            return true;

        log_error("[USB Mass Storage Driver] Something wrong with Command Status Wrapper, status = %d\n", status.status);
        return false;
    }

    bool msd_device::sendBulkOut(void* buffer, size_t size) const
    {
        return USB::bulkOut(*this, this->bulkOut, buffer, size);
    }
    bool msd_device::sendBulkIn(void* buffer, size_t size) const
    {
        return USB::bulkIn(*this, this->bulkIn, buffer, size);
    }

    bool msd_device::reset() const
    {
        // bulk reset packet
        request_packet reqPacket;
        reqPacket.requestType = USB_HOST_TO_DEVICE | USB_REQ_CLASS | USB_REP_INTERFACE;
        reqPacket.request = 0xFF;
        reqPacket.value = 0;
        reqPacket.index = interface->interfaceID;
        reqPacket.size = 0;

        return controlPacketOut(*this, reqPacket);
    }
    bool msd_device::fullReset() const
    {
        // First send the MSD Reset Request
        if(!reset())
        {
            log_error("[USB Mass Storage Driver] Reset Failed!\n");
            return false;
        }

        // Then the Clear feature for the IN-Endpoint
        if(!clearFeature(*this, USB_REP_ENDPOINT, 0, this->bulkIn->endpointAddress & 0xF))
        {
            log_error("[USB Mass Storage Driver] Clear feature (HALT) Failed for Bulk-In!\n");
            return false;
        }

        // Then the Clear feature for the OUT-Endpoint
        if(!clearFeature(*this, USB_REP_ENDPOINT, 0, this->bulkOut->endpointAddress & 0xF))
        {
            log_error("[USB Mass Storage Driver] Clear feature (HALT) Failed for Bulk-Out!\n");
            return false;
        }

        return true;
    }

    bool msd_device::init(const usb_device& device)
    {
        // TODO: identify the configuration that supports Bulk only transport
        USB::config_desc config0 = USB::getConfig(device, 0);
        
        bool status = USB::setConfig(device, config0);

        this->interface = &config0.interfaces[0];

        // copy 
        *(usb_device*)this = device;

        // bulk reset packet
        request_packet reqPacket;
        reqPacket.requestType = USB_DEVICE_TO_HOST | USB_REQ_CLASS | USB_REP_INTERFACE;
        reqPacket.request = 0xFE;
        reqPacket.value = 0;
        reqPacket.index = this->interface->interfaceID;
        reqPacket.size = 1;

        if(!controlPacketIn(*this, reqPacket, &this->logicalUnitCount))
        {
            log_warn("[USB Mass Storage Driver] get LUN Count Failed");
        }

        for(size_t i = 0; i < this->interface->endpointCount; i++)
        {
            if(this->interface->endpoints[i].endpointAddress & 0x80) this->bulkIn = &this->interface->endpoints[i];
            else this->bulkOut = &this->interface->endpoints[i];
        }

        ///////////////
        // Test Unit Ready
        ///////////////
        CommandBlockWrapper checkReadyCMD = SCSIprepareCommandBlock(SCSI_TEST_UNIT_READY, 0);
        for(int i = 0; i < 3; i++)
        {
            if(!SCSIRequest(*this, &checkReadyCMD, 0, 0))
            {
                log_warn("[USB Mass Storage Driver] Device not ready yet\n");
                PIT_sleep(100);
            }
            else break;
        }

        ///////////////
        // Inquiry
        ///////////////
        CommandBlockWrapper inquiryCMD = SCSIprepareCommandBlock(SCSI_INQUIRY, sizeof(InquiryReturnBlock));
        InquiryReturnBlock inquiryRet;
        if(SCSIRequest(*this, &inquiryCMD, (uint8_t*)&inquiryRet, sizeof(InquiryReturnBlock))) {
            vendorSCSI[8] = '\0'; memcpy(inquiryRet.vendorInformation, vendorSCSI, 8);
            productSCSI[16] = '\0'; memcpy(inquiryRet.productIdentification, productSCSI, 16);
            revisionSCSI[4] = '\0'; memcpy(inquiryRet.productRevisionLevel, revisionSCSI, 4);

            // Check if it is a Direct Access Block Device or CD-ROM/DVD device
            if(inquiryRet.peripheralDeviceType != 0x00 && inquiryRet.peripheralDeviceType != 0x05)
                return false;
            // Check if LUN is connected to something
            if(inquiryRet.peripheralQualifier != 0)
                return false;
            // Response Data Format should be 0x01 or 0x02
            if(inquiryRet.responseDataFormat != 0x01 && inquiryRet.responseDataFormat != 0x02)
                return false;
        }
        else {
            return false;
        }

        ///////////////
        // Read Capacity
        ///////////////
        CommandBlockWrapper readCapacityCMD = SCSIprepareCommandBlock(SCSI_READ_CAPACITY_10, sizeof(Capacity10Block));
        Capacity10Block readCapacityRet;
        if(SCSIRequest(*this, &readCapacityCMD, (uint8_t*)&readCapacityRet, sizeof(Capacity10Block)))
        {
            readCapacityRet.logicalBlockAddress = __builtin_bswap32(readCapacityRet.logicalBlockAddress);
            readCapacityRet.blockLength = __builtin_bswap32(readCapacityRet.blockLength);

            if(readCapacityRet.logicalBlockAddress == 0xFFFFFFFF) // We need to use Read Capacity 16
            {
                ////////////
                // Read Capacity (16)
                ////////////
                CommandBlockWrapper readCapacity16CMD = SCSIprepareCommandBlock(SCSI_READ_CAPACITY_16, sizeof(Capacity16Block));
                Capacity16Block readCapacityRet16;
                if(SCSIRequest(*this, &readCapacity16CMD, (uint8_t*)&readCapacityRet16, sizeof(Capacity16Block))) {
                    readCapacityRet16.logicalBlockAddress = __builtin_bswap64(readCapacityRet16.logicalBlockAddress);
                    readCapacityRet16.blockLength = __builtin_bswap64(readCapacityRet16.blockLength);

                    this->blockCount = readCapacityRet16.logicalBlockAddress - 1;
                    this->blockSize = readCapacityRet16.blockLength;
                    this->use16Base = true;
                }
            }
            else
            {
                this->blockCount = readCapacityRet.logicalBlockAddress - 1;
                this->blockSize = readCapacityRet.blockLength;
            }
        }

        this->size = this->blockCount * this->blockSize;
        return true;
    }

    uint32_t msd_device::get_sector_size()
    {
        return blockSize;
    }
    bool msd_device::read_sectors(uint32_t lba, size_t count, void* buffer)
    {
        CommandBlockWrapper sendBuf = SCSIprepareCommandBlock(use16Base ? SCSI_READ_16 : SCSI_READ_10, blockSize, lba, count);
        if(SCSIRequest(*this, &sendBuf, (uint8_t*)buffer, blockSize)) return true;

        log_error("[USB Mass Storage Driver] Error writing sector %x", lba);
        return false;
    }
}

