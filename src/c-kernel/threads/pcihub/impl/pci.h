#pragma once

#include <threads/includes.h>

typedef struct bar_info bar_info;
typedef u32 bar_type;

// Commands
#define PCI_CMD_IO_SPACE                ((u16)0x001)
#define PCI_CMD_MEMORY_SPACE            ((u16)0x002)
#define PCI_CMD_BUS_MASTER              ((u16)0x004)
#define PCI_CMD_SPECIAL_CYCLES          ((u16)0x008)
#define PCI_CMD_MW_INV_ENABLE           ((u16)0x010)
#define PCI_CMD_VGA_PALLETE_SNOOP       ((u16)0x020)
#define PCI_CMD_PARITY_ERROR_RESPONSE   ((u16)0x040)
#define PCI_CMD_SERR_ENABLE             ((u16)0x080)
#define PCI_CMD_FAST_BACKTOBACK_ENABLE  ((u16)0x100)
#define PCI_CMD_INT_DISABLE             ((u16)0x200)

// Status
#define PCI_STATUS_INT_STATUS       0x08
#define PCI_STATUS_CAPABILITY_LIST  0x10
#define PCI_STATUS_CAPABLE_66MHz    0x20
#define PCI_STATUS_CAPABLE_FAST_BACKTOBACK   0x40
#define PCI_STATUS_MASTER_DATA_PARITY_ERROR  0x80
#define PCI_STATUS_SIG_TARGET_ABORT  0x400
#define PCI_STATUS_REV_TARGET_ABORT  0x800
#define PCI_STATUS_REV_MASTER_ABORT  0x1000
#define PCI_STATUS_SIG_SYSTEM_ERROR  0x2000
#define PCI_STATUS_DETECTED_PARITY_ERROR  0x4000

u32 pci_config_read_u32(u8 bus_no, u8 device_no, u8 function, u8 register_offset);
u16 pci_config_read_u16(u8 bus_no, u8 device_no, u8 function, u8 register_offset);
u8 pci_config_read_u8(u8 bus_no, u8 device_no, u8 function, u8 register_offset);

void pci_config_write_u32(u8 bus_no, u8 device_no, u8 function, u8 register_offset, u32 value);
void pci_config_write_u16(u8 bus_no, u8 device_no, u8 function, u8 register_offset, u16 value);
void pci_config_write_u8(u8 bus_no, u8 device_no, u8 function, u8 register_offset, u8 value);

#define PCI_DEV_CONFIG_READ(type, dev, register_offset) \
    pci_config_read_##type(dev->bus, dev->device_no, dev->function, register_offset)
#define PCI_DEV_CONFIG_WRITE(type, dev, register_offset, value) \
    pci_config_write_##type(dev->bus, dev->device_no, dev->function, register_offset, value)

typedef struct pci_device {
    char name[16];

    u8 bus;
    u8 device_no;
    u8 function;

    u16 vendorID;
    u16 deviceID;
    u8  revisionID;

    u8 classCode;
    u8 subClass;
    u8 progIF;

    u32 portBase;
    u32 size;

    u8 headerType;

    bar_type type;

    struct pci_device* next;
} pci_device;

err_t alloc_bar(pci_device* dev, u8 barID, void* addr, bool isFrameBuffer);

void pci_outb(pci_device* dev, u16 register_offset, u8 value);
u8 pci_inb(pci_device* dev, u16 register_offset);

void pci_outw(pci_device* dev, u16 register_offset, u16 value);
u16 pci_inw(pci_device* dev, u16 register_offset);

void pci_outl(pci_device* dev, u16 register_offset, u32 value);
u32 pci_inl(pci_device* dev, u16 register_offset);

bool pci_is_dev_valid(pci_device* dev);

// setup the device
void pci_setup(pci_device* dev);

void pci_enumerate();

void pci_pretty_print();

err_t pci_init();

//std::vector<device*> get_all_devices();
//device* get_device(u8 classCode, u8 subClass);

