#
#  Makefile for files in this directory.
#

!include "..\config.mak"

all: misc.lib
     @echo Miscelanous routines compiled.

misc.lib: switchch.obj critical.obj hicritcl.obj gtdrvnms.obj \
     gtcdex.obj drvtypes.obj reboot.obj
     @-del misc.lib
     @-del misc.bak
     @$(libman) misc.lib + switchch.obj
     @$(libman) misc.lib + critical.obj
     @$(libman) misc.lib + hicritcl.obj
     @$(libman) misc.lib + gtdrvnms.obj
     @$(libman) misc.lib + gtcdex.obj
     @$(libman) misc.lib + drvtypes.obj
     @$(libman) misc.lib + reboot.obj
     @-del misc.bak
        
switchch.obj: switchch.asm
     @nasm switchch.asm -f obj -o switchch.obj

critical.obj: critical.asm
     @nasm critical.asm -f obj -o critical.obj

hicritcl.obj: hicritcl.asm
     @nasm hicritcl.asm -f obj -o hicritcl.obj

gtdrvnms.obj: gtdrvnms.c
     @$(compiler) $(options) gtdrvnms.c

gtcdex.obj: gtcdex.asm
     @nasm gtcdex.asm -f obj -o gtcdex.obj

reboot.obj: reboot.c 
     @$(compiler) $(options) -I..\engine\apmlib\include reboot.c 

drvtypes.obj: drvtypes.asm
     @nasm drvtypes.asm -f obj -o drvtypes.obj

clean:
     @-del switchch.obj
     @-del critical.obj
     @-del hicritcl.obj
     @-del gtdrvnms.obj
     @-del gtcdex.obj
     @-del reboot.obj
     @-del drvtypes.obj
     @-del misc.lib
     @-del *.bak