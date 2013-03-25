# MS-DOS Makefile for GNU grep
# Copyright (C) 1995 Summer Institute of Linguistics, Inc.

# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 2, or (at your option)
# any later version.

# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.

# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.

CFLAGS=-O -DGREP -DHAVE_CONFIG_H -I.

.SUFFIXES:

###############################################################################
#
#	Microsoft C 7.0+ rules
#
###############################################################################

.SUFFIXES: .c .obj
.c.obj:
	cl -Za -AL $(CFLAGS) -c $<

OBJ=grep.obj getopt.obj regex.obj dfa.obj kwset.obj obstack.obj search.obj
GLOBJ=ndir.obj cwild.obj dosglob.obj fnmatch.obj

all: grep.exe

grep.exe: $(OBJ) $(GLOBJ) grep.lnk
	link @grep.lnk
grep.lnk: makefile
	echo $(OBJ) +>grep.lnk
	echo $(GLOBJ) \msvc\lib\setargv >>grep.lnk
	echo grep/noe/stack:32768; >>grep.lnk

check: grep.exe
	cd tests
	check
	cd ..

###############################################################################
#
#	GNU C (DJGPP) compiling and linking
#
###############################################################################

.SUFFIXES: .o
.c.o:
	gcc -g $(CFLAGS) -I. -c $<

386: grep386.exe

O386=grep.o getopt.o regex.o dfa.o kwset.o obstack.o search.o

grep386.exe: $(O386)
	gcc -g $(O386)
	copy a.out grep386
	strip grep386
	coff2exe grep386
	copy a.out grep386
	del a.out

check386: grep386.exe
	cd tests
	check 386
	cd ..

###############################################################################

dfa.o     dfa.obj:     dfa.c     dfa.h regex.h
getopt.o  getopt.obj:  getopt.c  getopt.h
grep.o    grep.obj:    grep.c    grep.h getpages.h
kwset.o   kwset.obj:   kwset.c   kwset.h obstack.h
obstack.o obstack.obj: obstack.c obstack.h
regex.o   regex.obj:   regex.c   regex.h
search.o  search.obj:  search.c  grep.h dfa.h kwset.h regex.h

ndir.obj:    msdos/ndir.c    msdos/ndir.h
	cl -AL $(CFLAGS) -Imsdos -c -Fondir.obj msdos/ndir.c
cwild.obj:  msdos/cwild.c  msdos/dosglob.h
	cl -AL $(CFLAGS) -Imsdos -c -Focwild.obj msdos/cwild.c
dosglob.obj:    msdos/dosglob.c    msdos/dosglob.h   msdos/fnmatch.h
	cl -AL $(CFLAGS) -Imsdos -c -Fodosglob.obj msdos/dosglob.c
fnmatch.obj: msdos/fnmatch.c config.h       msdos/fnmatch.h
	cl -AL $(CFLAGS) -Imsdos -c -Fofnmatch.obj msdos/fnmatch.c

###############################################################################

install: all
	copy grep.exe c:\bin\grep.exe
	-md c:\docs
	-md c:\docs\grep
	copy msdos\grep.doc c:\docs\grep\grep.man

clean:
	-del *.o*
	-del grep.lnk
	-del grep386.
