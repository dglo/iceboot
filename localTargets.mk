$(LIB_DIR)/sfi.elf : $(LIBRARY)
	$(LD) $(LD_FLAGS) -N --script=$(KERNELX) -o $(@) $(LIB_DIR)/crt0.o  $(LOAD_LIBDIRS) $(LOAD_LIBS) $(SYS_LIBS)

$(BIN_DIR)/sfi.bin : $(LIB_DIR)/sfi.elf
	@test -d $(@D) || mkdir -p $(@D)
	$(OBJCOPY) -O binary $< $(@)
