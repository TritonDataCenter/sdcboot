#
# Makefile for the checks on the FAT
#

compiler = tcc -c 

options = -w -d -O -Z -ml

libman = tlib

includepath = ..\engine\header;..\misc

all: chkboot.lib
        @echo BOOT checks compiled

chkboot.lib: fsstring.obj descifat.obj descchk.obj  
       @-del chkboot.lib
       @$(libman) chkboot.lib + fsstring.obj
       @$(libman) chkboot.lib + descifat.obj
       @$(libman) chkboot.lib + descchk.obj
       @-del chkboot.bak

fsstring.obj: fsstring.c
       @$(compiler) $(options) -I$(includepath) fsstring.c

descifat.obj: descifat.c
       @$(compiler) $(options) -I$(includepath) descifat.c

descchk.obj: descchk.c
       @$(compiler) $(options) -I$(includepath) descchk.c

clean:
        @-del fsstring.obj
        @-del descifat.obj
        @-del descchk.obj
        @-del chkboot.lib
