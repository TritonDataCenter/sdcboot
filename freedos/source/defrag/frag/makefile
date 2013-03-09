#
#  Makefile for files in this directory.
#

!include "..\config.mak"

frag.exe: frag.obj ..\engine\lib\fte.lib 
        @$(linker) frag.obj ..\engine\lib\fte.lib    

frag.obj:
        @$(compiler) $(options) -I..\misc;..\engine\header frag.c
    
..\engine\lib\fte.lib: 
        @cd ..\engine\lib
        @makefte
        @cd ..\..\frag    
    
clean:
        @-del frag.obj
        @-del *.bak 
