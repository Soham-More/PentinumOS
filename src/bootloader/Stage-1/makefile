# Stage 1 makefile

BUILD_DIR ?= build
ASM ?= nasm

.PHONY: all clean

all: stage1

stage1: MBR VBR

MBR: $(BUILD_DIR)/MBR.bin

VBR: $(BUILD_DIR)/VBR.bin

$(BUILD_DIR)/MBR.bin: stage1.asm
	@echo "Compiling MBR.asm"
	@$(ASM) $< -f bin -o $@

$(BUILD_DIR)/VBR.bin: VBR.asm
	@echo "Compiling VBR.asm"
	@$(ASM) $< -f bin -o $@

clean:
	@rm -f $(BUILD_DIR)/stage1.bin
	@rm -f $(BUILD_DIR)/VBR.bin