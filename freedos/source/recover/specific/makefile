#
# Makefile for the recover specific code
#

!include "..\config.mak"

specific.lib: RecovDsk.obj RecovFil.obj crosslnk.obj lostclts.obj
    @-del specific.lib
    @tlib specific.lib + RecovDsk.obj
    @tlib specific.lib + RecovFil.obj
    @tlib specific.lib + crosslnk.obj
    @tlib specific.lib + lostclts.obj    
    @-del specific.bak

RecovDsk.obj: RecovDsk.c
    @$(compiler) $(options) -I..\misc;..\engine\header RecovDsk.c

RecovFil.obj: RecovFil.c
    @$(compiler) $(options) -I..\misc;..\engine\header RecovFil.c

crosslnk.obj: crosslnk.c
    @$(compiler) $(options) -I..\misc;..\engine\header crosslnk.c

lostclts.obj: lostclts.c
    @$(compiler) $(options) -I..\misc;..\engine\header lostclts.c

clean:
    @-del specific.lib
    @-del specific.bak
    @-del RecovDsk.obj
    @-del RecovFil.obj
    @-del crosslnk.obj
    @-del lostclts.obj
