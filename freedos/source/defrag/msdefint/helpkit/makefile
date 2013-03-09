#
#  Makefile for files in this directory.
#

!include "..\..\config.mak"

all: mkhelp.exe 
   @echo Helpkit build.

mkhelp.exe:  mkhelp.obj checksum.obj helpfile.obj 
   @$(linker) mkhelp.obj checksum.obj helpfile.obj 

mkhelp.obj: mkhelp.c mkhelp.h
   @$(compiler) $(options) mkhelp.c

checksum.obj: checksum.c checksum.h
   @$(compiler) $(options) -DCALCULATION_PROG checksum.c

helpfile.obj: helpfile.c mkhelp.h checksum.h
   @$(compiler) $(options) helpfile.c

clean:
   @-del mkhelp.obj
   @-del checksum.obj
   @-del helpfile.obj
   @-del mkhelp.exe
