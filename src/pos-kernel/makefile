# Stage 2 makefile

KERNEL_SRC_DIR = $(WORKSPACE_DIR)/src/pos-kernel/
KERNEL_BUILD_DIR = $(BUILD_DIR)/pos-kernel

TARGET_ASMFLAGS += -f elf
TARGET_CFLAGS += -ffreestanding -finstrument-functions
TARGET_LIBS += -lgcc
TARGET_LINKFLAGS += -T linker.ld -nostdlib
TARGET_CPPFLAGS += -std=c++17 -ffreestanding -finstrument-functions -D _novscode -I$(KERNEL_SRC_DIR) -g #-nostdlib

TARGET_LD = $(TARGET)-g++

ASM_EXCLUDE := $(KERNEL_BUILD_DIR)/asm/cpp/crti.o $(KERNEL_BUILD_DIR)/asm/cpp/crtn.o

# Make does not offer a recursive wildcard function, so here's one:
# DO NOT ADD SPACES TO ARGS WHILE CALLING
# leave 2nd arg empty to search from this directory
rwildcard=$(wildcard $1$2) $(foreach d,$(wildcard $1*),$(call rwildcard,$d/,$2))

SRC_C := $(call rwildcard,,*.c)
SRC_ASM := $(call rwildcard,,*.asm)
SRC_CPP :=  $(call rwildcard,,*.cpp)
OBJ_C := $(patsubst %.c, $(KERNEL_BUILD_DIR)/c/%.o, $(SRC_C))
OBJ_ASM := $(filter-out $(ASM_EXCLUDE), $(patsubst %.asm, $(KERNEL_BUILD_DIR)/asm/%.o, $(SRC_ASM)))
OBJ_CPP := $(patsubst %.cpp, $(KERNEL_BUILD_DIR)/cpp/%.o, $(SRC_CPP))

.PHONY: all kernel clean always

all: kernel

kernel: $(BUILD_DIR)/kernel.elf

$(BUILD_DIR)/kernel.elf: $(KERNEL_BUILD_DIR)/asm/cpp/crti.o $(TOOLCHAIN_LIBS)/crtbegin.o $(OBJ_ASM) $(OBJ_C) $(OBJ_CPP) $(TOOLCHAIN_LIBS)/crtend.o $(KERNEL_BUILD_DIR)/asm/cpp/crtn.o
	@echo "Linking Kernel"
	@$(TARGET_LD) $(TARGET_LINKFLAGS) -Wl,-Map=$(BUILD_DIR)/kernel.map -o $@ $^ $(TARGET_LIBS)
	@echo "Created kernel.elf"

$(KERNEL_BUILD_DIR)/c/%.o: %.c
	@mkdir -p $(@D)
	@echo "Compiling: " $<
	@$(TARGET_CXX) $(TARGET_CPPFLAGS) -c -o $@ $<

$(KERNEL_BUILD_DIR)/asm/%.o: %.asm
	@mkdir -p $(@D)
	@echo "Compiling: " $<
	@$(TARGET_ASM) $(TARGET_ASMFLAGS) -o $@ $<

$(KERNEL_BUILD_DIR)/asm/cpp/crti.o: cpp/crti.asm
	@mkdir -p $(@D)
	@echo "Compiling: " $<
	@$(TARGET_ASM) $(TARGET_ASMFLAGS) -o $@ $<

$(KERNEL_BUILD_DIR)/asm/cpp/crtn.o: cpp/crtn.asm
	@mkdir -p $(@D)
	@echo "Compiling: " $<
	@$(TARGET_ASM) $(TARGET_ASMFLAGS) -o $@ $<

$(KERNEL_BUILD_DIR)/cpp/%.o: %.cpp
	@mkdir -p $(@D)
	@echo "Compiling: " $<
	@$(TARGET_CXX) $(TARGET_CPPFLAGS) -c -o $@ $<

clean:
	@rm -f $(BUILD_DIR)/kernel.elf
