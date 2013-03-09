#
# makefile for miscellanous functions
#

libman = tlib

all: misc.lib
        @echo miscellanous functions compiled
                
misc.lib: switchch.obj
        @-del misc.lib
        @$(libman) misc.lib + switchch.obj
        @-del misc.bak

switchch.obj: switchch.asm
        @nasm switchch.asm -f obj -o switchch.obj

clean:
        @-del misc.lib
        @-del switchch.obj
     
