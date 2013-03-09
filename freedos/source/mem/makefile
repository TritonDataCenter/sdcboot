#
# Makefile for FreeDOS MEM
#
#
# You might need to change the following lines if you use another
# compiler than Turbo / Watcom C
#
# for Turbo C:
# assuming that a) tcc , tlink and nasm are all your path and
# b) these are configured the right way, for instance using a turboc.cfg
# file in the same directory with e.g. -IC:\TC\INCLUDE -LC:\TC\LIB.
#
# for Watcom: make sure it's setup correctly

!include "mkfiles\generic.mak"

#FIXME: don't commit this file with the modified flags to OpenWatcom
#       to enable debugging symbols

default: mem.exe

!include "mkfiles\nls.mak"

# MEMSUPT is set based on which compiler is selected in mkfiles\generic.mak
MEM_OBJS=prf.obj kitten.obj $(MEMSUPT)

mem.exe: mem.obj $(MEM_OBJS)
    $(CC) $(LFLAGS) mem.obj $(MEM_OBJS)
    $(UPX) mem.exe

memdbg.exe: memdbg.obj $(MEM_OBJS)
    $(CC) $(LFLAGS) memdbg.obj $(MEM_OBJS)

memdbg.obj: mem.c
    $(CC) $(CFLAGS) -DDEBUG $(OBJOUT)memdbg.obj mem.c

memtest.exe: memtest.obj
    $(CC) $(LFLAGS) $<

.c.obj:
    $(CC) $(CFLAGS) $<

.asm.obj:
    nasm -fobj $*.asm

mem.zip: dist
    makezip.bat

clean:
    del *.obj
    del *.map

distclean: clean
    del mem.exe
    del memtest.exe

dist: mem.exe ..\..\bin $(SYMBOLIC)
    copy mem.exe ..\..\bin
    del mem.exe

..\..\bin:
    mkdir ..\..\bin
