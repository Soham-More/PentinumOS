symbol-file ../build/kernel.elf
set disassembly-flavor intel
set print pretty on
target remote | qemu-system-i386 -S -gdb stdio -m 128 -hda ../build/image.img