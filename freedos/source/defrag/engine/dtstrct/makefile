#
# Makefile for the data structures
#

!include "..\..\config.mak"

all: dtstrct.lib
        @copy dtstrct.lib ..\lib
        @echo Data structures compiled

dtstrct.lib: bitfield.obj vfsbtfld.obj vfsarray.obj
       @-del dtstrct.lib
       @$(libman) dtstrct.lib + bitfield.obj
       @$(libman) dtstrct.lib + vfsbtfld.obj
       @$(libman) dtstrct.lib + vfsarray.obj
       @-del dtstrct.bak

bitfield.obj: bitfield.c
       @$(compiler) $(options) -I$(includepath) -I..\header;..\..\misc bitfield.c

vfsbtfld.obj: vfsbtfld.c
       @$(compiler) $(options) -I$(includepath) -I..\header;..\..\misc vfsbtfld.c

vfsarray.obj: vfsarray.c
       @$(compiler) $(options) -I$(includepath) -I..\header;..\..\misc vfsarray.c


clean:
        @-del bitfield.obj
        @-del vfsbtfld.obj
        @-del vfsarray.obj
        @-del dtstrct.lib
        @-del dtstrct.bak
