symbol-file ../build/kernel.elf
set disassembly-flavor intel
set print pretty on
target remote | qemu-system-i386 -S -gdb stdio -m 128 -drive if=none,id=stick,format=raw,file=../build/image.img \
                        -device usb-ehci,id=ehci                              \
                        -device usb-storage,bus=ehci.0,drive=stick