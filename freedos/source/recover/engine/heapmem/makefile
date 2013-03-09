#
# makefile for files in fte\low
#

!include "..\..\config.mak"

all: heapmem.lib
        @copy heapmem.lib ..\lib
        @echo FTE heap memory management compiled.

heapmem.lib: backmem.obj suremem.obj ftemem.obj
        @-del heapmem.lib
        @-del heapmem.bak
        @$(libman) heapmem + backmem.obj
        @$(libman) heapmem + suremem.obj
        @$(libman) heapmem + ftemem.obj
        @-del heapmem.bak

backmem.obj: backmem.c ..\..\misc\bool.h ..\header\backmem.h \
              ..\header\fteerr.h
        @$(compiler) $(options) -I..\header;..\..\misc backmem.c

suremem.obj: backmem.c ..\..\misc\bool.h ..\header\suremem.h \
             ..\header\fteerr.h
        @$(compiler) $(options) -I..\header;..\..\misc suremem.c

ftemem.obj: ftemem.c ..\..\misc\bool.h ..\header\ftemem.h \
            ..\header\backmem.h ..\header\suremem.h ..\header\fteerr.h
        @$(compiler) $(options) $(memory_manager) -I..\header;..\..\misc ftemem.c

clean:
        @-del backmem.obj
        @-del suremem.obj
        @-del ftemem.obj
        @-del heapmem.lib
        @-del *.bak
