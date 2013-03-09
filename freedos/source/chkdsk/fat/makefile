#
# Makefile for the checks on the FAT
#

compiler = tcc -c 

options = -w -d -O -Z -ml

libman = tlib

includepath = ..\engine\header;..\misc

all: chkfat.lib
        @echo FAT checks compiled

chkfat.lib: lblvalid.obj dfatchk.obj crosslnk.obj lostclts.obj
       @-del chkfat.lib
       @$(libman) chkfat.lib + lblvalid.obj
       @$(libman) chkfat.lib + dfatchk.obj
       @$(libman) chkfat.lib + crosslnk.obj
       @$(libman) chkfat.lib + lostclts.obj
       @-del chkfat.bak

lblvalid.obj: lblvalid.c
       @$(compiler) $(options) -I$(includepath) lblvalid.c

dfatchk.obj: dfatchk.c
       @$(compiler) $(options) -I$(includepath) dfatchk.c

crosslnk.obj: crosslnk.c
       @$(compiler) $(options) -I$(includepath) crosslnk.c

lostclts.obj: lostclts.c
       @$(compiler) $(options) -I$(includepath) lostclts.c

clean:
        @-del lblvalid.obj
        @-del dfatchk.obj
        @-del crosslnk.obj
        @-del lostclts.obj
        @-del chkfat.lib
