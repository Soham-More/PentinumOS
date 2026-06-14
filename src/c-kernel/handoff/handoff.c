#include "handoff.h"

u32 hdf_mmap_entry_type(E820_MMAP_ENTRY* entry) {
    
    switch (entry->regionType)
    {
        // MMAP_TYPE_USED_CRITICAL
        case 0x0: return PNODE_ON_RAM | PNODE_BLOCKED;
        // MMAP_TYPE_FREE
        case 0x1: return PNODE_ON_RAM | PNODE_FREE;
        // MMAP_TYPE_RESERVED
        case 0x2: return PNODE_BLOCKED;
        // MMAP_TYPE_ACPI_RECLAIMABLE
        case 0x3: return PNODE_ON_RAM | PNODE_ACPI;
        // MMAP_TYPE_ACPI_NVS
        case 0x4: return PNODE_ON_RAM | PNODE_ACPI | PNODE_BLOCKED;
        // MMAP_TYPE_BAD
        case 0x5: return PNODE_BLOCKED;
        default: return PNODE_BLOCKED;
    }
}
