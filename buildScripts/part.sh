#!/bin/sh

# $1: image file relative to this script location
# $2: partition start
# $3: partition end, -1s for end of image
# $4: reserved sectors in fat32
# $5: volume name

parted -s $1 -- mklabel msdos
parted -s $1 -- mkpart primary fat32 $2 $3
losetup --partscan --find --show $1 > $6
mkfs.fat $(cat $6)p1 -F 32 -R $4 -n $5
losetup -d $(cat $6)
