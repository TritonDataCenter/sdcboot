#
# makefile for files in fte\disklib
#

!include "..\..\config.mak"

incdir = -I.

all: disklib.lib
	@copy disklib.lib ..\lib
	@echo Gregg Jennings disklib compiled.
        
disklib.lib: read.obj write.obj 
        @-del disklib.lib
	@-del disklib.bak
	@$(libman) disklib + read.obj
        @$(libman) disklib + write.obj
        @-del disklib.bak

read.obj: read.c
	@$(compiler) $(options) $(incdir) -oread.obj read.c

write.obj: write.c
	@$(compiler) $(options) $(incdir) -owrite.obj write.c

clean:
        @-del disklib.lib
	@-del read.obj
        @-del write.obj
        @-del *.bak
