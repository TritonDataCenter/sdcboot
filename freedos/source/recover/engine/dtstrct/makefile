#
# Makefile for the data structures
#

!include "..\..\config.mak"

all: dtstrct.lib
        @copy dtstrct.lib ..\lib
        @echo Data structures compiled

dtstrct.lib: bitfield.obj 
       @-del dtstrct.lib
       @$(libman) dtstrct.lib + bitfield.obj
       @-del dtstrct.bak

bitfield.obj: bitfield.c
       @$(compiler) $(options) -I$(includepath) bitfield.c

clean:
        @-del bitfield.obj
        @-del dtstrct.lib
        @-del dtstrct.bak