# Stage 2 makefile

KERNEL_SRC_DIR = $(WORKSPACE_DIR)/src/c-kernel/
KERNEL_BUILD_DIR = $(BUILD_DIR)/c-kernel

TARGET_ASMFLAGS += -f elf
# TARGET_CFLAGS += -ffreestanding # -finstrument-functions
TARGET_LIBS += -lgcc
TARGET_LINKFLAGS += -T linker.ld -nostdlib
TARGET_CFLAGS += -Wall -Wno-unused-variable -Wno-parentheses -std=c17 -ffreestanding -D _novscode -I$(KERNEL_SRC_DIR) -g #-nostdlib -finstrument-functions

TARGET_LD = $(TARGET)-gcc

# ASM_EXCLUDE := $(KERNEL_BUILD_DIR)/asm/cpp/crti.o $(KERNEL_BUILD_DIR)/asm/cpp/crtn.o

# Make does not offer a recursive wildcard function, so here's one:
# DO NOT ADD SPACES TO ARGS WHILE CALLING
# leave 2nd arg empty to search from this directory
rwildcard=$(wildcard $1$2) $(foreach d,$(wildcard $1*),$(call rwildcard,$d/,$2))

SRC_C := $(call rwildcard,,*.c)
SRC_ASM := $(call rwildcard,,*.asm)
OBJ_C := $(patsubst %.c, $(KERNEL_BUILD_DIR)/c/%.o, $(SRC_C))
OBJ_ASM := $(filter-out $(ASM_EXCLUDE), $(patsubst %.asm, $(KERNEL_BUILD_DIR)/asm/%.o, $(SRC_ASM)))

.PHONY: all kernel clean always

all: kernel

kernel: $(BUILD_DIR)/kernel.elf

$(BUILD_DIR)/kernel.elf: $(OBJ_ASM) $(OBJ_C)
	@echo "Linking Kernel"
	@$(TARGET_LD) $(TARGET_LINKFLAGS) -Wl,-Map=$(BUILD_DIR)/kernel.map -o $@ $^ $(TARGET_LIBS)
	@echo "Created kernel.elf"

$(KERNEL_BUILD_DIR)/c/%.o: %.c
	@mkdir -p $(@D)
	@echo "Compiling: " $<
	@$(TARGET_CC) $(TARGET_CFLAGS) -c -o $@ $<

$(KERNEL_BUILD_DIR)/asm/%.o: %.asm
	@mkdir -p $(@D)
	@echo "Compiling: " $<
	@$(TARGET_ASM) $(TARGET_ASMFLAGS) -o $@ $<

clean:
	@rm -f $(BUILD_DIR)/kernel.elf
