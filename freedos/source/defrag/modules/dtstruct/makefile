#
# Makefile for defragmentation module.
#

!include "..\..\config.mak"

incdir = ..\..\engine\header;..\..\modlgate;..\..\misc

all: ..\modbins\dtstruct.lib
        @echo Defragmentation module created.

..\modbins\dtstruct.lib: clmovmap.obj vlhandle.obj
        @-del ..\modbins\dtstruct.lib
        @-del ..\modbins\dtstruct.bak
        @$(libman) ..\modbins\dtstruct.lib + clmovmap.obj
        @$(libman) ..\modbins\dtstruct.lib + vlhandle.obj
        @-del ..\modbins\dtstruct.bak

clmovmap.obj: clmovmap.c ..\..\engine\header\fte.h 
        @$(compiler) $(options) -I$(incdir) clmovmap.c

vlhandle.obj: vlhandle.c ..\..\engine\header\fte.h 
        @$(compiler) $(options) -I$(incdir) $(image) vlhandle.c
        
clean:
      @-del clmovmap.obj
      @-del vlhandle.obj
      @-del ..\modbins\dtstruct.lib