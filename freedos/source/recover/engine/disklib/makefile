#
# makefile for files in fte\disklib\dos, etc...
#

!include "..\..\config.mak"

incdir = -I.

all: disklib.lib
	@copy disklib.lib ..\lib
	@echo Gregg Jennings disklib compiled.
        
disklib.lib: ioctl.obj read.obj write.obj getdisk.obj xlate.obj
        @-del disklib.lib
	@-del disklib.bak
        @$(libman) disklib + ioctl.obj
	@$(libman) disklib + read.obj
        @$(libman) disklib + write.obj
        @$(libman) disklib + getdisk.obj
	@$(libman) disklib + xlate.obj
        @-del disklib.bak

ioctl.obj: ioctl.c
	@$(compiler) $(options) $(incdir) -oioctl.obj ioctl.c

read.obj: read.c
	@$(compiler) $(options) $(incdir) -oread.obj read.c

write.obj: write.c
	@$(compiler) $(options) $(incdir) -owrite.obj write.c

getdisk.obj: getdisk.c
	@$(compiler) $(options) $(incdir) -ogetdisk.obj getdisk.c

xlate.obj: xlate.c
	@$(compiler) $(options) $(incdir) -oxlate.obj xlate.c

clean:
        @-del disklib.lib
	@-del ioctl.obj
	@-del read.obj
        @-del write.obj
        @-del getdisk.obj
	@-del xlate.obj
        @-del *.bak