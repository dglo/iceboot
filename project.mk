C_BIN_NAMES := sfi
C_EXCLUDE_NAMES := iceboot parse crc load flash diag io main misc_funs xyzModem
USES_PROJECTS := hal dom-loader dom-fpga
USES_TOOLS := z

ifneq ("epxa10","$(strip $(PLATFORM))")
  USES_PROJECTS := $(filter-out dom-loader, $(USES_PROJECTS))
else
  C_BIN_NAMES := $(filter-out sfi, $(C_BIN_NAMES))
endif

include ../tools/resources/standard.mk
