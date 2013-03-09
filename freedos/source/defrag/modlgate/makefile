#
#  Makefile for files in this directory.
#

!include "..\config.mak"

all: modlgate.lib
        @echo Module gate routines compiled.

modlgate.lib: defrpars.obj scandrv.obj optidisk.obj sortdirs.obj chkdski.obj \
              callback.obj argvars.obj custerr.obj fatsize.obj
        @-del modlgate.lib
        @$(libman) modlgate.lib + defrpars.obj
        @$(libman) modlgate.lib + scandrv.obj
        @$(libman) modlgate.lib + optidisk.obj
        @$(libman) modlgate.lib + sortdirs.obj
        @$(libman) modlgate.lib + chkdski.obj
        @$(libman) modlgate.lib + callback.obj
        @$(libman) modlgate.lib + argvars.obj
        @$(libman) modlgate.lib + fatsize.obj                                        
        @$(libman) modlgate.lib + custerr.obj
        @-del modlgate.bak

callback.obj: callback.c expected.h
        @$(compiler) $(options) -I..\misc;..\engine\header callback.c

defrpars.obj: defrpars.c 
        @$(compiler) $(options) -I..\misc;..\engine\header defrpars.c

scandrv.obj: scandrv.c modlgate.h defrpars.h \
             ..\misc\bool.h ..\modules\modules.h
        @$(compiler) $(options) scandrv.c

optidisk.obj: optidisk.c modlgate.h defrpars.h
        @$(compiler) $(options) optidisk.c

sortdirs.obj: sortdirs.c modlgate.h defrpars.h
        @$(compiler) $(options) sortdirs.c

chkdski.obj: chkdski.c ..\misc\bool.h modlgate.h \
             defrpars.h ..\modules\modules.h
        @$(compiler) $(options) chkdski.c

argvars.obj: argvars.c argvars.h
        @$(compiler) $(options) argvars.c
   
custerr.obj: custerr.c custerr.h
   @$(compiler) $(options) -I..\misc;..\engine\header custerr.c

fatsize.obj: fatsize.c
   @$(compiler) $(options) fatsize.c

clean:
        @-del defrpars.obj
        @-del scandrv.obj
        @-del optidisk.obj
        @-del sortdirs.obj
        @-del chkdski.obj
        @-del callback.obj
        @-del argvars.obj
        @-del custerr.obj
        @-del fatsize.obj
        @-del modlgate.lib
        
        
