#
# Main makefile for chkdsk
#

!include "config.mak"

includepath = misc;engine\header

..\bin\chkdsk.exe: chkdsk.exe 
        @copy chkdsk.exe ..\..\bin

chkdsk.exe: chkdsk.obj chkdrvr.obj chkargs.obj engine\lib\fte.lib fat\chkfat.lib \
            summary\summary.lib boot\chkboot.lib dirs\dirs.lib misc\misc.lib \
            struct\struct.lib fragfact\fragfact.lib surface\surface.lib \
            errmsgs\errmsgs.lib
        @-del chkdsk.lib
        @$(libman) chkdsk.lib + chkdrvr.obj
        @$(libman) chkdsk.lib + chkargs.obj
        @$(libman) chkdsk.lib + engine\lib\fte.lib
        @$(libman) chkdsk.lib + fat\chkfat.lib
        @$(libman) chkdsk.lib + summary\summary.lib
        @$(libman) chkdsk.lib + boot\chkboot.lib
        @$(libman) chkdsk.lib + dirs\dirs.lib
        @$(libman) chkdsk.lib + misc\misc.lib
        @$(libman) chkdsk.lib + struct\struct.lib
        @$(libman) chkdsk.lib + fragfact\fragfact.lib
        @$(libman) chkdsk.lib + surface\surface.lib
        @$(libman) chkdsk.lib + errmsgs\errmsgs.lib
        @$(linker) -M chkdsk.obj chkdsk.lib
        @-del chkdsk.lib
        @-del chkdsk.bak

chkdsk.obj: chkdsk.c checks.h chkdrvr.h
        @$(compiler) $(options) -I$(includepath) $(logging) chkdsk.c

chkdrvr.obj: chkdrvr.c chkdrvr.h
        @$(compiler) $(options) -I$(includepath) $(logging) chkdrvr.c

chkargs.obj: chkargs.c chkdrvr.h
        @$(compiler) $(options) -I$(includepath) chkargs.c        
        
engine\lib\fte.lib:
        @cd engine\lib
        @makefte
        @cd ..\..

fat\chkfat.lib:
        @cd fat
        @make
        @cd ..
        
summary\summary.lib:
        @cd summary
        @make
        @cd ..

boot\chkboot.lib:
        @cd boot
        @make
        @cd ..

dirs\dirs.lib:
        @cd dirs
        @make
        @cd ..

misc\misc.lib:
        @cd misc
        @make
        @cd ..

struct\struct.lib:
        @cd struct 
        @make
        @cd ..

fragfact\fragfact.lib:
        @cd fragfact 
        @make
        @cd ..

surface\surface.lib:
        @cd surface
        @make
        @cd ..

errmsgs\errmsgs.lib:
        @cd errmsgs
        @make
        @cd ..

clean:
        @-del chkdsk.exe
        @-del chkdrvr.obj
        @-del chkdsk.obj
        @-del chkargs.obj
        @-del chkdsk.lib
        @-del chkdsk.bak
        @cd engine\lib
        @make clean
        @cd ..\..
        @cd fat
        @make clean
        @cd ..
        @cd summary
        @make clean
        @cd ..
        @cd boot
        @make clean
        @cd ..
        @cd dirs
        @make clean
        @cd ..
        @cd struct
        @make clean
        @cd ..
        @cd fragfact
        @make clean
        @cd ..
        @cd surface
        @make clean
        @cd ..
        @cd errmsgs
        @make clean
        @cd ..
