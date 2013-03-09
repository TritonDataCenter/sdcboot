# Makefile for UnZip(SFX) and fUnZip for Borland C++ 2.x-4.x and Turbo C++ 1.0
# Version: 5.20 and later        Alvin Koh, Jim Knoble, Christian Spieler, etc.
#
# Last revised:  8 Apr 96
#
# To compile with Turbo C++ 1.0, set the macro CC_REV to 1 at the command line
# (make -fmsdos/makefile.bc -DCC_REV=1).

# Hacks added by Jim Hall (jhall1@isd.net) on 20 June 1998, to support
# building a DLL library.  To build a library, make sure BUILD_DLL is
# uncommented, and then do a 'make unzip.lib'.  To clean up, do a
# 'make libclean'.  When linking this library into your own project,
# make sure to compile your project with the proper memory model.


#    GNU make doesn't like the return value from "rem"
#STRIP=rem
STRIP=echo  Ignore this line.
#    If you don't have LZEXE or PKLITE, get one of them. Then define:
#STRIP=lzexe
#    or
#STRIP=pklite
#    This makes a big difference in .exe size (and possibly load time).


#    Optional nonstandard preprocessor flags (as -DCHECK_EOF or -DDOS_WILD)
#    should be added to the environment via "set LOCAL_UNZIP=-DFOO" or added
#    to the declaration of LOC here:
LOC = 

# Type for CPU required: 0: 8086, 1: 80186, 2: 80286, 3: 80386, etc.
CPU_TYP = 0

# (De)Select inclusion of optimized assembler CRC32 routine:
USE_ASMCRC = 1

CC_REV=0

!if $(CC_REV) == 1
# Turbo C++ 1.0
CC = wcc
!else
# Borland C++ 2.0, 3.0, 3.1 ...
! if !$(CC_REV)
CC_REV = 3
! endif
CC = wcl
!endif

AS = wasm

# "near data" model is sufficient for UnZip and ZipInfo, now that strings moved
# switched to medium model; UnZip code has grown beyond the 64k limit.
UNMODEL = l     # medium model for UnZip and ZipInfo
ASUNMODEL=__LARGE__ # keep in sync with UNMODEL definition !!

FUMODEL = s     # always use small model for fUnZip
ASFUMODEL=__SMALL__ # keep in sync with FUMODEL definition !!

!if $(USE_ASMCRC)
ASMFLG = -DASM_CRC
ASMOBJS = crc_i86.obj
ASMOBJF = crc_i86_.obj
!else
ASMFLG =
ASMOBJS =
ASMOBJF =
!endif

# Added by Jim Hall:
# Uncomment the following *if* you are building a DLL:
BUILD_DLL = -DDLL

# compiler flags
# BUILD_DLL added by Jim Hall:

ASCPUFLAG = __$(CPU_TYP)86
!if $(CPU_TYP) != 0
CC_CPUFLG = -$(CPU_TYP)
!endif
ASFLAGS = -bt=DOS -ml -0 -zq
!if $(CC_REV) == 1
CFLAGS = -DUSE_ZLIB -O -G -Z -a -d $(CC_CPUFLG) -ff- -k- -I. $(ASMFLG) $(LOC) $(BUILD_DLL)
LDFLAGS = -lxncd        # for tcc
!else
# CFLAGS = -O2 $(CC_CPUFLG) -ff- -k- -P-.C -I. $(ASMFLG) $(LOC) $(BUILD_DLL)
# LDFLAGS = -lxncd -l-P       # for bcc
CFLAGS=-fo=.obj -bt=DOS -ml -zq -oasl -DNO_ZIPINFO $(ASMFLG) $(LOC) $(BUILD_DLL) -wx
LDFLAGS=
!endif
UNFLAGS = -m$(UNMODEL) $(CFLAGS)
FUFLAGS = -m$(FUMODEL) $(CFLAGS) -K -d

# implicit rules

.asm.obj:
    $(AS) $(ASFLAGS) -D$(ASUNMODEL) $<

.c.obj:
    $(CC) -c $(UNFLAGS) $*.c

# list macros

unzip_dependencies = &
    unzip.obj &
    crc32.obj &
    crctab.obj &
    crypt.obj &
    envargs.obj &
    explode.obj &
    extract.obj &
    fileio.obj &
    globals.obj &
    inflate.obj &
    list.obj &
    match.obj &
    process.obj &
    ttyio.obj &
    unreduce.obj &
    unshrink.obj &
    zipinfo.obj &
    msdos.obj &
    $(ASMOBJS)

funzip_dependencies = &
    funzip.obj &
    crc32_.obj &
    crypt_.obj &
    globals_.obj &
    inflate_.obj &
    ttyio_.obj &
    $(ASMOBJF)

unzipsfx_dependencies = &
    unzipsfx.obj &
    crc32.obj &
    crctab.obj &
    crypt.obj &
    extract_.obj &
    fileio.obj &
    globals.obj &
    inflate.obj &
    match.obj &
    process_.obj &
    ttyio.obj &
    msdos_.obj &
    $(ASMOBJS)

# Added by Jim Hall:
dll_dependencies = &
    $(unzip_dependencies) &
    api.obj

UNZIP_H = unzip.h unzpriv.h globals.h msdos/doscfg.h

# explicit rules
all1: unzip.lib
all:    unzip.exe funzip.exe unzipsfx.exe

# Added by Jim Hall:
unzip.lib:      $(dll_dependencies)
	@if exist unzip.lib rm unzip.lib
    wlib -q unzip.lib +unzip.obj
    wlib -q unzip.lib +crc32.obj
    wlib -q unzip.lib +crctab.obj
    wlib -q unzip.lib +crypt.obj
    wlib -q unzip.lib +envargs.obj
    wlib -q unzip.lib +explode.obj
    wlib -q unzip.lib +extract.obj
    wlib -q unzip.lib +fileio.obj
    wlib -q unzip.lib +$(ASMOBJS)
    wlib -q unzip.lib +globals.obj
    wlib -q unzip.lib +inflate.obj
    wlib -q unzip.lib +list.obj
    wlib -q unzip.lib +match.obj
    wlib -q unzip.lib +process.obj
    wlib -q unzip.lib +ttyio.obj
    wlib -q unzip.lib +unreduce.obj
    wlib -q unzip.lib +unshrink.obj
    wlib -q unzip.lib +zipinfo.obj
    wlib -q unzip.lib +msdos.obj
    wlib -q unzip.lib +api.obj
#   tlib unzip.lib @&&|
# +unzip.obj +crc32.obj +crctab.obj +crypt.obj +envargs.obj +explode.obj &
# +extract.obj +fileio.obj +$(ASMOBJS) +globals.obj +inflate.obj &
# +list.obj +match.obj +process.obj +ttyio.obj +unreduce.obj &
# +unshrink.obj +zipinfo.obj +msdos.obj +api.obj
# |

#unzip.exe:      $(unzip_dependencies)
#    $(CC) -m$(UNMODEL) $(LDFLAGS) -eunzip.exe @&&|
#unzip.obj
#crc32.obj
#crctab.obj
#crypt.obj
#envargs.obj
#explode.obj
#extract.obj
#fileio.obj
#$(ASMOBJS)
#globals.obj
#inflate.obj
#list.obj
#match.obj
#process.obj
#ttyio.obj
#unreduce.obj
#unshrink.obj
#zipinfo.obj
#msdos.obj
#|
#    $(STRIP) unzip.exe

funzip.exe:     $(funzip_dependencies)
#    $(CC) -m$(FUMODEL) $(LDFLAGS) -efunzip.exe @&&|
#funzip.obj
#$(ASMOBJF)
#crc32_.obj
#crypt_.obj
#globals_.obj
#inflate_.obj
#ttyio_.obj
#|
    $(STRIP) funzip.exe

unzipsfx.exe:   $(unzipsfx_dependencies)
#    $(CC) -m$(UNMODEL) $(LDFLAGS) -eunzipsfx.exe @&&|
#unzipsfx.obj
#crc32.obj
#crctab.obj
#crypt.obj
#extract_.obj
#fileio.obj
#$(ASMOBJS)
#globals.obj
#inflate.obj
#match.obj
#process_.obj
#ttyio.obj
#msdos_.obj
#|
    $(STRIP) unzipsfx.exe

clean: .SYMBOLIC
    #rem Ignore any errors in the following...
    -rm *.obj
    -rm unzip.exe
    -rm funzip.exe
    -rm unzipsfx.exe

# Added by Jim Hall:
libclean: clean .SYMBOLIC
    -rm *.lib

# individual file dependencies

crc32.obj:      crc32.c $(UNZIP_H) zip.h
crctab.obj:     crctab.c $(UNZIP_H) zip.h
crypt.obj:      crypt.c $(UNZIP_H) zip.h crypt.h ttyio.h
envargs.obj:    envargs.c $(UNZIP_H)
explode.obj:    explode.c $(UNZIP_H)
extract.obj:    extract.c $(UNZIP_H) crypt.h
fileio.obj:     fileio.c $(UNZIP_H) crypt.h ttyio.h ebcdic.h
globals.obj:    globals.c $(UNZIP_H)
inflate.obj:    inflate.c inflate.h $(UNZIP_H)
list.obj:       list.c $(UNZIP_H)
match.obj:      match.c $(UNZIP_H)
process.obj:    process.c $(UNZIP_H)
ttyio.obj:      ttyio.c $(UNZIP_H) zip.h crypt.h ttyio.h
unreduce.obj:   unreduce.c $(UNZIP_H)
unshrink.obj:   unshrink.c $(UNZIP_H)
unzip.obj:      unzip.c $(UNZIP_H) crypt.h version.h consts.h
zipinfo.obj:    zipinfo.c $(UNZIP_H)

crc_i86.obj:    msdos/crc_i86.asm
    $(AS) $(ASFLAGS) msdos/crc_i86.asm -fo=./crc_i86.obj

crc_i86_.obj:   msdos/crc_i86.asm
    $(AS) $(ASFLAGS) msdos/crc_i86.asm -fo=./crc_i86.obj

msdos.obj:      msdos/msdos.c $(UNZIP_H)
   $(CC) -c $(UNFLAGS) msdos/msdos.c

funzip.obj:     funzip.c $(UNZIP_H) crypt.h ttyio.h tables.h
    $(CC) -c $(FUFLAGS) funzip.c

crc32_.obj:     crc32.c $(UNZIP_H) zip.h
    $(CC) -c $(FUFLAGS) -DFUNZIP -ocrc32_.obj crc32.c

crypt_.obj:     crypt.c $(UNZIP_H) zip.h crypt.h ttyio.h
    $(CC) -c $(FUFLAGS) -DFUNZIP -ocrypt_.obj crypt.c

globals_.obj:   globals.c $(UNZIP_H)
    $(CC) -c $(FUFLAGS) -DFUNZIP -oglobals_.obj globals.c

inflate_.obj:   inflate.c inflate.h $(UNZIP_H) crypt.h
    $(CC) -c $(FUFLAGS) -DFUNZIP -oinflate_.obj inflate.c

unzipsfx.obj:   unzip.c $(UNZIP_H) crypt.h version.h consts.h
    $(CC) -c $(UNFLAGS) -DSFX -ounzipsfx.obj unzip.c

extract_.obj:   extract.c $(UNZIP_H) crypt.h
    $(CC) -c $(UNFLAGS) -DSFX -oextract_.obj extract.c

process_.obj:   process.c $(UNZIP_H)
    $(CC) -c $(UNFLAGS) -DSFX -oprocess_.obj process.c

ttyio_.obj:     ttyio.c $(UNZIP_H) zip.h crypt.h ttyio.h
    $(CC) -c $(FUFLAGS) -DFUNZIP -ottyio_.obj ttyio.c

# msdos_.obj:     msdos/msdos.c $(UNZIP_H)
#   $(CC) -c $(UNFLAGS) -DSFX -omsdos_.obj msdos/msdos.c
