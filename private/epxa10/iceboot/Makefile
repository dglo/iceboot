#
# executables for the altera chips go through
# many stages of processing:
#
#  1) compile and link "kernel" images
#  2) link these kernel images with the executable
#       this guy is mapped at location 0
#     a) to flash this directly, convert this to intel
#        hex file format and use exec_flash_programmer...
#  3) create a binary image of this file
#  4) assemble a "raw" header to load this
#  5) link this new executable with the binary image
#  6) convert this new executable to motorola "S-rec" format or
#       binary (image) format
#

#
# files we export...
#
BINDIR=../../../../epxa10/bin
SFIBIN=$(BINDIR)/sfi.bin

#
# files we import...
#
KERNELX=../../../../dom-loader/public/epxa10/loader/kernel.x
RAWX=../../../../dom-loader/public/epxa10/loader/raw.x
RAWS=../../../../dom-loader/public/epxa10/loader/raw.S
EPXAHD = ../../../../epxa10/build/dom-loader/source
EPXAH = $(EPXAHD)/booter/epxa.h
CRT0=../../../../epxa10/lib/crt0.o

ARM_HOME = /usr/local
AFLAGS = -mcpu=arm920t  -I$(PTESD)
CPPFLAGS = -I$(EPXAHD)
CFLAGS = -mlittle-endian -mcpu=arm920 -Wall \
	-nostdinc -I$(ARM_HOME)/arm-elf/arm-elf/include \
	-I$(ARM_HOME)/arm-elf/lib/gcc-lib/arm-elf/3.2/include \
	-I../expat/xmlparse \
	-I../../../public \
	-I../../../../hal/public \
	-I../../../../stf/public \
	-I../../../../hal/private/epxa10 \
	-I/usr/local/arm-elf/include \
	$(CPPFLAGS)

CPP = arm-elf-cpp
CC = arm-elf-gcc
AS = arm-elf-as
LD = arm-elf-ld -N
OBJCOPY = arm-elf-objcopy

SYSLIBS = $(ARM_HOME)/arm-elf/arm-elf/lib/libc.a \
	$(ARM_HOME)/arm-elf/lib/gcc-lib/arm-elf/3.2/libgcc.a \
	$(ARM_HOME)/arm-elf/lib/libz.a

.c.o:
	$(CC) -c $(CFLAGS) $<

all: $(SFIBIN)

clean:
	rm -f *.o *.i *.bin *.elf *.hex *.gz

ICELIBS = ../../../../epxa10/lib/libhal.a ../redboot/libiceboot.a
KOBJS = ../../../../epxa10/lib/libkernel.a

memtests.o: memtests.c
	$(CC) -c $(CFLAGS) $<

SFIOBJS = $(CRT0) $(KOBJS) sfi.o memtests.o flashdrv.o osepxa.o

sfi.o: sfi.c $(EPXAH)
memtest.o: memtests.c $(EPXAH)
flashdrv.o: flashdrv.c flash.h $(EPXAH)
osepxa.o: osepxa.c osdep.h $(EPXAH)

sfi.elf: $(SFIOBJS) $(ICELIBS)
	$(LD) --script=$(KERNELX) -o sfi.elf $(SFIOBJS) $(ICELIBS) $(SYSLIBS)

$(SFIBIN): sfi.elf
	@test -d $(@D) || mkdir -p $(@D)
	$(OBJCOPY) -O binary $< $(SFIBIN)

#
# sim boot...
#
memtests.c: ../../../../stf/private/stf-apps/memtests.c
	cp -f ../../../../stf/private/stf-apps/memtests.c .

sfi: sfi.c memtests.c simflashdrv.c oslinux.c
	gcc -g -Wall -I../../../../stf/public \
		-I../../../../hal/public \
		-I../../../../hal/private/epxa10 \
		-I$(EPXAHD) -o sfi sfi.c memtests.c simflashdrv.c oslinux.c \
		../../../../Linux-i386/lib/libhal.a \
		../../../../Linux-i386/lib/libicecube.a \
		-lz

sfi.exe: sfi.c memtests.c simflash.c oslinux.c
	gcc -Wall -I../../../../stf/public \
		-I../../../../hal/public \
		-I../../../../hal/private/epxa10 \
		-I$(EPXAHD) -o sfi.exe sfi.c memtests.c simflash.c oslinux.c \
		../../../../cygwin-x86/lib/libhal.a \
		../../../../cygwin-x86/lib/libicecube.a \
		-lz




