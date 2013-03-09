#
# Makefile for the recover specific code
#

!include "..\config.mak"

misc.lib: switchch.obj path.obj truname.obj intelwlk.obj
    @-del misc.lib
    @tlib misc.lib + switchch.obj
    @tlib misc.lib + path.obj
    @tlib misc.lib + intelwlk.obj
    @tlib misc.lib + truname.obj
    @-del misc.bak

switchch.obj:
    @nasm switchch.asm -f obj -o switchch.obj
    
path.obj: path.c
    @$(compiler) $(options) path.c  
    
truname.obj: truname.c
    @$(compiler) $(options) truname.c  

intelwlk.obj: intelwlk.c
    @$(compiler) $(options) -I..\misc;..\engine\header intelwlk.c  
    
clean:
    @-del misc.lib
    @-del switchch.obj
    @-del path.obj
    @-del truname.obj
    @-del intelwlk.obj
    @-del misc.bak
