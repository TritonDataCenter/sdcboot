#
# Makefile for the checks on the FAT
#

compiler = tcc -c 

options = -w -d -O -Z -ml

libman = tlib

includepath = ..\engine\header;..\misc

all: fragfact.lib
        @echo Fragmentation checking compiled

fragfact.lib: defrfact.obj
       @-del fragfact.lib
       @$(libman) fragfact.lib + defrfact.obj
       @-del chkfat.bak

defrfact.obj: defrfact.c
       @$(compiler) $(options) -I$(includepath) defrfact.c

clean:
        @-del defrfact.obj
        @-del fragfact.lib
        @-del fragfact.bak
