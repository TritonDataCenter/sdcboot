#
#  Makefile for files in this directory.
#

!include "..\..\config.mak"

libman   = tlib

all: hlpread.obj hlpsysid.obj idxstack.obj hlplnprs.obj hlpparse.obj \
     hlpfncs.obj checksum.obj
        @-del helpsys.lib
        @$(libman) helpsys.lib + hlpread.obj
        @$(libman) helpsys.lib + hlpsysid.obj
        @$(libman) helpsys.lib + idxstack.obj
        @$(libman) helpsys.lib + hlplnprs.obj
        @$(libman) helpsys.lib + hlpparse.obj
        @$(libman) helpsys.lib + hlpfncs.obj
        @$(libman) helpsys.lib + checksum.obj
        @-del helpsys.bak
        @echo Help system compiled.

hlpread.obj: hlpread.c hlpread.h hlpparse.h
        @$(compiler) $(options) hlpread.c

hlpsysid.obj: hlpsysid.c
        @$(compiler) $(options) hlpsysid.c

idxstack.obj: idxstack.c idxstack.h ..\logman\logman.h
        @$(compiler) $(options) idxstack.c

hlpparse.obj: hlpparse.c hlpread.h
   @$(compiler) $(options) hlpparse.c

hlpfncs.obj: hlpfncs.c ..\screen\screen.h hlpfncs.h hlpread.h hlpparse.h \
             hlplnprs.h hlpsysid.h ..\..\misc\bool.h ..\dialog\dialog.h \
	     hlplnprs.h
        @$(compiler) $(options) hlpfncs.c

hlplnprs.obj: hlplnprs.c ..\..\misc\bool.h
   @$(compiler) $(options) hlplnprs.c

checksum.obj: ..\helpkit\checksum.c
   @$(compiler) $(options) -DRECALCULATION_PROG -I..\helpkit ..\helpkit\checksum.c

clean:
        @-del hlpread.obj
        @-del hlpsysid.obj
        @-del idxstack.obj
        @-del hlplnprs.obj
        @-del hlpparse.obj
        @-del hlpfncs.obj
        @-del checksum.obj
        @-del helpsys.lib
        @-del helpsys.bak