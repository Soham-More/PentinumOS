# Stage 2 makefile

TARGET_ASMFLAGS += -f elf
TARGET_CFLAGS += -ffreestanding
TARGET_LIBS += -lgcc
TARGET_LINKFLAGS += -T linker.ld -nostdlib
TARGET_CPPFLAGS += -ffreestanding -D _novscode -I $(WORKSPACE_DIR)/src/kernel/ -g #-nostdlib

TARGET_LD = $(TARGET)-g++

ASM_EXCLUDE := $(BUILD_DIR)/kernel/asm/cpp/crti.o $(BUILD_DIR)/kernel/asm/cpp/crtn.o

# Make does not offer a recursive wildcard function, so here's one:
# DO NOT ADD SPACES TO ARGS WHILE CALLING
# leave 2nd arg empty to search from this directory
rwildcard=$(wildcard $1$2) $(foreach d,$(wildcard $1*),$(call rwildcard,$d/,$2))

SRC_C := $(call rwildcard,,*.c)
SRC_ASM := $(call rwildcard,,*.asm)
SRC_CPP :=  $(call rwildcard,,*.cpp)
OBJ_C := $(patsubst %.c, $(BUILD_DIR)/kernel/c/%.o, $(SRC_C))
OBJ_ASM := $(filter-out $(ASM_EXCLUDE), $(patsubst %.asm, $(BUILD_DIR)/kernel/asm/%.o, $(SRC_ASM)))
OBJ_CPP := $(patsubst %.cpp, $(BUILD_DIR)/kernel/cpp/%.o, $(SRC_CPP))

.PHONY: all kernel clean always

all: kernel

kernel: $(BUILD_DIR)/kernel.elf

$(BUILD_DIR)/kernel.elf: $(BUILD_DIR)/kernel/asm/cpp/crti.o $(TOOLCHAIN_LIBS)/crtbegin.o $(OBJ_ASM) $(OBJ_C) $(OBJ_CPP) $(TOOLCHAIN_LIBS)/crtend.o $(BUILD_DIR)/kernel/asm/cpp/crtn.o
	@echo "Linking Kernel"
	@$(TARGET_LD) $(TARGET_LINKFLAGS) -Wl,-Map=$(BUILD_DIR)/kernel.map -o $@ $^ $(TARGET_LIBS)
	@echo "Created kernel.elf"

$(BUILD_DIR)/kernel/c/%.o: %.c
	@mkdir -p $(@D)
	@echo "Compiling: " $<
	@$(TARGET_CXX) $(TARGET_CPPFLAGS) -c -o $@ $<

$(BUILD_DIR)/kernel/asm/%.o: %.asm
	@mkdir -p $(@D)
	@echo "Compiling: " $<
	@$(TARGET_ASM) $(TARGET_ASMFLAGS) -o $@ $<

$(BUILD_DIR)/kernel/asm/cpp/crti.o: cpp/crti.asm
	@mkdir -p $(@D)
	@echo "Compiling: " $<
	@$(TARGET_ASM) $(TARGET_ASMFLAGS) -o $@ $<

$(BUILD_DIR)/kernel/asm/cpp/crtn.o: cpp/crtn.asm
	@mkdir -p $(@D)
	@echo "Compiling: " $<
	@$(TARGET_ASM) $(TARGET_ASMFLAGS) -o $@ $<

$(BUILD_DIR)/kernel/cpp/%.o: %.cpp
	@mkdir -p $(@D)
	@echo "Compiling: " $<
	@$(TARGET_CXX) $(TARGET_CPPFLAGS) -c -o $@ $<

clean:
	@rm -f $(BUILD_DIR)/kernel.elf
