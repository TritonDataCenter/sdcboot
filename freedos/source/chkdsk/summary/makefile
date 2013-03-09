#
# Makefile for summary information
#

compiler = tcc -c 

options = -w -d -O -Z -ml

libman = tlib

includepath = ..\engine\header;..\misc

all: summary.lib
        @echo Summary information compiled

summary.lib: fatsum.obj filessum.obj summary.obj
       @-del summary.lib
       @$(libman) summary.lib + fatsum.obj
       @$(libman) summary.lib + filessum.obj
       @$(libman) summary.lib + summary.obj
       @-del summary.bak

fatsum.obj: fatsum.c fatsum.h
       @$(compiler) $(options) -I$(includepath) fatsum.c

filessum.obj: filessum.c filessum.h
       @$(compiler) $(options) -I$(includepath) filessum.c

summary.obj: summary.c fatsum.h filessum.h
       @$(compiler) $(options) -I$(includepath) summary.c

clean:
        @-del fatsum.obj
        @-del filessum.obj
        @-del summary.obj
        @-del summary.lib
