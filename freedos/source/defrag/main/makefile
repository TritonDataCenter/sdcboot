#
#  Makefile for files in this directory.
#

!include "..\config.mak"

all: defrag.obj
     @echo Defrag startup code compiled.

defrag.obj: defrag.c
     $(compiler) $(options) defrag.c

clean:
     @-del defrag.obj
     @-del *.bak   
