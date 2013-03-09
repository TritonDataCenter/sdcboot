#
# Makefile for files in this directory.
#

!include "..\..\config.mak"

all: ovlimpl.lib
     @echo host implementation compiled.

ovlimpl.lib: ovlimpl.obj
     @-del ovlimpl.lib
     @$(libman) ovlimpl.lib + ovlimpl.obj
     @-del ovlimpl.bak

ovlimpl.obj: ovlimpl.c ..\..\modlgate\expected.h \
             ..\main\chkargs.h
     @$(compiler) $(options) -I..\..\misc;..\..\engine\header ovlimpl.c

clean: 
     @-del ovlimpl.obj
     @-del ovlimpl.lib