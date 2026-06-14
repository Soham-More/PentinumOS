#pragma once

#include <includes.h>

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

// page flag mask
#define PNODE_FLAG_MASK  (PNODE_STATUS_MASK | PNODE_MAP_MASK)

// node flags
#define PNODE_BUDDY_OFFSET_NEGATIVE ((u8)0x80) // the buddy of this node is located just before this node(in memory)

// node flags
#define PNODE_ALL_STATUS  (PNODE_FREE | PNODE_USED | PNODE_BLOCKED | PNODE_ACPI)
#define PNODE_ALL_MAPPING (PNODE_UNMAPPED | PNODE_ON_RAM | PNODE_IO_MAPPED | PNODE_BLK_MAPPED)

// searching settings
#define PNODE_SEARCH_MODIFY      ((u16)0x0400) // permission to modify the tree(i.e split nodes to match order if flags matched)
#define PNODE_SEARCH_REVERSE     ((u16)0x0800) // start the search from max address to 0
#define PNODE_FORCE_OVERRIDE     ((u16)0x1000) // allow overriding flags of found node, regarless of the node. NOTE: Can allow ovverriding BLOCKED/ON_RAM flag, so use with caution!

