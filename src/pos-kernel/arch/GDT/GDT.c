#include "GDT.h"

#include "TSS.h"

typedef struct
{
    u16 limit_low;
    u16 base_low;
    u8  base_mid;
    u8  access_byte;
    u8  flags_limit_high;
    u8  base_high;
} _packed GDT_ENTRY;

typedef struct
{
    u16 limit;
    GDT_ENTRY* gdt_entries;
} _packed GDT_DESC;

enum GDT_ACCESS_TYPE
{
    GDT_PRESENT_ENTRY   = 0x80,

    GDT_PRIVILEGE_RING0 = 0x00,
    GDT_PRIVILEGE_RING1 = 0x20,
    GDT_PRIVILEGE_RING2 = 0x40,
    GDT_PRIVILEGE_RING3 = 0x60,

    GDT_CODE_SEGMENT    = 0x18,
    GDT_DATA_SEGMENT    = 0x10,
    GDT_TSS_DESC        = 0x00,

    GDT_CONFORMING      = 0x04,
    GDT_DIRECTION_UP    = 0x00,
    GDT_DIRECTION_DOWN  = 0x04,

    GDT_ALLOW_CODE_READ  = 0x02,
    GDT_ALLOW_DATA_WRITE = 0x02
};

enum GDT_FLAGS
{
    GDT_GRANULARITY_4K = 0x80,
    GDT_GRANULARITY_1B = 0x00,

    GDT_16BIT_SEGMENT  = 0x00,
    GDT_32BIT_SEGMENT  = 0x40,
    GDT_64BIT_SEGMENT  = 0x20,
};

#define _SET_BASE_LO(base)   ((u16)( base & 0xFFFF ))
#define _SET_BASE_MID(base)  ((u8)( (base >> 16) & 0xFF ))
#define _SET_BASE_HI(base)   ((u8)( (base >> 24) & 0xFF ))

#define _SET_FLAGS_LIMIT_HI(flags, limit) (( (limit >> 16) & 0x0F ) | ( flags & 0xF0 ))
#define _SET_LIMIT_LO(limit) ( limit & 0xFFFF )

#define GDT_ENTRY(base, limit, access, flags)  { \
        _SET_LIMIT_LO(limit), \
        _SET_BASE_LO(base), \
        _SET_BASE_MID(base), \
        access, \
        _SET_FLAGS_LIMIT_HI(flags, limit), \
        _SET_BASE_HI(base)\
    }

GDT_ENTRY gdt_entries[] = 
{
    // null desc
    GDT_ENTRY(0, 0, 0, 0),

    // kernel 32 bit code segment
    GDT_ENTRY(0,
        0xFFFF,
        GDT_PRESENT_ENTRY | GDT_PRIVILEGE_RING0 | GDT_CODE_SEGMENT | GDT_ALLOW_CODE_READ,
        GDT_32BIT_SEGMENT | GDT_GRANULARITY_4K
        ),

    // kernel 32 bit data segment
    GDT_ENTRY(0,
        0xFFFF,
        GDT_PRESENT_ENTRY | GDT_PRIVILEGE_RING0 | GDT_DATA_SEGMENT | GDT_ALLOW_DATA_WRITE,
        GDT_32BIT_SEGMENT | GDT_GRANULARITY_4K
        ),
    
    // null desc
    GDT_ENTRY(0, 0, 0, 0),

    // null desc
    GDT_ENTRY(0, 0, 0, 0),

    // null desc
    GDT_ENTRY(0, 0, 0, 0),

    // null desc
    GDT_ENTRY(0, 0, 0, 0),
};

GDT_DESC gdt_desc = { sizeof(gdt_entries) - 1, gdt_entries };

_import void _asmcall i686_load_gdt(GDT_DESC* gdt_desc, u16 code_segment, u16 data_segment);

void i686_init_gdt()
{
    i686_load_gdt(&gdt_desc, i686_GDT_CODE_SEGMENT, i686_GDT_DATA_SEGMENT);
}

void x86_gdt_add_tss(tss_entry_t* tss, u32 gdt_entry_id)
{
    // make sure the wrong entry does not get modified
    if(gdt_entry_id < 3) return;

    gdt_entries[gdt_entry_id] = GDT_ENTRY(((u32)tss), (sizeof(tss_entry_t) - 1), GDT_PRESENT_ENTRY | GDT_PRIVILEGE_RING0 | GDT_CODE_SEGMENT | GDT_ALLOW_CODE_READ, 0);
}
