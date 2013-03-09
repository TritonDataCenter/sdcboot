#
# Makefile for defragmentation module.
#

!include "..\..\config.mak"

incdir = ..\..\engine\header;..\..\modlgate;..\..\misc

all: ..\modbins\dfragfat.lib
        @echo Defragmentation module created.

..\modbins\dfragfat.lib: dfragfat.obj dfrgdrvr.obj  \
                         FllDfrag\FllDfrg.obj       \
                         clear\clrusct.obj          \
                         ordering\ordrdfrg.obj      \
                         crunch\crunch.obj          \
                         defrmap.obj                \
                         unfrgfls\unfrag.obj        \
                         unfrgfls\isflfgtd.obj      \
                         QuickTry\POWALKTR.obj      \
                         QuickTry\QuickTry.obj      \
                         QuickTry\RelocFil.obj                          
        @-del ..\modbins\dfragfat.lib
        @-del ..\modbins\dfragfat.bak
        @$(libman) ..\modbins\dfragfat.lib + dfragfat.obj
        @$(libman) ..\modbins\dfragfat.lib + dfrgdrvr.obj
        @$(libman) ..\modbins\dfragfat.lib + FllDfrag\FllDfrg.obj
        @$(libman) ..\modbins\dfragfat.lib + clear\ClrUSct.obj
        @$(libman) ..\modbins\dfragfat.lib + ordering\ordrdfrg.obj
        @$(libman) ..\modbins\dfragfat.lib + crunch\crunch.obj
        @$(libman) ..\modbins\dfragfat.lib + defrmap.obj
        @$(libman) ..\modbins\dfragfat.lib + unfrgfls\unfrag.obj
        @$(libman) ..\modbins\dfragfat.lib + unfrgfls\isflfgtd.obj
        @$(libman) ..\modbins\dfragfat.lib + QuickTry\POWALKTR.obj
        @$(libman) ..\modbins\dfragfat.lib + QuickTry\QuickTry.obj
        @$(libman) ..\modbins\dfragfat.lib + QuickTry\Relocfil.obj
        @-del ..\modbins\dfragfat.bak

dfragfat.obj: dfragfat.c ..\..\engine\header\fte.h ..\..\modlgate\expected.h
        @$(compiler) $(options) -I$(incdir) dfragfat.c

dfrgdrvr.obj: dfrgdrvr.c ..\..\engine\header\fte.h
        @$(compiler) $(options) -I$(incdir) dfrgdrvr.c

FllDfrag\FllDfrg.obj: FllDfrag\FllDfrg.c ..\..\engine\header\fte.h DfrgDrvr.h
     @$(compiler) $(options) -I$(incdir) -oFllDfrag\FllDfrg.obj FllDfrag\FllDfrg.c 

clear\ClrUSct.obj: clear\ClrUsct.c ..\..\engine\header\fte.h
     @$(compiler) $(options) -I$(incdir) -oclear\ClrUSct.obj clear\ClrUSct.c 

ordering\ordrdfrg.obj: ordering\ordrdfrg.c ..\..\engine\header\fte.h
     @$(compiler) $(options) -I$(incdir) -oordering\ordrdfrg.obj ordering\ordrdfrg.c

crunch\crunch.obj: crunch\crunch.c ..\..\engine\header\fte.h
     @$(compiler) $(options) -I$(incdir) -ocrunch\crunch.obj crunch\crunch.c

defrmap.obj: defrmap.c ..\..\engine\header\fte.h
     @$(compiler) $(options) -I$(incdir) -odefrmap.obj defrmap.c

unfrgfls\unfrag.obj: unfrgfls\unfrag.c
     @$(compiler) $(options) -I$(incdir) -ounfrgfls\unfrag.obj unfrgfls\unfrag.c

unfrgfls\isflfgtd.obj: unfrgfls\isflfgtd.c
     @$(compiler) $(options) -I$(incdir) -ounfrgfls\isflfgtd.obj unfrgfls\isflfgtd.c  

QuickTry\POWALKTR.obj: QuickTry\POWALKTR.c
     @$(compiler) $(options) -I$(incdir) -oQuickTry\POWALKTR.obj QuickTry\POWALKTR.c  

QuickTry\QuickTry.obj: QuickTry\QuickTry.c
     @$(compiler) $(options) -I$(incdir) -oQuickTry\QuickTry.obj QuickTry\QuickTry.c  

QuickTry\RelocFil.obj: QuickTry\RelocFil.c
     @$(compiler) $(options) -I$(incdir) -oQuickTry\RelocFil.obj QuickTry\RelocFil.c  

clean:
      @-del dfragfat.obj
      @-del dfrgdrvr.obj
      @-del defrmap.obj
      @-del ordering\ordrdfrg.obj
      @-del FllDfrag\FllDfrg.obj
      @-del clear\ClrUSct.obj
      @-del crunch\crunch.obj
      @-del unfrgfls\unfrag.obj        
      @-del unfrgfls\isflfgtd.obj
      @-del QuickTry\POWALKTR.obj      
      @-del QuickTry\QuickTry.obj      
      @-del QuickTry\RelocFil.obj                          
      @-del ..\modbins\dfragfat.lib



