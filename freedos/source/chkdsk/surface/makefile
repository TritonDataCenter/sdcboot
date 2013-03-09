#
# Makefile for surface scanning
#

compiler = tcc -c 

options = -w -d -O -Z -ml

libman = tlib

includepath = ..\engine\header;..\misc

all: surface.lib
       @echo Surface scanning compiled

surface.lib: surface.obj   
       @-del surface.lib
       @$(libman) surface.lib + surface.obj
       @-del surface.bak

surface.obj: surface.c
       @$(compiler) $(options) -I$(includepath) surface.c

clean:
       @-del surface.obj
       @-del surface.bak
       @-del surface.lib
