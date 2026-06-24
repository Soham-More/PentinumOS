
#include "pci.h"

#include <arch/i686.h>

#define PCI_BAR_MAP_NONE   0
#define PCI_BAR_MAP_MEMORY 1
#define PCI_BAR_MAP_IO     2

#define PCI_CONFIG_ADDRESS 0xCF8
#define PCI_CONFIG_DATA    0xCFC

typedef struct device_info {
    u8 bus;
    u8 device_no;

    bool isMultiFunction;

    pci_device functions[8];
} device_info;
typedef struct bar_info {
    void* address;
    size_t size;
    bar_type type;
} bar_info;

struct {
    pci_device* head;
    pci_device* tail;
    usize pci_device_count;
} g_pci_ctx;

bool pci_is_dev_info_valid(device_info* devInfo) { return devInfo->functions[0].vendorID != 0xFFFF; }

//static std::vector<device_info> pci_devices;

pci_device getDeviceFunction(u8 bus, u8 device, u8 function);
device_info getDevice(u8 bus, u8 device);
void checkFunction(device_info* deviceInfo, u8 function);
void checkDevice(device_info* deviceInfo);
void checkBus(u8 bus);

u32 pci_config_read_u32(u8 bus_no, u8 device_no, u8 function, u8 register_offset)
{
    u32 lbus = bus_no;
    u32 ldevice = device_no & 0x1F;
    u32 lfunc = function & 0x7;
    u32 loffset = register_offset & 0xFC;

    // (1 << 31): enable bit
    u32 configAddress = (1 << 31) | (lbus << 16) | (ldevice << 11) | (lfunc << 8) | (loffset);

    // send address
    x86_outl(PCI_CONFIG_ADDRESS, configAddress);

    // get register
    u32 configData = x86_inl(PCI_CONFIG_DATA);
    return configData;
}
u16 pci_config_read_u16(u8 bus_no, u8 device_no, u8 function, u8 register_offset)
{
    u8 dword_offset = (register_offset & 0x2) >> 1;

    u32 dword = pci_config_read_u32(bus_no, device_no, function, register_offset);

    return (dword >> (dword_offset * 16)) & 0xFFFF;
}
u8 pci_config_read_u8(u8 bus_no, u8 device_no, u8 function, u8 register_offset)
{
    u8 dword_offset = (register_offset & 0x3);

    u32 dword = pci_config_read_u32(bus_no, device_no, function, register_offset);

    return (dword >> (dword_offset * 8)) & 0xFF;
}

void pci_config_write_u32(u8 bus_no, u8 device_no, u8 function, u8 register_offset, u32 value)
{
    u32 lbus = bus_no;
    u32 ldevice = device_no & 0x1F;
    u32 lfunc = function & 0x7;
    u32 loffset = register_offset & 0xFC;

    // (1 << 31): enable bit
    u32 configAddress = (1 << 31) | (lbus << 16) | (ldevice << 11) | (lfunc << 8) | (loffset);

    // send address
    x86_outl(PCI_CONFIG_ADDRESS, configAddress);
    x86_outl(PCI_CONFIG_DATA, value);
}
void pci_config_write_u16(u8 bus_no, u8 device_no, u8 function, u8 register_offset, u16 value)
{
    u8 dword_offset = (register_offset & 0x2) >> 1;

    // preserve other bits
    u32 curr_value = pci_config_read_u32(bus_no, device_no, function, register_offset);
    pci_config_write_u32(bus_no, device_no, function, register_offset, (curr_value & ~(UINT16_MAX)) | value);
}
void pci_config_write_u8 (u8 bus_no, u8 device_no, u8 function, u8 register_offset, u8 value)
{
    u8 dword_offset = (register_offset & 0x2) >> 1;

    // preserve other bits
    u32 curr_value = pci_config_read_u32(bus_no, device_no, function, register_offset);
    pci_config_write_u16(bus_no, device_no, function, register_offset, (curr_value & ~(UINT8_MAX)) | value);
}

pci_device getDeviceFunction(u8 bus, u8 device_no, u8 function)
{
    pci_device finfo;

    finfo.function = function;
    finfo.bus = bus;
    finfo.device_no = device_no;

    finfo.vendorID = pci_config_read_u16(bus, device_no, function, 0x00);

    if(finfo.vendorID != 0xFFFF)
    {
        finfo.deviceID = pci_config_read_u16(bus, device_no, function, 0x01);

        finfo.revisionID = pci_config_read_u8(bus, device_no, function, 0x08);
        finfo.progIF     = pci_config_read_u8(bus, device_no, function, 0x09);
        finfo.subClass   = pci_config_read_u8(bus, device_no, function, 0x0A);
        finfo.classCode  = pci_config_read_u8(bus, device_no, function, 0x0B);

        // mask multi function bit
        finfo.headerType = pci_config_read_u8(bus, device_no, function, 0x0E) & (~0x80);
    }

    finfo.portBase = nullptr;

    return finfo;
}
device_info getDevice(u8 bus, u8 device)
{
    device_info dinfo;

    dinfo.bus = bus;
    dinfo.device_no = device;

    dinfo.functions[0] = getDeviceFunction(bus, device, 0);

    u8 multiFunction = pci_config_read_u8(bus, device, 0, 0x0E) & 0x80;

    if(multiFunction > 0) dinfo.isMultiFunction = true;
    else dinfo.isMultiFunction = false;

    if(dinfo.isMultiFunction)
    {
        for(u8 i = 1; i < 8; i++)
        {
            dinfo.functions[i] = getDeviceFunction(bus, device, i);
        }
    }

    return dinfo;
}

void add_device(pci_device* _device)
{
    //log_warn("adding device: {s} (bus:{u}, dev:{u}, func:{u})\n", _device->name, _device->bus, _device->device_no, _device->function);

    format(_device->name, sizeof(_device->name), "dev{u}", g_pci_ctx.pci_device_count);

    pci_device* new_device = (pci_device*)malloc(kmt_get_heap(), sizeof(pci_device));
    panic_on_err_ptr(new_device, "Failed to allocate memory for new PCI device");
    *new_device = *_device;

    if(g_pci_ctx.head == nullptr) {
        g_pci_ctx.head = new_device;
        g_pci_ctx.tail = new_device;
    } else {
        g_pci_ctx.tail->next = new_device;
        g_pci_ctx.tail = new_device;
    }
    g_pci_ctx.pci_device_count++;
}

void checkFunction(device_info* deviceInfo, u8 function) {
    u8 secondaryBus;

    pci_device* f0 = &deviceInfo->functions[0];
    
    if ((f0->classCode == 0x6) && (f0->subClass == 0x4)) {
        secondaryBus = pci_config_read_u8(deviceInfo->bus, deviceInfo->device_no, function, 0x19);
        checkBus(secondaryBus);
    }
}
void checkDevice(device_info* deviceInfo) {
    if(!pci_is_dev_info_valid(deviceInfo)) return;

    checkFunction(deviceInfo, 0);

    // add device to list
    //pci_devices.push_back(deviceInfo);
    add_device(&deviceInfo->functions[0]);

    if(deviceInfo->isMultiFunction) {
        // multi function device
        for(u8 func = 1; func < 8; func++) {
            if(pci_is_dev_valid(&deviceInfo->functions[func])) {
                checkFunction(deviceInfo, func);
            }
        }
    }
}
void checkBus(u8 bus) {
    for(u8 device = 0; device < 32; device++) {
        device_info Dinfo = getDevice(bus, device);

        if(pci_is_dev_info_valid(&Dinfo)) {
            checkDevice(&Dinfo);
        }
    }
}

void pci_enumerate() {
    device_info device0 = getDevice(0, 0);

    if (!device0.isMultiFunction) {
        // Single PCI host controller
        checkBus(0);
    } else {
        // Multiple PCI host controllers
        for (u8 function = 0; function < 8; function++) {
            if (device0.functions[function].vendorID != 0xFFFF) break;
            checkBus(function);
        }
    }
}

void pci_pretty_print() {
    // lock the tty for exclusive access
    // so that other threads don't interleave their output with this
    LOG_AQUIRE_SCOPED_LOCK();
    log_debug("PCI Devices:\n");
    log_debug("  BUS | Device | Class | Subclass | Prog IF | Vendor ID | Device ID\n");

    pci_device* child = g_pci_ctx.head;

    while(child != nullptr)
    {
        pci_device* f0 = child;

        log_debug("  {3u%> } | {6u%> } | {5h%> } | {8u%> } | {7h%> } | {9h%> } | {9h%> }\n", 
            f0->bus, f0->device_no, 
            f0->classCode, f0->subClass, 
            f0->progIF, f0->vendorID, 
            f0->deviceID
        );
        
        child = child->next;
    }
}

err_t pci_init()
{
    //i32 error_code = vfs::add_vnode(gvfs, "/hw/", vfs::make_node("pci", true));
    //// propogate error
    //if(error_code != OP_SUCCESS) { log_error("[PCI] Failed to make pci vnode in vfs!\ns"); return error_code;}

    pci_enumerate();

    return ESUCCESS;
}

bar_info getDeviceBAR(pci_device* pciDevice, u8 bar) {
    u8 offset = 0x10 + (bar * 4);
    pci_device* function = pciDevice;

    // preserve BAR Value
    u32 BAR = PCI_DEV_CONFIG_READ(u32, function, offset);

    u32 BAR_size = 0;

    // disable memory decode and IO decode
    u16 commandByte = PCI_DEV_CONFIG_READ(u16, function, 0x04);
    commandByte &= ~(0x03);
    PCI_DEV_CONFIG_WRITE(u32, function, 0x04, commandByte);

    // write all 1s
    PCI_DEV_CONFIG_WRITE(u32, function, offset, UINT32_MAX);

    u32 sizeMask = PCI_DEV_CONFIG_READ(u32, function, offset);

    // if it is 0, it is a memory mapped bar
    if((sizeMask & 0x01) == 0) {
        if((BAR & 0x6) == 0x00) {
            BAR_size = ~(sizeMask & ~((u32)0xF));
        }
        // else if it is 16-bit
        else if((BAR & 0x6) == 0x2) {
            BAR_size = ~(sizeMask & ~((u16)0xF));
        }
        // else if it is 64-bit
        // currently fails
        else if((BAR & 0x6) == 0x4) {
            // enable memory decode and IO decode
            commandByte = PCI_DEV_CONFIG_READ(u16, function, 0x04);
            commandByte |= 0x03;
            PCI_DEV_CONFIG_WRITE(u16, function, 0x04, commandByte);

            log_error("\nERROR: Detected 64-bit BAR Device(not supported): bus:%d, device_no:%f, functionid:%d\n", function->bus, function->device_no, function->function);
            return (bar_info){.address = nullptr, .size = 0, .type = PCI_BAR_MAP_NONE};
        }
    }
    // IO Mapped BAR
    else {
        // enable memory decode and IO decode
        commandByte = PCI_DEV_CONFIG_READ(u16, function, 0x04);
        commandByte |= 0x03;
        PCI_DEV_CONFIG_WRITE(u16, function, 0x04, commandByte);

        // restore bar settings
        PCI_DEV_CONFIG_WRITE(u32, function, offset, BAR & ~0x3);

        return (bar_info){.address = (void*)(BAR & ~0x3), .size = ~(sizeMask & ~3) + 1, .type = PCI_BAR_MAP_IO};
    }

    // enable memory decode and IO decode
    commandByte = PCI_DEV_CONFIG_READ(u16, function, 0x04);
    commandByte |= 0x03;
    PCI_DEV_CONFIG_WRITE(u16, function, 0x04, commandByte);

    PCI_DEV_CONFIG_WRITE(u32, function, offset, BAR);

    return (bar_info){.address = (void*)(BAR & ~(0xF)), .size = ~(sizeMask & ~0xF), .type = PCI_BAR_MAP_MEMORY};
}

bool pci_is_dev_valid(pci_device* dev) {
    return dev->vendorID != 0xFFFF;
}
err_t alloc_bar(pci_device* dev, u8 barID, void* addr, bool isFrameBuffer) {
    u8 offset = 0x10 + (barID * 4);

    // preserve BAR Value
    u32 BAR = PCI_DEV_CONFIG_READ(u32, dev, offset);

    u32 BAR_size = 0;

    // disable memory decode and IO decode
    u16 commandByte = PCI_DEV_CONFIG_READ(u16, dev, 0x04);
    commandByte &= ~(0x03);
    PCI_DEV_CONFIG_WRITE(u32, dev, 0x04, commandByte);

    // write all 1s
    PCI_DEV_CONFIG_WRITE(u32, dev, offset, UINT32_MAX);

    u32 sizeMask = PCI_DEV_CONFIG_READ(u32, dev, offset);

    // if it is 0, it is a memory mapped bar
    if((sizeMask & 0x01) == 0) {
        if((BAR & 0x6) == 0x00) { BAR_size = ~(sizeMask & ~((u32)0xF)); }
        // else if it is 16-bit
        else if((BAR & 0x6) == 0x2) { BAR_size = ~(sizeMask & ~((u16)0xF)); }
        // else if it is 64-bit
        // currently fails
        else if((BAR & 0x6) == 0x4) {
            // enable memory decode and IO decode
            commandByte = PCI_DEV_CONFIG_READ(u16, dev, 0x04);
            commandByte |= 0x03;
            PCI_DEV_CONFIG_WRITE(u16, dev, 0x04, commandByte);

            log_error("\nERROR: Detected 64-bit BAR Device(not supported): bus:%d, device_no:%f, functionid:%d\n", dev->bus, dev->device_no, dev->function);
            return EUNSUPPORTED;
        }
    }
    // IO Mapped BAR
    else
    {
        // restore bar settings
        PCI_DEV_CONFIG_WRITE(u32, dev, offset, BAR & ~0x3);

        return EUNSUPPORTED;
    }

    // now allocate it some memory
    u32 page_flags = X86_PAGE_PRESENT | X86_PAGE_RW;

    // BAR is prefetchable
    if((BAR & (1 << 3)) > 0)
    {
        if(isFrameBuffer) {
            // mark as Write Comb
            // TODO
            log_warn("TODO: mark BAR as Write Comb");
        }
        else {
            // then it is write through
            page_flags = page_flags | X86_PAGE_WRITETHROUGH;
        }
    }
    // mark as Uncachable
    else
    {
        page_flags = page_flags | X86_PAGE_DISABLE_CACHING;
    }

    err_t err = syscore_alloc_mmio_pages((BAR_size + 1) / X86_PAGE_SIZE, (ptr_t)addr, page_flags);
    if (err != ESUCCESS) {
        log_error("Failed to allocate MMIO pages for BAR");
        return err;
    }
    u32 new_bar = kmt_get_phys_addr((ptr_t)addr);

    // restore bar settings
    new_bar |= BAR & 0xF;

    PCI_DEV_CONFIG_WRITE(u32, dev, offset, new_bar);

    // enable memory decode and IO decode
    commandByte = PCI_DEV_CONFIG_READ(u16, dev, 0x04);
    commandByte |= 0x03;
    PCI_DEV_CONFIG_WRITE(u16, dev, 0x04, commandByte);

    u32 read_word = PCI_DEV_CONFIG_READ(u32, dev, offset);

    return ESUCCESS;
}
void pci_outb(pci_device* dev, u16 register_offset, u8 value) {
    x86_outw(dev->portBase + register_offset, value);
}
u8 pci_inb(pci_device* dev, u16 register_offset) {
    return x86_inw(dev->portBase + register_offset);
}
void pci_outw(pci_device* dev, u16 register_offset, u16 value) {
    x86_outw(dev->portBase + register_offset, value);
}
u16 pci_inw(pci_device* dev, u16 register_offset) {
    u32 val = x86_inw(dev->portBase + register_offset);
    return val;
}
void pci_outl(pci_device* dev, u16 register_offset, u32 value) {
    x86_outl(dev->portBase + register_offset, value);
}
u32 pci_inl(pci_device* dev, u16 register_offset) {
    u32 val = x86_inl(dev->portBase + register_offset);
    return val;
}

void pci_setup(pci_device* dev) {
    // the device is already setup
    if(dev->portBase != nullptr) return;

    for(size_t i = 0; i < 6; i++) {
        bar_info bInfo = getDeviceBAR(dev, i);
        if(bInfo.type == PCI_BAR_MAP_IO) {
            dev->portBase = (u32)(bInfo.address);
        }
    }
}

/*
std::vector<device*> get_all_devices()
{
    std::vector<device*> devices;

    for(size_t i = 0; i < pci_devices.size(); i++)
    {
        devices.push_back(&pci_devices[i].functions[0]);

        if(!pci_devices[i].isMultiFunction) continue;

        for(size_t j = 1; j < 8; j++)
        {
            devices.push_back(&pci_devices[i].functions[j]);
        }
    }

    return devices;
}
*/
/*
device *get_device(u8 classCode, u8 subClass)
{
    device* pciDevice = nullptr;

    for(size_t i = 0; i < pci_devices.size(); i++)
    {
        if(pci_devices[i].functions[0].classCode == classCode && pci_devices[i].functions[0].subClass == subClass)
        {
            pciDevice = &pci_devices[i].functions[0];
        }

        if(!pci_devices[i].isMultiFunction) continue;

        for(size_t j = 1; j < 8; j++)
        {
            if(pci_devices[i].functions[j].classCode == classCode && pci_devices[i].functions[j].subClass == subClass)
            {
                pciDevice = &pci_devices[i].functions[j];
            }
        }
    }
    if(!pciDevice) return nullptr;

    for(size_t i = 0; i < 6; i++)
    {
        bar_info bInfo = getDeviceBAR(pciDevice, i);
        if(bInfo.type == bar_type::IO_MAPPED)
        {
            pciDevice->portBase = reinterpret_cast<u32>(bInfo.address);
        }
    }

    return pciDevice;
}
*/


