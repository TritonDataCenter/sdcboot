# makefile for compiling the FreeDOS INSTALL program

CC=wcc
CFLAGS=-ml -q -sg

CL=wcl
LFLAGS=-q

LIBUNZIP=unzip60/unzip.lib

OBJS=pkginst.obj progress.obj istrichr.obj window.obj yesno.obj

all: install.exe unzip.exe reset.exe .symbolic

install.exe: install.obj $(OBJS) $(LIBUNZIP)
	$(CL) $(LFLAGS) install.obj $(OBJS) $(LIBUNZIP)

unzip.exe: unzip.obj
	$(CL) $(LFLAGS) unzip.obj $(LIBUNZIP)

reset.exe: reset.obj
	$(CL) $(LFLAGS) reset.obj

$(LIBUNZIP):
	cd unzip60
	wmake unzip.lib
	cd ..

# deps:

.c.obj: .autodepend
	$(CC) $(CFLAGS) $<

distclean: realclean .symbolic
	-del *.exe
	cd unzip60
	$(MAKE) cleaner
	-del unzip.lib
	cd ..
	-deltree /y test

realclean: clean .symbolic
	-del *.obj

clean: .symbolic
	-del *.err
