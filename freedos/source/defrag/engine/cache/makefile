#
# makefile for files in fte\cache
#

!include "..\..\config.mak"

include = -I..\..\misc;..\header

all: cache.lib
        @copy cache.lib ..\lib
        @echo FTE cache routines compiled.
        
cache.lib: sctcache.obj phscache.obj logcache.obj blkcache.obj xms.obj ems.obj \
           preread.obj WrtBack.obj
        @-del cache.lib
        @-del cache.bak
        @$(libman) cache + sctcache.obj
        @$(libman) cache + phscache.obj
        @$(libman) cache + logcache.obj
        @$(libman) cache + blkcache.obj
        @$(libman) cache + xms.obj
        @$(libman) cache + ems.obj
        @$(libman) cache + preread.obj                
        @$(libman) cache + WrtBack.obj                
        @-del cache.bak

sctcache.obj: sctcache.c 
        @$(compiler) $(options) $(include) $(caching) sctcache.c

phscache.obj: phscache.c 
        @$(compiler) $(options) $(include) phscache.c

logcache.obj: logcache.c 
        @$(compiler) $(options) $(include) logcache.c

blkcache.obj: blkcache.c 
        @$(compiler) $(options) $(include) blkcache.c

xms.obj: xms.asm
        @nasm xms.asm -f obj -o xms.obj

ems.obj: ems.asm
        @nasm ems.asm -f obj -o ems.obj

preread.obj: preread.c
        @$(compiler) $(options) $(include) preread.c
        
WrtBack.obj: wrtback.c
        @$(compiler) $(options) $(include) wrtback.c        
        
clean:
        @-del sctcache.obj
        @-del phscache.obj
        @-del logcache.obj
        @-del blkcache.obj
        @-del wrtback.obj
        @-del xms.obj
        @-del ems.obj
        @-del preread.obj
        @-del cache.lib
        @-del *.bak        
