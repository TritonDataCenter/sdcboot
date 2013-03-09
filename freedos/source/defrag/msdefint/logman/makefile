#
#  Makefile for files in this directory.
#

!include "..\..\config.mak"

libman = tlib

all: logman.lib
        @echo Log manager routines compiled.

logman.lib: scrlog.obj lologman.obj logprint.obj
        @-del logman.lib
        @$(libman) logman.lib + scrlog.obj
        @$(libman) logman.lib + lologman.obj
        @$(libman) logman.lib + logprint.obj
        @-del logman.bak

scrlog.obj: scrlog.c ..\screen\screen.h ..\event\event.h
        @$(compiler) $(options) scrlog.c

lologman.obj: lologman.asm
        @nasm lologman.asm -f obj -o lologman.obj

logprint.obj: logprint.c logman.h
        @$(compiler) $(options) logprint.c

clean:
        @-del scrlog.obj
        @-del lologman.obj
        @-del logprint.obj
        @-del logman.obj
        @-del logman.lib