#
# Makefile for the special speed up structures
#

compiler = tcc -c 

options = -w -d -O -Z -ml

libman = tlib

includepath = ..\engine\header;..\misc

all: struct.lib
        @echo Special speed up structures compiled

struct.lib: fsttrmap.obj intelwlk.obj
       @-del struct.lib
       @$(libman) struct.lib + fsttrmap.obj
       @$(libman) struct.lib + intelwlk.obj
       @-del struct.bak

fsttrmap.obj: fsttrmap.c
       @$(compiler) $(options) -I$(includepath) fsttrmap.c
       
intelwlk.obj: intelwlk.c
       @$(compiler) $(options) -I$(includepath) intelwlk.c               

clean:
        @-del fsttrmap.obj
        @-del intelwlk.obj
        @-del struct.lib