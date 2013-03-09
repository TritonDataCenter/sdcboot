# simple makefile for compiling kitten

CC=wcc
CFLAGS=-ml -q


kitten.obj
	$(CC) $(CFLAGS) kitten.c


# cleanup:

distclean: realclean .symbolic
	-del *.lib

realclean: clean .symbolic
	-del *.obj

clean: .symbolic
	-del *.err
