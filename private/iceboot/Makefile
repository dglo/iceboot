#
# makefile for iceboot, not useful without being called
# by a workspace makefile, see iceboot-ws/Makefile for
# details...
#
.c.o:
	$(CC) -c $(CFLAGS) $<

all: $(ICEBOOT)

clean:
	rm -f *.o *.i *.bin *.elf *.hex *.gz

SFIOBJS = $(CRT0) $(LIBK) sfi.o memtests.o flashdrv.o osdep.o fis.o

sfi.o: sfi.c $(EPXAH)
memtests.o: memtests.c $(EPXAH)
flashdrv.o: flashdrv.c flash.h $(EPXAH)
osdep.o: osdep.c osdep.h $(EPXAH)
fis.o: fis.c fis.h $(EPXAH)

$(ICEBOOTBIN): $(SFIOBJS)
	$(CC) -o $(ICEBOOTBIN) $(SFIOBJS) $(SYSLIBS)
