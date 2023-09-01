# Stage 2 makefile

TARGET_ASMFLAGS += -f elf
TARGET_CFLAGS += -ffreestanding -nostdlib
TARGET_LIBS += -lgcc
TARGET_LINKFLAGS += -T linker.ld -nostdlib

SRC_C = $(wildcard *.c)
SRC_ASM = $(wildcard *.asm)
OBJ_C = $(patsubst %.c, $(BUILD_DIR)/stage2/c/%.obj, $(SRC_C))
OBJ_ASM = $(patsubst %.asm, $(BUILD_DIR)/stage2/asm/%.obj, $(SRC_ASM))

.PHONY: all stage2 clean always

all: stage2

stage2: $(BUILD_DIR)/stage2.bin

$(BUILD_DIR)/stage2.bin: $(OBJ_ASM) $(OBJ_C)
	@echo "Linking Stage2"
	@$(TARGET_LD) $(TARGET_LINKFLAGS) -Wl,-Map=$(BUILD_DIR)/stage2.map -o $@ $^ $(TARGET_LIBS)

$(BUILD_DIR)/stage2/c/%.obj: %.c
	@mkdir -p $(@D)
	@echo "Compiling " $<
	@$(TARGET_CC) $(TARGET_CFLAGS) $(OPT_CFLAGS) -c -o $@ $<

$(BUILD_DIR)/stage2/asm/%.obj: %.asm
	@mkdir -p $(@D)
	@echo "Compiling " $<
	@$(TARGET_ASM) $(TARGET_ASMFLAGS) -o $@ $<

clean:
	@rm -f $(BUILD_DIR)/stage2.bin