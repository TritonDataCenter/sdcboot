#
# Makefile for CheckFAT module
#

!include "..\..\config.mak"

incpath = ..\..\engine\header;..\..\misc

checkfat.lib: chkvol.obj descchk.obj descifat.obj dfatchk.obj checkfat.obj 
     @-del checkfat.lib
     @$(libman) checkfat.lib + chkvol.obj
     @$(libman) checkfat.lib + descchk.obj
     @$(libman) checkfat.lib + descifat.obj
     @$(libman) checkfat.lib + dfatchk.obj
     @$(libman) checkfat.lib + checkfat.obj
     @-del checkfat.bak
     @copy checkfat.lib ..\modbins
     @echo FAT integrity checks compiled

chkvol.obj: chkvol.c ..\..\engine\header\fte.h \
            checkfat.h chkrouts.h
     @$(compiler) $(options) -I$(incpath);..\..\modlgate chkvol.c

descchk.obj: descchk.c ..\..\engine\header\fte.h checkfat.h
     @$(compiler) $(options) -I$(incpath) descchk.c

descifat.obj: descifat.c ..\..\engine\header\fte.h checkfat.h
     @$(compiler) $(options) -I$(incpath) descifat.c

dfatchk.obj: dfatchk.c  ..\..\engine\header\fte.h checkfat.h
     @$(compiler) $(options) -I$(incpath) dfatchk.c

checkfat.obj: checkfat.c ..\..\engine\header\fte.h checkfat.h
     @$(compiler) $(options) -I$(incpath);..\..\modlgate checkfat.c

clean:
     @-del chkvol.obj
     @-del descchk.obj
     @-del descifat.obj
     @-del dfatchk.obj
     @-del checkfat.obj
     @-del checkfat.lib
     @-del checkfat.bak
     @-del ..\modbins\checkfat.lib
     @-del ..\modbins\checkfat.bak