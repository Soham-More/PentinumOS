#pragma once

// page flags

// page status flags
// a parent node is given OR of the status of its children
#define PNODE_FREE        ((u8)0x01) // free leaf
#define PNODE_USED        ((u8)0x02) // used leaf, can be freed
#define PNODE_BLOCKED     ((u8)0x04) // this node is bad/reserved - cannot be freed
// status descriptor flags
#define PNODE_ACPI        ((u8)0x00) // this node is in use by ACPI(can be requested back unless PNODE_BLOCKED) - only valid if paired with BLOCKED/USED
// status mask
#define PNODE_STATUS_MASK (PNODE_FREE | PNODE_USED | PNODE_BLOCKED | PNODE_ACPI)

// page mapping flags
#define PNODE_UNMAPPED   ((u8)0x08) // this node is not mapped to anything
#define PNODE_ON_RAM     ((u8)0x10) // this node is available on ram
#define PNODE_IO_MAPPED  ((u8)0x20) // this node is mapped to a io device
#define PNODE_BLK_MAPPED ((u8)0x40) // this node is mapped to a block device
// page map mask
#define PNODE_MAP_MASK   (PNODE_UNMAPPED | PNODE_BLK_MAPPED | PNODE_IO_MAPPED | PNODE_ON_RAM)
