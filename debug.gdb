symbol-file ./build/kernel.elf
set disassembly-flavor intel
set print pretty on
target remote | qemu-system-i386 -S -gdb stdio -m 128 -drive if=none,id=stick,format=raw,file=./build/image.img \
                        -device piix3-usb-uhci,id=uhci                              \
                        -device usb-storage,bus=uhci.0,drive=stick
