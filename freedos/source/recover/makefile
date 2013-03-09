#
# Makefile for recover.exe
#

!include "config.mak"

Recover.exe: recover.obj misc\misc.lib specific\specific.lib engine\lib\fte.lib
    @-del recover.exe
    @tlib recover.lib + misc\misc.lib
    @tlib recover.lib + specific\specific.lib
    @tlib recover.lib + engine\lib\fte.lib
    @$(linker) recover.obj recover.lib 
    @-del recover.lib 
    @copy recover.exe ..\..\bin

recover.obj: recover.c
    @$(compiler) $(options) recover.c


misc\misc.lib:
    @cd misc
    @make
    @cd ..

specific\specific.lib:
    @cd specific 
    @make
    @cd ..

engine\lib\fte.lib:
    @cd engine\lib
    @makefte
    @cd ..\..    
    
clean:
    @-del recover.lib
    @-del recover.obj
    @-del recover.exe
    @cd misc
    @make clean
    @cd ..
    @cd specific
    @make clean
    @cd ..
    @cd engine\lib
    @cleanfte
    @cd ..\..
