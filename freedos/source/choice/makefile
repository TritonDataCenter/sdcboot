# Makefile for CHOICE (using Borland C)
# Copyright (C) 1998-2000 Jim Hall <jhall@freedos.org>

# set 
#   UPX=-rem 
# if you dont want to UPX choice

UPX=-rem
UPX=upx --8086 --best

############# WATCOM ########################
CC=wcl
CFLAGS=-oas -s -wx -we -zq -fm -fe=

############# TURBO_C ########################

CC=tcc
CFLAGS=-w -M -Z -O -k- -e


CFILES=choice.c prf.c talloc.c tcdummy.c

# targets:

all: choice.exe





choice.exe: $(CFILES) kitten.c makefile
	$(CC) $(CFLAGS)choice $(CFILES) kitten.c
	$(UPX) choice.exe
	$(CC) -DNOCATS $(CFLAGS)_choice $(CFILES)
	$(UPX) _choice.exe

clean:
	-del *.obj

realclean: clean
	cd getopt
	$(MAKE) -f makefile.bc $@
	cd ..

distclean: real clean
	-del *.exe
