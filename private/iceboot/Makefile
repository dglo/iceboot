#
# makefile for iceboot, not useful without being called
# by a workspace makefile, see iceboot-ws/Makefile for
# details...
#
## .c.o:
#%.o : %.c *.h
#	$(CC) -c $(CFLAGS) $<

all: $(ICEBOOTBIN)

clean:
	rm -f *.o *.i *.bin *.elf *.hex *.gz nfis.c

SFIOBJS = $(CRT0) $(LIBK) sfi.o memtests.o flashdrv.o osdep.o fis.o \
	install.o md5.o radio_daq_hal.o

sfi.o: sfi.c $(EPXAH)
memtests.o: memtests.c $(EPXAH)
flashdrv.o: flashdrv.c flash.h $(EPXAH)
osdep.o: osdep.c osdep.h $(EPXAH)
fis.o: fis.c fis.h $(EPXAH)

$(ICEBOOTBIN): $(SFIOBJS) $(LIBHAL)
	$(CC) -c $(CFLAGS) versions.c
	$(CC) -m32 -g -o $(ICEBOOTBIN) versions.o $(SFIOBJS) $(LIBHAL) $(SYSLIBS) 


