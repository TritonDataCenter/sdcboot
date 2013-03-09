#
# Makefile for files in this directory.
#

!include "..\..\config.mak"

libman = tlib

all: main.lib
        @echo Main routines compiled.

main.lib: cmdefint.obj chkargs.obj 
        @-del main.lib
        @$(libman) main.lib + cmdefint.obj
        @$(libman) main.lib + chkargs.obj
        @-del main.bak

cmdefint.obj: cmdefint.c ..\..\modlgate\modlgate.h ..\..\misc\version.h \
            ..\..\misc\misc.h chkargs.h
        @$(compiler) $(options) -I..\..\misc;..\..\engine\header cmdefint.c

chkargs.obj: chkargs.c ..\..\modlgate\defrpars.h ..\..\misc\bool.h \
             chkargs.h
        @$(compiler) $(options) chkargs.c 

clean:
        @-del chkargs.obj
        @-del cmdefint.obj
        @-del main.lib