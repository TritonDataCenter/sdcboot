# this is a DOS makefile

CC=wcl
CFLAGS=-c -fo=.obj -oasl -bt=DOS -zq -ml -DNDEBUG -i=../include

all: libs dat.obj

libs:
	cd lsm
	wmake -f makefile.wat
	cd ../cats
	wmake -f makefile.wat libs
	cd ../unzip
	wmake  -f msdos/makefile.wat unzip.lib
	cd ..

clean: libclean
	del *.obj
	del *.lib

dat.obj : dat.c
	$(CC) $(CFLAGS) dat.c

libclean:
	cd lsm
	wmake -f makefile.wat clean
	cd ../cats
	wmake -f makefile.wat distclean
	cd ../unzip
	wmake -f msdos/makefile.wat clean
	cd ..
