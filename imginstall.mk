
# execute only once
DEVICE := $(shell losetup --partscan --find --show $(IMAGE_FILE))

install:
	@mount $(DEVICE)p1 $(IMG_MOUNT)

	@cp -r $(ROOT_FILES)/* $(IMG_MOUNT)
	@cp $(BUILD_DIR)/kernel.elf $(IMG_MOUNT)

	@umount $(IMG_MOUNT)
	@losetup -d $(DEVICE)
