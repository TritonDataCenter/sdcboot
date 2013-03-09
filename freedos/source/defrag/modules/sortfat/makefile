#
# Makefile for files in this directory.
#

!include "..\..\config.mak"

include = -I..\..\engine\header;..\..\misc

all: sortdir\sortdir.exe
        @echo Directory sort module compiled 

sortdir\sortdir.exe: ..\modbins\sortfat.lib obj\sortdir.obj
   @-$(linker) -N -M -esortdir\sortdir.exe obj\sortdir.obj ..\modbins\sortfat.lib ..\..\engine\lib\fte.lib ..\..\misc\misc.lib

..\modbins\sortfat.lib: obj obj\sorttree.obj obj\sortcfgf.obj obj\ssortdir.obj \
         obj\srtentrs.obj obj\cmpfncs.obj obj\dsksort.obj \
         obj\memsort.obj obj\rdentrs.obj obj\wrentrs.obj obj\sortfat.obj
   @-del ..\modbins\sortfat.lib
   @-del ..\modbins\sortfat.bak
   @$(libman) ..\modbins\sortfat.lib + obj\sorttree.obj
   @$(libman) ..\modbins\sortfat.lib + obj\sortcfgf.obj
   @$(libman) ..\modbins\sortfat.lib + obj\ssortdir.obj
   @$(libman) ..\modbins\sortfat.lib + obj\srtentrs.obj
   @$(libman) ..\modbins\sortfat.lib + obj\cmpfncs.obj
   @$(libman) ..\modbins\sortfat.lib + obj\dsksort.obj
   @$(libman) ..\modbins\sortfat.lib + obj\memsort.obj
   @$(libman) ..\modbins\sortfat.lib + obj\rdentrs.obj
   @$(libman) ..\modbins\sortfat.lib + obj\wrentrs.obj
   @$(libman) ..\modbins\sortfat.lib + obj\sortfat.obj
   @-del sortfat.bak

obj\sorttree.obj: sorttree.c ..\..\engine\header\fte.h
        @$(compiler) $(options) $(include) -oobj\sorttree.obj sorttree.c

obj\sortcfgf.obj: sortcfgf.c ..\..\engine\header\fte.h sortfatf.h
        @$(compiler) $(options) $(include) -oobj\sortcfgf.obj sortcfgf.c

obj\ssortdir.obj: ssortdir.c ..\..\engine\header\fte.h sortfatf.h
       @$(compiler) $(options) $(include) -I..\..\modlgate -oobj\ssortdir.obj ssortdir.c         

obj\srtentrs.obj: srtentrs.c ..\..\engine\header\fte.h sortfatf.h
       @$(compiler) $(options) $(include) -I..\..\modlgate -oobj\srtentrs.obj srtentrs.c 

obj\cmpfncs.obj: cmpfncs.c ..\..\engine\header\fte.h sortfatf.h
       @$(compiler) $(options) $(include) -oobj\cmpfncs.obj cmpfncs.c

obj\sortdir.obj: sortdir\sortdir.c ..\..\engine\header\fte.h sortfatf.h
       @$(compiler) $(options) $(include) -I..\..\engine\apmlib\include sortdir\sortdir.c
       @move sortdir.obj obj

obj\dsksort.obj: dskcfg\dsksort.c ..\..\engine\header\fte.h sortfatf.h
       @$(compiler) $(options) $(include) -oobj\dsksort.obj dskcfg\dsksort.c 

obj\memsort.obj: memcfg\memsort.c ..\..\engine\header\fte.h sortfatf.h
       @$(compiler) $(options) $(include) -oobj\memsort.obj memcfg\memsort.c

obj\rdentrs.obj: memcfg\rdentrs.c ..\..\engine\header\fte.h sortfatf.h
       @$(compiler) $(options) $(include) -I..\..\modlgate -oobj\rdentrs.obj memcfg\rdentrs.c

obj\wrentrs.obj: memcfg\wrentrs.c ..\..\engine\header\fte.h sortfatf.h
       @$(compiler) $(options) $(include) -I..\..\modlgate -oobj\wrentrs.obj memcfg\wrentrs.c

obj\sortfat.obj: sortfat.c ..\..\engine\header\fte.h sortfatf.h
       @$(compiler) $(options) $(include) -I..\..\modlgate -oobj\sortfat.obj sortfat.c

obj: 
       @md obj 

clean:
        @-del sortfat.lib
        @-del ..\modbins\sortfat.lib
        @-del obj\sorttree.obj
        @-del obj\sortcfgf.obj
        @-del obj\ssortdir.obj
        @-del obj\srtentrs.obj
        @-del obj\cmpfncs.obj
        @-del obj\memsort.obj
        @-del obj\dsksort.obj
        @-del obj\rdentrs.obj
        @-del obj\wrentrs.obj 
        @-del obj\wrentrs.obj
        @-del obj\sortdir.obj
        @-del obj\sortfat.obj
        @-del sortdir\sortdir.exe   
   
