# Makefile for SORT (using Borland C or Turbo C)
# Based on the CHOICE makefile by Tom Ehlert

# set 
#   UPX=-rem 
# if you dont want to UPX choice
# if yu use upx: --8086 for 8086 compatibility, --best for smallest

# UPX=-rem
UPX=upx --8086

############# WATCOM ########################
# CC=wcl
# CFLAGS=-oas -s -wx -we -zq -fm -fe=

############# TURBO_C ########################
# -w warn -M create map -f- no floating point -Z register optimize
# -O jump optimize -k- no standard stack frome -K unsigned char
# -exxx executable name (must be last) -mt tiny (default is small)
# -N stack checking -a- byte alignment  -ln no default libs
# -lt create .com file -lx no map file ...
# *** changed 7/2004: tiny has near qsort / malloc pointers, very limited!
# compact large: large has far function pointers, compact only far heap.
# makes no real difference here, but compact saves a few bytes in size.
CC=tcc
CFLAGS=-w -M -f- -a- -K -ln -e
COMFLAGS=-mt -lt -Z -O -k-
EXEFLAGS=-mc -N -Z -O -k-

CFILES=sort.c lfnapi.c l2s95.c

# prf.c talloc.c tcdummy.c

# targets:

all: sort.com
### can be sort.com for tiny model - but then limited to < 32k file size.
### .com is about 4.5k upxed, .exe about 5.7k ... full sizes: 6.9k / 9.3k

# no space between -e and sort (set exe file name)!
# upx either sort.com or sort.exe, depends...

sort.com: $(CFILES) kitten.c makefile
   $(CC) $(COMFLAGS) $(CFLAGS)sort $(CFILES) kitten.c 
   $(UPX) sort.com

sort.exe: $(CFILES) kitten.c makefile
   $(CC) $(EXEFLAGS) $(CFLAGS)sort $(CFILES) kitten.c 
   $(UPX) sort.exe


clean:
   -del *.obj

