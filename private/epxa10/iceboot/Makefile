#
# makefile for iceboot, not useful without being called
# by a workspace makefile, see dom-ws/Makefile for
# details...
#
.SUFFIXES: .c .o .elf .bin

.c.o:
	$(CC) -c $(CFLAGS) $<

all: $(SFIBIN)

clean:
	rm -f *.o *.i *.bin *.elf *.hex *.gz

.elf.bin:
	$(OBJCOPY) -O binary $*.elf $*.bin
	$(CPP) $(CPPFLAGS) -DBINFILE=\"$*.bin\" -o $*-raw.i $(RAWS)
	$(AS) $(AFLAGS) -o $*-raw.o $*-raw.i
	$(LD) --script=$(RAWX) -o $*-raw.elf $*-raw.o
	$(OBJCOPY) -O binary $*-raw.elf $*.bin

SFIOBJS = $(CRT0) $(LIBK) sfi.o memtests.o flashdrv.o osdep.o fis.o install.o \
	md5.o
YMOBJS = $(CRT0) $(LIBK) ymodem.o osdep.o
LBOBJS = $(CRT0) $(LIBK) lookback.o

sfi.o: sfi.c $(EPXAH)
memtests.o: memtests.c $(EPXAH)
flashdrv.o: flashdrv.c flash.h $(EPXAH)
osdep.o: osdep.c osdep.h $(EPXAH)
fis.o: fis.c fis.h $(EPXAH)
ymodem.o: ymodem.c $(EPXAH)
install.o: install.c $(EPXAH)

sfi.elf: $(SFIOBJS) $(LIBHAL) $(SYSLIBS) $(KERNELX)
	$(CC) -o versions.o -c $(CFLAGS) versions.c
	$(LD) --script=$(KERNELX) -o sfi.elf versions.o $(SFIOBJS) $(LIBHAL) $(SYSLIBS)

ymodem.elf: $(YMOBJS) $(LIBHAL) $(SYSLIBS) $(KERNELX)
	$(LD) --script=$(KERNELX) -o ymodem.elf \
		$(YMOBJS) $(LIBHAL) $(SYSLIBS)

lookback.elf: $(LBOBJS) $(LIBHAL) $(SYSLIBS) $(KERNELX)
	$(LD) --script=$(KERNELX) -o lookback.elf \
		$(LBOBJS) $(LIBHAL) $(SYSLIBS)

$(YMODEMBINGZ): ymodem.bin
	gzip -c ymodem.bin > $(YMODEMBINGZ)

$(SFIBIN): sfi.elf
	$(OBJCOPY) -O binary $< $(SFIBIN)

$(LOOKBACKBINGZ): lookback.bin
	gzip -c lookback.bin > $(LOOKBACKBINGZ)
