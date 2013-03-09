#
# Makefile for information module (fat 12).
#

!include "..\..\config.mak"

incdir = ..\..\engine\header;..\..\modlgate;..\..\misc

all: ..\modbins\infofat.lib
        @echo Information module created.

..\modbins\infofat.lib: infofat.obj flldrvmp.obj defrfact.obj mrkunmve.obj ifatsize.obj \
                        refilcnt.obj
        @-del ..\modbins\infofat.lib
        @-del ..\modbins\infofat.bak
        @$(libman) ..\modbins\infofat.lib + infofat.obj
        @$(libman) ..\modbins\infofat.lib + flldrvmp.obj
        @$(libman) ..\modbins\infofat.lib + defrfact.obj
        @$(libman) ..\modbins\infofat.lib + mrkunmve.obj
        @$(libman) ..\modbins\infofat.lib + ifatsize.obj
        @$(libman) ..\modbins\infofat.lib + refilcnt.obj
        @-del ..\modbins\infofat.bak

infofat.obj: infofat.c ..\..\engine\header\fte.h
        @$(compiler) $(options) -I$(incdir) infofat.c

flldrvmp.obj: flldrvmp.c ..\..\modlgate\expected.h ..\..\engine\header\fte.h
        @$(compiler) $(options) -I$(incdir) flldrvmp.c

defrfact.obj: defrfact.c ..\..\engine\header\fte.h ..\..\misc\bool.h \
              infofat.h
        @$(compiler) $(options) -I$(incdir) defrfact.c

mrkunmve.obj: mrkunmve.c ..\..\engine\header\fte.h ..\..\misc\bool.h \
              infofat.h
        @$(compiler) $(options) -I$(incdir) mrkunmve.c

ifatsize.obj: ifatsize.c
        @$(compiler) $(options) -I$(incdir) ifatsize.c

refilcnt.obj: refilcnt.c
        @$(compiler) $(options) -I$(incdir) refilcnt.c

clean:
      @-del infofat.obj
      @-del flldrvmp.obj
      @-del defrfact.obj
      @-del mrkunmve.obj
      @-del walktree.obj
      @-del ifatsize.obj
      @-del refilcnt.obj
      @-del ..\modbins\infofat.lib
