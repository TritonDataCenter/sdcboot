#
#  Makefile for files in this directory.
#

!include "..\..\config.mak"

all: ovlhost.lib
        @echo Host implementation routines compiled.

ovlhost.lib: ovlimpl.obj lowtime.obj
        @-del ovlhost.lib
        @$(libman) ovlhost.lib + ovlimpl.obj
        @$(libman) ovlhost.lib + lowtime.obj
        @-del ovlhost.bak
        
ovlimpl.obj: ovlimpl.c ..\screen\screen.h ..\dialog\dialog.h \
             ..\dialog\msgbxs.h ..\..\modlgate\expected.h    \
             lowtime.h
        @$(compiler) $(options) -I..\..\misc;..\..\engine\header ovlimpl.c

lowtime.obj: lowtime.asm
        @nasm lowtime.asm -f obj -o lowtime.obj

clean:
        @-del ovlimpl.obj 
        @-del lowtime.obj 
        @-del ovlhost.lib
        @-del ovlhost.bak