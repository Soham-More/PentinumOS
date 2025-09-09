#include "mass_storage.hpp"

#include <io/io.h>
#include <std/std.hpp>
#include <std/ds.hpp>

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
/*
namespace drivers::usb
{
    //using namespace bus;
    //using usb_device;

    struct CommandBlockWrapper
    {
        u32 signature;     // USBC in hexadecimal, acting as magic number
        u32 tag;           // Signature
        u32 transferLen;   // Number of bytes to transfer excluding size of CBW
        u8 flags;          // 7: 0=Out 1=In, 6:0=Reserved
        u8 lun;            // 7:4 Reserved, 3:0 Logical Unit Number
        u8 cmdLen;         // Length of command in next field [1-16]
        u8 command[16];    // Command Data
    } _packed;
    struct CommandStatusWrapper
    {
        u32 signature;     // CSW Magic number
        u32 tag;           // Signature, same as CBW
        u32 dataResidue;   // Difference in data actually read/written
        u8 status;         // Status Byte
    } _packed;
    struct InquiryReturnBlock
    {
        u8 peripheralDeviceType : 5;
        u8 peripheralQualifier : 3;
        u8 resv1 : 7;
        u8 RMB : 1;
        u8 version;
        u8 responseDataFormat : 4;
        u8 HiSup : 1;
        u8 NormACA : 1;
        u8 resv2 : 2;
        u8 additionalLength;
        u8 prot : 1;
        u8 resv3 : 2;
        u8 _3PC : 1;
        u8 TPGS : 2;
        u8 ACC : 1;
        u8 SCCS : 1;
        u8 addr16 : 1;
        u8 resv4 : 3;
        u8 multiP : 1;
        u8 VS1 : 1;
        u8 ENCServ : 1;
        u8 resv5 : 1;
        u8 VS2 : 1;
        u8 cmndQue : 1;
        u8 resv6 : 2;
        u8 SYNC : 1;
        u8 wbus16 : 1;
        u8 resv7 : 2;
        char vendorInformation[8];
        char productIdentification[16];
        char productRevisionLevel[4];
    } _packed;
    struct CapacityListHeader
    {
        u8 reserved[3];
        u8 listLength;
    } _packed;
    struct CapacityDescriptor
    {
        u32 numberOfBlocks;
        u8 descriptorCode : 2;
        u8 reserved : 6;
        u32 blockLength : 24;
    } _packed;
    struct Capacity10Block
    {
        u32 logicalBlockAddress;
        u32 blockLength;
    } _packed;
    struct Capacity16Block
    {
        uint64_t logicalBlockAddress;
        u32 blockLength;
        u8 unused[20];
    } _packed;
    struct RequestSenseBlock
    {
        u8 errorCode : 7;
        u8 valid : 1;
        u8 resv1;
        u8 senseKey : 4;
        u8 resv2 : 1;
        u8 ILI : 1;
        u8 EOM : 1;
        u8 fileMark : 1;
        u8 information[4];
        u8 additionalLength;
        u8 resv3[4];
        u8 ASC;
        u8 ASCQ;
        u8 FRUC;
        u8 specific[3];
    } _packed;

    CommandBlockWrapper SCSIprepareCommandBlock(u8 command, int length, uint64_t lba = 0, int sectors = 0)
    {
        CommandBlockWrapper cmd;
        std::memset(&cmd, 0, sizeof(CommandBlockWrapper));

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
    bool SCSIRequest(const msd_device& device, CommandBlockWrapper* request, u8* dataPointer, int dataLength)
    {
        // Send request to device
        if(!device.sendBulkOut(request, sizeof(CommandBlockWrapper)))
        {
            log_error("[USB Mass Storage Driver] Error Sending command %b to bulk out endpoint\n", request->command[0]);
            
            // Clear HALT for the OUT-Endpoint
            if(!usb::clear_feature(device.target, USB_REP_ENDPOINT, 0, device.bulkOut->endpointAddress & 0xF))
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
                    if(!usb::clear_feature(device.target, USB_REP_ENDPOINT, 0, device.bulkIn->endpointAddress & 0xF))
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
                    if(!usb::clear_feature(device.target, USB_REP_ENDPOINT, 0, device.bulkOut->endpointAddress & 0xF))
                    {
                        log_error("[USB Mass Storage Driver] Clear feature (HALT) Failed for Bulk-Out!\n");
                        return false;
                    }
                }
            }
        }

        // Receive status descriptor from device.
        // Also when receiving data failed this is required
        CommandStatusWrapper status; std::memset(&status, 0, sizeof(CommandStatusWrapper));

        if(!device.sendBulkIn(&status, sizeof(CommandStatusWrapper))) {
            log_error("[USB Mass Storage Driver] Error reading Command Status Wrapper from bulk in endpoint\n");

            // Clear HALT feature for the IN-Endpoint
            if(!usb::clear_feature(device.target, USB_REP_ENDPOINT, 0, device.bulkIn->endpointAddress & 0xF))
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

            if(SCSIRequest(device, &requestSenseCMD, (u8*)&requestSenseRet, sizeof(RequestSenseBlock)))
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
        return usb::bulk_out(this->target, this->bulkOut, buffer, size);
    }
    bool msd_device::sendBulkIn(void* buffer, size_t size) const
    {
        return usb::bulk_in(this->target, this->bulkIn, buffer, size);
    }

    bool msd_device::detect(const usb_device &device)
    {
        // TODO: identify the configuration that supports Bulk only transport
        usb::config_desc config0 = usb::get_config(device, 0);
        return config0.interfaces[0].classCode == 0x08 && config0.interfaces[0].subClass == 0x06;
    }

    bool msd_device::reset() const
    {
        // bulk reset packet
        usb::request_packet reqPacket;
        reqPacket.requestType = USB_HOST_TO_DEVICE | USB_REQ_CLASS | USB_REP_INTERFACE;
        reqPacket.request = 0xFF;
        reqPacket.value = 0;
        reqPacket.index = interface->interfaceID;
        reqPacket.size = 0;

        return usb::control_packet_out(this->target, reqPacket);
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
        if(!usb::clear_feature(this->target, USB_REP_ENDPOINT, 0, this->bulkIn->endpointAddress & 0xF))
        {
            log_error("[USB Mass Storage Driver] Clear feature (HALT) Failed for Bulk-In!\n");
            return false;
        }

        // Then the Clear feature for the OUT-Endpoint
        if(!usb::clear_feature(this->target, USB_REP_ENDPOINT, 0, this->bulkOut->endpointAddress & 0xF))
        {
            log_error("[USB Mass Storage Driver] Clear feature (HALT) Failed for Bulk-Out!\n");
            return false;
        }

        return true;
    }

    bool msd_device::init(const usb_device& device)
    {
        // TODO: identify the configuration that supports Bulk only transport
        usb::config_desc config0 = usb::get_config(device, 0);
        bool status = usb::set_config(device, config0);

        this->interface = &config0.interfaces[0];

        // copy 
        this->target = device;

        // bulk reset packet
        usb::request_packet reqPacket;
        reqPacket.requestType = USB_DEVICE_TO_HOST | USB_REQ_CLASS | USB_REP_INTERFACE;
        reqPacket.request = 0xFE;
        reqPacket.value = 0;
        reqPacket.index = this->interface->interfaceID;
        reqPacket.size = 1;

        if(!usb::control_packet_in(this->target, reqPacket, &this->logicalUnitCount))
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
        if(SCSIRequest(*this, &inquiryCMD, (u8*)&inquiryRet, sizeof(InquiryReturnBlock))) {
            vendorSCSI[8] = '\0'; std::memcpy(inquiryRet.vendorInformation, vendorSCSI, 8);
            productSCSI[16] = '\0'; std::memcpy(inquiryRet.productIdentification, productSCSI, 16);
            revisionSCSI[4] = '\0'; std::memcpy(inquiryRet.productRevisionLevel, revisionSCSI, 4);

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
        if(SCSIRequest(*this, &readCapacityCMD, (u8*)&readCapacityRet, sizeof(Capacity10Block)))
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
                if(SCSIRequest(*this, &readCapacity16CMD, (u8*)&readCapacityRet16, sizeof(Capacity16Block))) {
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

    u32 msd_device::get_sector_size()
    {
        return blockSize;
    }
    bool msd_device::read_sectors(u32 lba, size_t count, void* buffer)
    {
        CommandBlockWrapper sendBuf = SCSIprepareCommandBlock(use16Base ? SCSI_READ_16 : SCSI_READ_10, blockSize, lba, count);
        if(SCSIRequest(*this, &sendBuf, (u8*)buffer, blockSize)) return true;

        log_error("[USB Mass Storage Driver] Error writing sector %x", lba);
        return false;
    }

    namespace msd
    {
        bool read_sectors_wrapper(void* instance, u32 lba, size_t count, void* buffer)
        {
            return reinterpret_cast<msd_device*>(instance)->read_sectors(lba, count, buffer);
        }
    }
    disk_driver msd_device::as_disk_driver()
    {
        disk_driver driver;

        driver.driver_instance = (void*)this;
        driver.block_byte_size = this->blockSize;
        driver.read_blocks_fptr = msd::read_sectors_wrapper;

        return driver;
    }
}
*/