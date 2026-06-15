#include "memory.h"

// allocate page_cnt pages(may allocate more) on memory not on ram
// if status is PNODE_BLK_MAPPED - then mark allocated pages as BLK_MAPPED
// if status is PNODE_IO_MAPPED - then mark allocated pages as IO_MAPPED
// DOES NOT UPDATE THE PTABLE, just marks the pages as mapped in the buddy tree.
const page_alloc_info_t allocate_mapped_pages(u16 status, usize page_cnt);


