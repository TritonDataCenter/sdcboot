#
# Makefile for the checks on the FAT
#

compiler = tcc -c 

options = -w -d -O -Z -ml

libman = tlib

includepath = ..\engine\header;..\misc

all: errmsgs.lib
        @echo Error message reporting compiled

errmsgs.lib: chkmsgs.obj PrintBuf.obj
       @-del errmsgs.lib
       @$(libman) errmsgs.lib + chkmsgs.obj
       @$(libman) errmsgs.lib + PrintBuf.obj
       @-del errmsgs.bak

chkmsgs.obj: chkmsgs.c
       @$(compiler) $(options) -I$(includepath) chkmsgs.c

PrintBuf.obj: PrintBuf.c        
       @$(compiler) $(options) -I$(includepath) PrintBuf.c
       
clean:
        @-del chkmsgs.obj
        @-del PrintBuf.obj
        @-del errmsgs.lib