$(BUILD_DIR)/sfi.elf : $(LIB_DIR)/crt0.o $(LIB_OBJS)
	$(LD) $(LD_FLAGS) --script=$(KERNEL_X) -o $(@) $(<) $(LOAD_LIBDIRS) $(LOAD_LIBS) $(SYS_LIBS)

$(BUILD_DIR)/sfi.bin : $(BUILD_DIR)/sfi.elf
	@test -d $(@D) || mkdir -p $(@D)
	$(OBJCOPY) -O binary $(<) $(@)

$(BUILD_DIR)/boot.o : boot.S $(BOOT_PAYLOAD)
	$(CPP) $(CPP_FLAGS) -DBINFILE=\"$(BOOT_PAYLOAD)\" -o $(BUILD_DIR)/$(*F).i $(<)
	$(AS) $(A_FLAGS) $(INC_FLAGS) -o $(@) $(BUILD_DIR)/$(*F).i

$(BUILD_DIR)/%.elf : $(BUILD_DIR)/%.o
	$(LD) --script=$(BOOT_X) -o $(@) $(<)

$(BUILD_DIR)/ice%.bin : $(BUILD_DIR)/%.elf
	@test -d $(@D) || mkdir -p $(@D)
	$(OBJCOPY) -O binary $(<) $(@)

$(BIN_DIR)/ice%.hex : $(BUILD_DIR)/%.elf
	@test -d $(@D) || mkdir -p $(@D)
	$(OBJCOPY) -O ihex $(<) $(@)

$(BIN_DIR)/%.bin.gz: $(BUILD_DIR)/%.bin
	gzip -c $(<) > $(@)

$(BIN_DIR)/iceboot : $(BIN_DIR)/sfi
	@test -L $(@D) || $(LN) $(<F) $(@) 

$(BIN_DIR)/iceboot-nfis: $(NFIS_OBJS) $(LIBRARY_EXISTS)
	@test -d $(@D) || mkdir -p $(@D)
	$(CC)  $(NFIS_OBJS) $(C_FLAGS) $(LD_FLAGS) $(LOAD_LIBDIRS) $(filter-out -liceboot,$(LOAD_LIBS)) -o $@
