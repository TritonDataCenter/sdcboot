#.SUFFIXES: .obj .c .asm

# To build the ring 0 version (no paging): make -DRING=0
!if !$d(RING)
RING = 3
IMAGE = cwsdpmi.exe
STUBI = cwsdstub.exe
!else
IMAGE = cwsdpr$(RING).exe
STUBI = cwsdstr$(RING).exe
!endif

AFLAGS = /mx /t /drun_ring=$(RING) /dI31PROT
CFLAGS = -ms -G- -O -Z -a -3 -f- -d -w5 -g12 -Drun_ring=$(RING) -DI31PROT

CC=bcc
ASM=tasm

.c.obj:
	$(CC) $(CFLAGS) -c $*.c

.c.s:
	$(CC) $(CFLAGS) -S $*.c

.asm.obj:
	$(ASM) $(AFLAGS) $*;

OBJ1 =\
	dalloc.obj\
	doutils.obj\
	dpmisim.obj\
	exphdlr.obj\
	mswitch.obj\
	paging.obj\
	tables.obj\
	uextmem.obj\
	utils.obj

OBJ2 =\
	valloc.obj\
	xms.obj\
	vcpi.obj

all : $(IMAGE) $(STUBI)

constub.obj : control.c control.obj
	$(CC) $(CFLAGS) -DSTUB -c -oconstub.obj control.c

$(IMAGE) : $(RING).rng start.obj control.obj $(OBJ1) unload.obj $(OBJ2) ehdrfix.exe cwsparam.exe
	echo start.obj control.obj+ > cws.lnk
	echo $(OBJ1)+ >> cws.lnk
	echo unload.obj $(OBJ2),$*.exe,$*.map >> cws.lnk
	tlink /3 /s /c /m @cws.lnk
	del cws.lnk
	./ehdrfix $*.exe
	./cwsparam -v $*.exe

$(STUBI) : cwsdstub.obj start.obj constub.obj $(OBJ1) $(OBJ2) ehdrfix.exe cwsparam.exe
	echo cwsdstub.obj start.obj constub.obj+ > cws.lnk
	echo $(OBJ1)+ >> cws.lnk
	echo $(OBJ2),$*.exe,$*.map >> cws.lnk
	tlink /3 /s /c /m @cws.lnk
	del cws.lnk
	./ehdrfix $*.exe
	./cwsparam -v $*.exe

ehdrfix.exe : ehdrfix.c
	$(CC) $(CFLAGS) -e$*.exe ehdrfix.c
	del ehdrfix.obj

cwsparam.exe : cwsparm.c control.h gotypes.h
	$(CC) $(CFLAGS) -e$*.exe cwsparm.c
	del cwsparm.obj

$(RING).rng : makefile
	del *.rng
	del *.obj
	echo $(RING) > $(RING).rng

clean :
	del *.obj
	del *.map
	del *.exe
	del *.rng

# DEPENDENCIES

control.obj : gotypes.h gdt.h idt.h tss.h valloc.h utils.h vcpi.h paging.h exphdlr.h dalloc.h mswitch.h xms.h control.h

dalloc.obj : gotypes.h dalloc.h control.h

doutils.obj : segdefs.inc gdt.inc

dpmisim.obj : segdefs.inc tss.inc gdt.inc

exphdlr.obj : gotypes.h gdt.h idt.h tss.h utils.h paging.h vcpi.h dpmisim.h dalloc.h valloc.h control.h mswitch.h exphdlr.h

mswitch.obj : segdefs.inc tss.inc gdt.inc vcpi.inc

paging.obj : gotypes.h paging.h tss.h idt.h gdt.h valloc.h dalloc.h utils.h vcpi.h exphdlr.h control.h mswitch.h

tables.obj : segdefs.inc tss.inc gdt.inc

utils.obj : gotypes.h tss.h gdt.h utils.h mswitch.h exphdlr.h control.h

unload.obj : segdefs.inc

valloc.obj : gotypes.h valloc.h xms.h vcpi.h paging.h control.h mswitch.h

xms.obj : segdefs.inc

vcpi.obj : segdefs.inc vcpi.inc
