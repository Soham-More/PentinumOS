#pragma once

#include <includes.h>

// page flags

// page status flags
#define PNODE_FREE        ((u8)0x00) // free
#define PNODE_AVAIL       ((u8)0x01) // atleast one descendant is PNODE_FREE
#define PNODE_USED        ((u8)0x02) // node is used
#define PNODE_BLOCKED     ((u8)0x03) // this node is bad/reserved - cannot be freed
// status descriptor flags
#define PNODE_ACPI        ((u8)0x04) // this node is in use by ACPI(can be requested back unless PNODE_BLOCKED) - only valid if paired with BLOCKED/USED
// status mask
#define PNODE_STATUS_MASK (PNODE_BLOCKED | PNODE_ACPI)

// page mapping flags
#define PNODE_UNMAPPED   ((u8)0x00) // this node is not mapped to anything
#define PNODE_ON_RAM     ((u8)0x08) // this node is available on ram
#define PNODE_IO_MAPPED  ((u8)0x10) // this node is mapped to a io device
#define PNODE_BLK_MAPPED ((u8)0x18) // this node is mapped to a block device
#define PNODE_MIX_MAPPED ((u8)0x20) // this node has descendants with different mappings
// page map mask
#define PNODE_MAP_MASK   (PNODE_UNMAPPED | PNODE_BLK_MAPPED | PNODE_IO_MAPPED | PNODE_ON_RAM | PNODE_MIX_MAPPED)

// page flag mask
#define PNODE_FLAG_MASK  (PNODE_STATUS_MASK | PNODE_MAP_MASK)

// node flags
#define PNODE_BUDDY_OFFSET_NEGATIVE ((u8)0x40) // the buddy of this node is located just before this node(in memory)

// searching flags
#define PNODE_SEARCH_ANY_STATUS  ((u16)0x0100) // match to any status
#define PNODE_SEARCH_ANY_MAPPING ((u16)0x0200) // match to any mapping
#define PNODE_SEARCH_MODIFY      ((u16)0x0400) // permission to modify the tree(i.e split nodes to match order if flags matched)
#define PNODE_SEARCH_REVERSE     ((u16)0x0800) // start the search from max address to 0

