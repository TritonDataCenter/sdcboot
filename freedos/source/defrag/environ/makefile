#
# Makefile for the files in this directory.
#

!include "..\config.mak"

all: environ.lib
        @echo Environment routines compiled.

environ.lib: os_id.obj dosemu.obj dpmitst.obj checkos.obj extmem.obj xmsinst.obj
        @-del environ.lib
        @$(libman) environ.lib + os_id.obj
        @$(libman) environ.lib + dosemu.obj
        @$(libman) environ.lib + dpmitst.obj
        @$(libman) environ.lib + checkos.obj
	@$(libman) environ.lib + extmem.obj
	@$(libman) environ.lib + xmsinst.obj
        @-del environ.bak

os_id.obj: os_id.c os_id.h dosemu.h
        @$(compiler) $(options) os_id.c

dosemu.obj: dosemu.c dosemu.h
        @$(compiler) $(options) dosemu.c

dpmitst.obj: dpmitst.asm 
        @nasm dpmitst.asm -f obj -o dpmitst.obj

checkos.obj: checkos.c 
        @$(compiler) $(options) checkos.c
	
extmem.obj: extmem.asm
	@nasm extmem.asm -f obj -o extmem.obj

xmsinst.obj: xmsinst.asm
	@nasm xmsinst.asm -f obj -o xmsinst.obj

clean: 
        @-del os_id.obj
        @-del dosemu.obj
        @-del dpmitst.obj
        @-del checkos.obj
	@-del extmem.obj
	@-del xmsinst.obj
        @-del environ.lib
        @-del *.bak
