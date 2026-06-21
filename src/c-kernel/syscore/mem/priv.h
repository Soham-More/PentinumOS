#pragma once

#include <includes.h>
#include <defs/page_status.h>

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

