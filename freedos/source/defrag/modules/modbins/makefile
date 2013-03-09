#
# Makefile to put all modules in one lib.
#

!include "..\..\config.mak"

all: modules.lib
        @echo General module lib created.

modules.lib: infofat.lib checkfat.lib sortfat.lib
        @-del modules.lib        
        @-del modules.bak
        @$(libman) modules.lib + infofat.lib
        @$(libman) modules.lib + checkfat.lib
        @$(libman) modules.lib + sortfat.lib
        @$(libman) modules.lib + dfragfat.lib
        @$(libman) modules.lib + dtstruct.lib
        @-del modules.bak

clean:
      @-del modules.lib
      @-del modules.bak