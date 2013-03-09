#
# Makefile for files in this directory.
#

!include "..\..\config.mak"

libman = tlib

all: mouse.lib
        @echo Mouse routines compiled.

mouse.lib: mouse.obj himouse.obj
        @-del mouse.lib
        @$(libman) mouse.lib + mouse.obj
        @$(libman) mouse.lib + himouse.obj
        @-del mouse.bak

mouse.obj: mouse.asm 
        @nasm mouse.asm -f obj -o mouse.obj

himouse.obj: himouse.c mouse.h ..\..\misc\bool.h
        @$(compiler) $(options) himouse.c

clean:
        @-del mouse.obj 
        @-del himouse.obj
        @-del mouse.lib
        @-del *.bak