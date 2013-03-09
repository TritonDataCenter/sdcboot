#
#  Makefile for files in this directory.
#

!include "..\..\config.mak"

all: event.lib
        @echo Event routines compiled.

event.lib: event.obj
        @-del event.lib
        @$(libman) event.lib + event.obj
        @-del event.bak 

event.obj: event.c event.h ..\mouse\mouse.h ..\keyboard\keyboard.h
        @$(compiler) $(options) event.c

clean:
        @-del event.obj 
        @-del event.lib