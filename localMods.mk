-include ../dom-loader/$(PLATFORM_PUB_ROOT)/loader/loaderMods.mk

ifeq ("epxa10","$(strip $(PLATFORM))")
  OBJS += $(BUILD_DIR)/boot.o
  BOOT_PAYLOAD := $(BUILD_DIR)/sfi.bin
  EPXA10_BINS := $(BIN_DIR)/iceboot.hex $(BIN_DIR)/iceboot.bin.gz
  BIN_EXES += $(EPXA10_BINS)
  TO_BE_CLEANED += $(EPXA10_BINS)
  vpath %.S ../dom-loader/$(PLATFORM_PUB_ROOT)/booter
endif
