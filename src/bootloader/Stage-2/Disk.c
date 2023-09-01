#include "Disk.h"
#include "x86.h"
#include "IO.h"

typedef struct CHS
{
    uint16_t cyl;
    uint16_t head;
    uint16_t sector;
} CHS;

void disk_operation_failed(const char* err_msg)
{
    printf("DISK FAIL: %s", err_msg);
}

CHS lba_to_chs(uint32_t lba, DISK disk)
{
    CHS chs;

    uint16_t temp = lba / disk.sector_count;
    chs.sector = (lba % disk.sector_count) + 1;
    chs.head = temp % disk.head_count;
    chs.cyl = temp / disk.head_count;

    return chs;
}

bool disk_initialise(int drive, DISK* disk)
{
    disk->drive_id = drive;
    bool ret = x86_read_disk_params(drive, &disk->driveType, &disk->cyl_count, &disk->sector_count, &disk->head_count);

    disk->cyl_count += 1;

    return ret;
}

bool disk_readSectors(DISK disk, uint32_t lba, uint16_t no_of_sectors, void* read_bytes)
{
    CHS chs = lba_to_chs(lba, disk);

    uint32_t readLocation = (uint32_t)read_bytes;

    DataAddressPacket dap;

    dap.sizeDAP = 0x10;
    dap.reserved = 0;
    dap.lba = lba;
    dap.sector_count = no_of_sectors;
    dap.segment = readLocation / 16;
    dap.offset = readLocation % 16;

    int retry_count = 3;
    bool error = 0;
    bool isFirst = true;

    for(int retry_no = 0; (retry_no < retry_count) && ((error != 0) || isFirst); retry_no++)
    {
        isFirst = false;

        //error = !x86_read_disk(disk.drive_id, chs.cyl, chs.sector, chs.head, no_of_sectors, read_bytes);

        //printf("Values: lba: %u, count: %u, address: 0x%x, drive ID: %u\n", lba, no_of_sectors, read_bytes, disk.drive_id);

        error = !x86_read_disk_ext(disk.drive_id, &dap);

        if(error)
        {
            if(!x86_reset_disk(disk.drive_id)) // Disk Reset failed
            {
                disk_operation_failed("Disk Reset Failed!\n");
                return false;
            }
        }
    }

    if(error) // all retries failed
    {
        disk_operation_failed("Disk Read Sectors Failed!\n");

        printf("Values: lba: %u, count: %u, address: %x, drive ID: %u\n", lba, no_of_sectors, read_bytes, disk.drive_id);
        return false;
    }

    // Success
    return true;
}
