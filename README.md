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

# Planned Features
- Task Scheduler and Multi Tasking
- DOS like terminal
- USB Devices
  - Mouse
  - Keyboard
- Better Stack Trace during a crash

# Known Bugs
- Uncommenting a printf line in elf.c in bootloader code causes UHCI driver in kernel to crash.

