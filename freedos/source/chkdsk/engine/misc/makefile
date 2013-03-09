#
#  Makefile for files in this directory.
#

!include "..\..\config.mak"

incdir = -I..\header;..\..\misc

all: misc.lib
     @copy misc.lib ..\lib   
     @echo FTE miscelanous routines compiled.

misc.lib: bufshift.obj entshift.obj wildcard.obj pathconv.obj mkabspth.obj
     @-del misc.lib
     @-del misc.bak
     @$(libman) misc.lib + bufshift.obj
     @$(libman) misc.lib + entshift.obj
     @$(libman) misc.lib + wildcard.obj
     @$(libman) misc.lib + pathconv.obj
     @$(libman) misc.lib + mkabspth.obj
     @-del misc.bak

bufshift.obj: bufshift.c 
     @$(compiler) $(options) $(incdir) bufshift.c
     
entshift.obj: entshift.c
     @$(compiler) $(options) $(incdir) entshift.c

wildcard.obj: wildcard.c
     @$(compiler) $(options) $(incdir) wildcard.c

pathconv.obj: pathconv.c
     @$(compiler) $(options) $(incdir) pathconv.c

mkabspth.obj: mkabspth.c
     @$(compiler) $(options) $(incdir) mkabspth.c
     
clean:
     @-del bufshift.obj
     @-del entshift.obj
     @-del wildcard.obj
     @-del pathconv.obj
     @-del mkabspth.obj
     @-del misc.lib
     @-del *.bak