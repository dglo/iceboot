#
# makefile for iceboot, not useful without being called
# by a workspace makefile, see iceboot-ws/Makefile for
# details...
#
#.c.o:
#	$(CC) -c $(CFLAGS) $<

all: $(ICEBOOTBIN)

clean:
	rm -f *.o *.i *.bin *.elf *.hex *.gz nfis.c

SFIOBJS = $(CRT0) $(LIBK) sfi.o memtests.o flashdrv.o osdep.o nfis.o \
	install.o md5.o

sfi.o: sfi.c $(EPXAH)
memtests.o: memtests.c $(EPXAH)
flashdrv.o: flashdrv.c flash.h $(EPXAH)
osdep.o: osdep.c osdep.h $(EPXAH)
nfis.c: ../../epxa10/iceboot/fis.c
	cp -v ../../epxa10/iceboot/fis.c nfis.c
nfis.o: nfis.c fis.h $(EPXAH)

$(ICEBOOTBIN): $(SFIOBJS) $(LIBHAL)
	$(CC) -g -o $(ICEBOOTBIN) $(SFIOBJS) $(LIBHAL) $(SYSLIBS)


