-include ../dom-loader/$(PLATFORM_PUB_ROOT)/loader/loaderMods.mk

ifeq ("epxa10","$(strip $(PLATFORM))")
  OBJS += $(BUILD_DIR)/boot.o
  BOOT_PAYLOAD := $(BUILD_DIR)/sfi.bin
  EPXA10_BINS := $(BIN_DIR)/iceboot.hex $(BIN_DIR)/iceboot.bin.gz
  BIN_EXES += $(EPXA10_BINS)
  TO_BE_CLEANED += $(EPXA10_BINS)
  vpath %.S ../dom-loader/$(PLATFORM_PUB_ROOT)/booter
else
  BIN_EXES += $(BIN_DIR)/iceboot $(BIN_DIR)/iceboot-nfis
  NFIS_OBJS := $(BUILD_DIR)/sfi.o $(BUILD_DIR)/memtests.o $(BUILD_DIR)/flashdrv.o $(BUILD_DIR)/osdep.o $(BUILD_DIR)/nfis.o $(BUILD_DIR)/install.o
endif

LIB_OBJS := $(filter-out $(LIBRARY)(nfis.o), $(LIB_OBJS))
LOAD_LIBS := $(filter-out -ldom-fpga, $(LOAD_LIBS))

