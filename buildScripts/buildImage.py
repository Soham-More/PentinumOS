from decimal import Decimal
import subprocess
import os
import re

import sys

from subprocess import check_call

def to_bytes(size: int, Unit: str):

    Unit = Unit.lower()

    if Unit == 'mib':
        return int(size * 1024 * 1024)
    elif Unit == 'kib':
        return int(size * 1024)
    elif Unit == 'gib':
        return int(size * 1024 * 1024 * 1024)

imageFilePath = sys.argv[1]
imageSize     = to_bytes(int(sys.argv[2]), sys.argv[3])

MBR = sys.argv[4]
VBR = sys.argv[5]
Stage2 = sys.argv[6]

sectorSize = 512

imageSectorCount = imageSize // sectorSize

def imageFile():
    return os.fdopen(os.open(imageFilePath, os.O_WRONLY | os.O_CREAT), 'wb+')

# generates image file with given size
def gen_image_file():
    with open(imageFilePath, 'wb') as img:
        img.write(bytes(imageSectorCount * sectorSize))
        img.close()

# writes MBR to sector 0 of image
def write_MBR(fileMBR : str):

    partition_table = []
    partition_table_location = 0x1b8
    partition_table_size = 0x200 - 0x1b8

    # save partition table
    with open(imageFilePath, 'rb') as img:
        img.seek(partition_table_location, 0)
        partition_table = img.read(partition_table_size)

    with imageFile() as img:
        with open(fileMBR, "rb") as MBR:
            img.write(MBR.read())
    
    # write back partition table
    with imageFile() as img:
        img.seek(partition_table_location, 0)
        img.write(partition_table)

# PARTITION_A:
#    .isActive: db 0x00
#    .first_CHS: db 0x00, 0x21, 0x00
#    .type: db 0x0b
#    .last_CHS: db 0xf0, 0xe4, 0xcc
#    .lba: db 20, 00, 00, 00
#    .sector_count: db 0xe0, 0x7f, 0xee, 0x00

def make_partition(reserved_sectors: int):

    isActive = bytes([0x00])
    firstCHS = bytes([0x00, 0x21, 0x00])
    type     = bytes([0x0b])
    lastCHS  = bytes([0xf0, 0xe4, 0xcc])
    lba      = bytes([0x20, 0x00, 0x00, 0x00])
    sector_count = bytes([0xe0, 0x7f, 0xee, 0x00])

    partition_table_location = 0x1be
    VBR_reserved_sector_value_location = 0x4000 + 0x0E
    VBR_hidden_sector_value_location = 0x4000 + 0x1C

    VBR_sector_count_value_location_16 = 0x4000 + 0x13
    VBR_sector_count_value_location_32 = 0x4000 + 0x20

    VBR_sector_per_fat_value_location = 0x4000 + 0x24

    with imageFile() as img:
        # write at partition table location
        img.seek(partition_table_location, 0)

        img.write(isActive)
        img.write(firstCHS)
        img.write(type)
        img.write(lastCHS)
        img.write(lba)
        img.write(sector_count)

        # write at reserved sector location
        img.seek(VBR_reserved_sector_value_location, 0)

        img.write(reserved_sectors.to_bytes(2, 'little'))

        # write at hidden sector location
        img.seek(VBR_hidden_sector_value_location, 0)

        img.write((0x4000 // sectorSize).to_bytes(4, 'little'))

        sectorCount = (imageSize - 0x4000) // sectorSize

        # more than 16 bit value, set 32 bit field
        if sectorCount > 65535:
            img.seek(VBR_sector_count_value_location_16, 0)
            img.write((0).to_bytes(2, 'little'))

            img.seek(VBR_sector_count_value_location_32, 0)
            img.write(sectorCount.to_bytes(4, 'little'))
        else:
            img.seek(VBR_sector_count_value_location_16, 0)
            img.write(sectorCount.to_bytes(2, 'little'))

            img.seek(VBR_sector_count_value_location_32, 0)
            img.write((0).to_bytes(4, 'little'))

        freeSectorCount = sectorCount - reserved_sectors

        sectorsPerFAT = (freeSectorCount * 32 + sectorSize - 1) // sectorSize

        img.seek(VBR_sector_per_fat_value_location, 0)
        img.write(sectorsPerFAT.to_bytes(4, 'little'))

# write VBR
def write_VBR(fileVBR: str, offset: int):

    VBR_reserved_sector_value_location = 0x4000 + 0x0E
    VBR_hidden_sector_value_location = 0x4000 + 0x1C

    VBR_sector_count_value_location_16 = 0x4000 + 0x13
    VBR_sector_count_value_location_32 = 0x4000 + 0x20

    VBR_sector_per_fat_value_location = 0x4000 + 0x24

    reserved_sector_count = []
    hidden_sector_count = []

    sectorCount16 = []
    sectorCount32 = []

    sectorsPerFat = []

    FAT32_table = 0x4000
    table = []

    with open(imageFilePath, 'rb') as img:

        #img.seek(FAT32_table, 0)
        #table = img.read(90)

        # store reserved sector
        img.seek(VBR_reserved_sector_value_location, 0)
        reserved_sector_count = img.read(2)

        # store hidden sector
        img.seek(VBR_hidden_sector_value_location, 0)
        hidden_sector_count = img.read(4)

        # store sectorCount
        img.seek(VBR_sector_count_value_location_16, 0)
        sectorCount16 = img.read(2)
        img.seek(VBR_sector_count_value_location_32, 0)
        sectorCount32 = img.read(4)

        # store sectors per fat value
        img.seek(VBR_sector_per_fat_value_location, 0)
        sectorsPerFat = img.read(4)

    # write VBR
    with imageFile() as img:
        with open(fileVBR, "rb") as VBR:
            img.seek(offset * sectorSize, 0)

            img.write(VBR.read())

    with imageFile() as img:

        #img.seek(FAT32_table, 0)
        #img.write(table)

        # store reserved sector
        img.seek(VBR_reserved_sector_value_location, 0)
        img.write(reserved_sector_count)

        # store hidden sector
        img.seek(VBR_hidden_sector_value_location, 0)
        img.write(hidden_sector_count)

        # store sectorCount
        img.seek(VBR_sector_count_value_location_16, 0)
        img.write(sectorCount16)
        img.seek(VBR_sector_count_value_location_32, 0)
        img.write(sectorCount32)

        # store sectors per fat value
        img.seek(VBR_sector_per_fat_value_location, 0)
        img.write(sectorsPerFat)

# write stage2
def write_stage2(fileVBR: str, offset: int):
    with imageFile() as img:
        with open(fileVBR, "rb") as VBR:
            img.seek(offset * sectorSize, 0)

            img.write(VBR.read())

# uses parted to make a MBR partition
# write FAT32 to image
def init_image(reserved_sectors: int):
    check_call(["./buildScripts/part.sh", imageFilePath, "16KiB", "-1s", str(reserved_sectors), "ROS"])

# make disk image
def build_disk():
    MBRfile = MBR
    VBRfile = VBR
    st2file = Stage2

    st2size = (os.stat(st2file).st_size + sectorSize - 1) // sectorSize

    partition_begin = 0x4000 // sectorSize

    gen_image_file()

    init_image(st2size + 10)

    write_MBR(MBRfile)

    # write VBR
    write_VBR(VBRfile, partition_begin)

    # write after VBR + FSInfo + backup
    write_stage2(st2file, partition_begin + 10)

build_disk()
