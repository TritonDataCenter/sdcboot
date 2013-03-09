# MODIFIED AND CUT DOWN VERSION FOR HTMLHELP
 
# Makefile for zlib
# Turbo C 2.01, Turbo C++ 1.01
# Last updated: 15-Mar-2003

# To use, do "make -fmakefile.tc"
# To compile in small model, set below: MODEL=s

# WARNING: the small model is supported but only for small values of
# MAX_WBITS and MAX_MEM_LEVEL. For example:
#    -DMAX_WBITS=11 -DMAX_MEM_LEVEL=3
# If you wish to reduce the memory requirements (default 256K for big
# objects plus a few K), you can add to CFLAGS below:
#   -DMAX_MEM_LEVEL=7 -DMAX_WBITS=14
# See zconf.h for details about the memory requirements.

# ------------ Turbo C 2.01, Turbo C++ 1.01 ------------
MODEL=l
#CC=tcc
CC=bcc
AR=tlib
CFLAGS=-O2 -G -Z -m$(MODEL)

# variables
ZLIB_LIB = unz\zlib_$(MODEL).lib

OBJ1 = adler32.obj crc32.obj
OBJ2 = inffast.obj inflate.obj inftrees.obj zutil.obj
OBJP1 = +adler32.obj+crc32.obj
OBJP2 = +inffast.obj+inflate.obj+inftrees.obj+zutil.obj


# targets
all: copyheaders $(ZLIB_LIB) cleanup

.c.obj:
   $(CC) -c $(CFLAGS) $*.c

adler32.obj: adler32.c zlib.h zconf.h

crc32.obj: crc32.c zlib.h zconf.h crc32.h

infback.obj: infback.c zutil.h zlib.h zconf.h inftrees.h inflate.h \
 inffast.h inffixed.h

inffast.obj: inffast.c zutil.h zlib.h zconf.h inftrees.h inflate.h \
 inffast.h

inflate.obj: inflate.c zutil.h zlib.h zconf.h inftrees.h inflate.h \
 inffast.h inffixed.h

inftrees.obj: inftrees.c zutil.h zlib.h zconf.h inftrees.h

zutil.obj: zutil.c zutil.h zlib.h zconf.h


# the command line is cut to fit in the MS-DOS 128 byte limit:
$(ZLIB_LIB): $(OBJ1) $(OBJ2)
   -del $(ZLIB_LIB)
   $(AR) $(ZLIB_LIB) $(OBJP1)
   $(AR) $(ZLIB_LIB) $(OBJP2)

copyheaders:
   -copy zlib.h unz\zlib.h
   -copy zconf.h unz\zconf.h

cleanup:
   -del *.obj
   -del unz\*.bak
