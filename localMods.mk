BIN_EXES += $(BIN_DIR)/sfi.bin

TO_BE_CLEANED += $(LIB_DIR)/sfi.elf $(BIN_DIR)/sfi.bin

OBJCOPY := $(ARM_HOME)/arm-elf/bin/arm-elf-objcopy
KERNELX := ../dom-loader/$(PLATFORM_PUB_ROOT)/loader/kernel.x
