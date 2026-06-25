# PentinumOS
An OS that targets an old Compaq computer featuring a Pentinum 4 processor

# Building
To build the source code, you need to install the toolchain:
```bash
sudo apt-get install texinfo
apt-get install libmpc-dev
make toolchain
```
Then
```bash
make
```

# Current Features
## C++ Kernel(src/pos-kernel/)
- PS/2 Mouse and Keyboard Support[Done]
- PCI Device Enumeration [Done]
- Filesystem support [Basic]
  - FAT32 [Basic, Read Only]
- USB Support [Basic]
  - UHCI
- USB Devices [Basic]
  - Mass Storage Device
- Boot from USB [Done, Default]
- Heap Memory Management [Basic]
- VFS [Basic]

## C Kernel(src/c-kernel/)
- [x] Boot from USB [Done, Default]
- [x] Heap Memory Management (Basic)
- [x] x86 Paging setup
- [x] Buddy Page Allocator
- [x] Multitasking (Basic)
- [x] syscore thread (currently does memory mangement)
- [x] PCI Bus thread + device enumeration
- [ ] USB device driver thread (UHCI)
- [ ] VFS (basic)
- [ ] FAT32 (readonly, USB stick)

# Known Bugs
- Uncommenting a printf line in elf.c in bootloader code causes UHCI driver in kernel to crash. [Fixed]

