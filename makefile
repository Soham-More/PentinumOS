
include buildScripts/config.mk

export OPT_CFLAGS = 

.PHONY: all floppy_image kernel bootloader clean always hexdump

all: floppy_image

include buildScripts/toolchain.mk

dbuild:
	$(MAKE) clean
	$(MAKE) OPT_CFLAGS=-Dr_Debug
	$(MAKE) iso

floppy_image: $(BUILD_DIR)/image.img

$(BUILD_DIR)/image.img: bootloader kernel mount_dir
	@$(IMG_BUILDER) $@ 64 MiB build/MBR.bin build/VBR.bin build/stage2.bin build/device

	@$(MAKE) -f $(IMG_INSTALLER) BUILD_DIR=$(abspath $(BUILD_DIR)) ROOT_FILES=$(abspath $(ROOT_FILES)) IMG_MOUNT=$(abspath $(IMG_MOUNT)) IMAGE_FILE=$@ install

	@chmod a+rwx build/image.img

	@losetup -a

mount_dir:
	@mkdir -p $(IMG_MOUNT)

#@dd if=/dev/zero of=$@ bs=512 count=2880
#@mkfs.fat -F 12 -n "NBOS" $@
#@dd if=$(BUILD_DIR)/stage1.bin of=$@ conv=notrunc
#@mcopy -i $@ $(BUILD_DIR)/stage2.bin "::stage2.bin"
#@mcopy -i $@ $(BUILD_DIR)/kernel.bin "::kernel.bin"
#@mmd -i $@ "::data"
#@mcopy -i $@ text.txt "::data/text.txt"
#@echo "Finished Writing Image: " $@

bootloader: stage1 stage2

stage1: $(BUILD_DIR)/stage1.bin

$(BUILD_DIR)/stage1.bin: always
	@$(MAKE) -C $(SRC_DIR)/bootloader/Stage-1 BUILD_DIR=$(abspath $(BUILD_DIR))

stage2: $(BUILD_DIR)/stage2.bin

$(BUILD_DIR)/stage2.bin: always
	@$(MAKE) -C $(SRC_DIR)/bootloader/Stage-2 BUILD_DIR=$(abspath $(BUILD_DIR))

kernel: $(BUILD_DIR)/kernel.elf

$(BUILD_DIR)/kernel.elf: always
	@$(MAKE) -C $(SRC_DIR)/kernel BUILD_DIR=$(abspath $(BUILD_DIR))

always:
	@mkdir -p $(BUILD_DIR)

clean:
	@$(MAKE) -C $(SRC_DIR)/bootloader/Stage-1 BUILD_DIR=$(abspath $(BUILD_DIR)) clean
	@$(MAKE) -C $(SRC_DIR)/bootloader/Stage-2 BUILD_DIR=$(abspath $(BUILD_DIR)) clean
	@$(MAKE) -C $(SRC_DIR)/kernel BUILD_DIR=$(abspath $(BUILD_DIR)) clean
	@rm -rf $(BUILD_DIR)
	@echo "Removed: " $(BUILD_DIR)

hexdump:
	@hd $(BUILD_DIR)/image.img > $(BUILD_DIR)/image.info
	@echo "Finished hexdump to: image.info"

iso_dir:
	@mkdir -p $(CD_ISO)

iso: iso_dir realOS.iso
	@cp $(BUILD_DIR)/image.img $(CD_ISO)
	@mkisofs -o realOS.iso -V RealOS -b image.img $(CD_ISO)
