#
# makefile for files in fte\low
#

!include "..\..\config.mak"

all: low.lib
        @copy low.lib ..\lib 
        @echo Low FTE routines compiled.

low.lib: rdwrsect.obj boot.obj direct.obj drive.obj fat.obj fteerr.obj \
    subdir.obj FSinfo.obj lowfat32.obj backup.obj volume.obj lfn.obj
   @-del low.lib 
   @-del low.bak
   @$(libman) low + rdwrsect.obj
   @$(libman) low + boot.obj
   @$(libman) low + direct.obj
   @$(libman) low + drive.obj
   @$(libman) low + fat.obj
   @$(libman) low + fteerr.obj
   @$(libman) low + subdir.obj
   @$(libman) low + FSinfo.obj
   @$(libman) low + LowFat32.obj
   @$(libman) low + backup.obj
   @$(libman) low + volume.obj
   @$(libman) low + lfn.obj
   @-del low.bak

rdwrsect.obj: rdwrsect.c ..\..\misc\bool.h ..\header\drive.h \
              ..\header\fteerr.h
        @$(compiler) $(options) $(caching) $(media) rdwrsect.c

boot.obj: boot.c ..\header\rdwrsect.h ..\..\misc\bool.h
        @$(compiler) $(options) boot.c

direct.obj: direct.c ..\header\rdwrsect.h ..\header\boot.h \
            ..\..\misc\bool.h ..\header\direct.h
        @$(compiler) $(options) direct.c

drive.obj: drive.c ..\..\misc\bool.h ..\header\drive.h
        @$(compiler) $(options) drive.c

fat.obj: fat.c ..\..\misc\bool.h ..\header\rdwrsect.h ..\header\fat.h \
         ..\header\boot.h ..\header\fatconst.h
        @$(compiler) $(options) fat.c

fteerr.obj: fteerr.c ..\header\fteerr.h
        @$(compiler) $(options) fteerr.c

subdir.obj: subdir.c ..\header\rdwrsect.h ..\header\direct.h    \
            ..\..\misc\bool.h ..\header\fat.h ..\header\subdir.h \
            ..\header\fatconst.h ..\header\boot.h
        @$(compiler) $(options) subdir.c

FSinfo.obj: fsinfo.c ..\..\misc\bool.h ..\header\rdwrsect.h \
       ..\header\fat.h ..\header\fatconst.h ..\header\boot.h \
       ..\header\fsinfo.h
   @$(compiler) $(options) FSinfo.c

lowfat32.obj: lowfat32.c
   @$(compiler) $(options) lowfat32.c
        
backup.obj: backup.c ..\..\misc\bool.h ..\header\rdwrsect.h ..\header\boot.h \
            ..\header\fat.h ..\header\fatconst.h ..\header\backup.h 
   @$(compiler) $(options) -I..\header -I..\..\misc backup.c

volume.obj: volume.c ..\..\misc\bool.h ..\header\rdwrsect.h ..\header\boot.h \
            ..\header\fat.h ..\header\fatconst.h ..\header\volume.h 
   @$(compiler) $(options) -I..\header -I..\..\misc volume.c        

lfn.obj: lfn.c ..\header\direct.h
   @$(compiler) $(options) -I..\header -I..\..\misc lfn.c        

clean:
   @-del rdwrsect.obj
   @-del boot.obj
   @-del direct.obj
   @-del drive.obj
   @-del fat.obj
   @-del fteerr.obj
   @-del subdir.obj
   @-del FSinfo.obj
   @-del lowfat32.obj
   @-del backup.obj
   @-del lfn.obj
   @-del low.lib
   @-del volume.obj
   @-del *.bak
