#
# Makefile for move
#

############# [Open]Watcom ########################
#CC=wcl
#CFLAGS= -DINLINE -DHAS_DIR_PERMS -oas -d0 -ms -fe=

############# Turbo/Borland C[++] #################
CC=tcc
#CC=bcc
CFLAGS= -DINLINE -O -d -k- -Z -a -w -ms -e

SRCS=move.c misc.c movedir.c

# Targets:

all: _move.exe move.exe
   @copy _move.exe ..\bin
   @copy move.exe ..\bin

release: _move.exe move.exe
   @upx --8086 --best _move.exe
   @upx --8086 --best move.exe
   @copy _move.exe ..\bin
   @copy move.exe ..\bin

_move.exe: $(SRCS) makefile
   @$(CC) $(CFLAGS)_move $(SRCS)

move.exe: $(SRCS) kitten.c makefile
   @$(CC) -DUSE_KITTEN $(CFLAGS)move $(SRCS) kitten.c

clean: 
   @-del move.obj
   @-del misc.obj
   @-del movedir.obj
   @-del kitten.obj

distclean: clean
   @-del _move.exe
   @-del move.exe
