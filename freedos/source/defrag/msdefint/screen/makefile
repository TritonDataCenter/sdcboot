#
#  Makefile for files in this directory.
#

!include "..\..\config.mak"

all: screen.lib
        @echo Screen routines compiled.
        
screen.lib: drawscr.obj lowvideo.obj scrclip.obj screen.obj scrmask.obj drvmap.obj
        @-del screen.lib
        @$(libman) screen.lib + drawscr.obj
        @$(libman) screen.lib + lowvideo.obj
        @$(libman) screen.lib + scrclip.obj
        @$(libman) screen.lib + screen.obj
        @$(libman) screen.lib + scrmask.obj
        @$(libman) screen.lib + drvmap.obj
        @-del screen.bak

drawscr.obj: drawscr.c screen.h
        @$(compiler) $(options) -I..\..\misc;..\..\engine\header drawscr.c

lowvideo.obj: lowvideo.c screen.h ..\mouse\mouse.h
        @$(compiler) $(options) lowvideo.c

scrclip.obj: scrclip.c ..\mouse\mouse.h
        @$(compiler) $(options) scrclip.c

screen.obj: screen.asm
        @nasm screen.asm -f obj -o screen.obj

scrmask.obj: scrmask.c screen.h
        @$(compiler) $(options) -I..\..\misc;..\..\engine\header scrmask.c

drvmap.obj: drvmap.c screen.h
        @$(compiler) $(options) -I..\..\misc;..\..\engine\header drvmap.c

clean:
        @-del drawscr.obj
        @-del lowvideo.obj
        @-del scrclip.obj
        @-del screen.obj
        @-del scrmask.obj
        @-del drvmap.obj
        @-del screen.lib