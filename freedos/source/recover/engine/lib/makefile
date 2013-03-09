#
# Makefile for the FAT transformation engine
#

!include "..\..\config.mak"

fte.lib: low.lib high.lib misc.lib apmlib.lib cache.lib disklib.lib \
         heapmem.lib dtstrct.lib
        @-del fte.lib
        @-del fte.bak
        @$(libman) fte.lib + low.lib
        @$(libman) fte.lib + high.lib
        @$(libman) fte.lib + misc.lib
        @$(libman) fte.lib + apmlib.lib
        @$(libman) fte.lib + cache.lib
        @$(libman) fte.lib + disklib.lib
        @$(libman) fte.lib + heapmem.lib
        @$(libman) fte.lib + dtstrct.lib
        @-del fte.bak
        
low.lib:
        @cd ..\low
        @make
        @cd ..\lib
        
high.lib:
        @cd ..\high
        @make
        @cd ..\lib
        
misc.lib:
        @cd ..\misc
        @make
        @cd ..\lib

apmlib.lib: 
        @cd ..\apmlib
        @make
        @cd ..\lib

cache.lib:
        @cd ..\cache
        @make
        @cd ..\lib
        
disklib.lib:
   @cd ..\disklib
        @make
   @cd ..\lib
     
heapmem.lib:
        @cd ..\heapmem
        @make
        @cd ..\lib

dtstrct.lib:
        @cd ..\dtstrct
        @make
        @cd ..\lib

clean:
        @cd ..\low
        @make clean
        @cd ..\lib
        @cd ..\high
        @make clean
        @cd ..\lib
        @cd ..\misc
        @make clean
        @cd ..\lib
        @cd ..\apmlib
        @make clean
        @cd ..\lib
        @cd ..\cache
        @make clean
        @cd ..\lib
        @cd ..\disklib
        @make clean
        @cd ..\lib
        @cd ..\heapmem
        @make clean
        @cd ..\lib
        @cd ..\dtstrct
        @make clean
        @cd ..\lib
        @-del misc.lib
        @-del apmlib.lib
        @-del cache.bak     
        @-del misc.lib
        @-del apmlib.lib
        @-del cache.lib    
        @-del disklib.lib           
        @-del fte.lib
        @-del heapmem.lib
        @-del high.lib
        @-del low.lib
        @-del dtstrct.lib
        @-del misc.bak
        @-del apmlib.bak
        @-del cache.bak     
        @-del misc.bak
        @-del apmlib.bak
        @-del cache.bak    
        @-del disklib.bak
        @-del fte.bak
        @-del heapmem.bak
        @-del high.bak
        @-del low.bak
        @-del dtstrct.bak
