#
#  Makefile for files in this directory.
#

!include "..\..\config.mak"

libman = tlib

all: keyboard.lib
        @echo Keyboard routines compiled.

keyboard.lib: keyboard.obj altkey.obj
        @-del keyboard.lib
        @$(libman) keyboard.lib + keyboard.obj
        @$(libman) keyboard.lib + altkey.obj
        @-del keyboard.bak 

keyboard.obj: keyboard.asm
        @nasm keyboard.asm -f obj -o keyboard.obj         

altkey.obj: altkey.c keyboard.h
        @$(compiler) $(options) -c altkey.c 

clean:
        @-del keyboard.obj 
        @-del altkey.obj   
        @-del keyboard.lib