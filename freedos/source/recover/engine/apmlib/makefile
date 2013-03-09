#
# makefile for files in fte\low
#

!include "..\..\config.mak"

incdir = -I..\header;..\..\misc;include

all: apmlib.lib
        @copy apmlib.lib ..\lib 
        @echo Advanced power management routines compiled.
   
apmlib.lib:  obj\apmlib.obj 
   @-del apmlib.lib
   @-del apmlib.bak
   @$(libman) apmlib + obj\apmlib.obj
   @-del apmlib.bak

obj\apmlib.obj: source\apmlib.c include\cext.h include\apmlib.h 
        @$(compiler) $(options) $(incdir) -oobj\apmlib.obj source\apmlib.c

clean:
   @-del obj\apmlib.obj
   @-del apmlib.lib
   @-del *.bak