# this is a Borland makefile


# set 
#   UPX=-rem 
# if you dont want to UPX choice

UPX=-rem
UPX=upx --8086 --best


LIBFILES=kitten.c tcdummy.c talloc.c prf.c lfnapi.c l2s95.c find95.c     


############# WATCOM ########################
CC=wcl
CFLAGS=-oas -s -wx -we -zq -fm -fe=

############# TURBO_C ########################

CC=tcc
CFLAGS=-w -M -Z -O -k- -e

all: more.exe

more.exe: more.c $(LIBFILES)
 $(CC)  -DNOCATS  $(CFLAGS)_more more.c $(LIBFILES)
 $(UPX) _more.exe
 $(CC)            $(CFLAGS)more  more.c $(LIBFILES)
 $(UPX) more.exe

realclean clean:
	-del *.obj

distclean: clean
	-del *.exe
