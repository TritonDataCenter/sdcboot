# NMAKE Makefile for Windows NT/Windows 95
#   D. Feinleib 7/92 <t-davefe@microsoft.com>
#   H. Gessau 9/93 <henryg@kullmar.kullmar.se>
#   J. Lee 8/95 (johnnyl@microsoft.com)
#
# Last revised:  11 Feb 96
#
# Tested with VC++ 2.0 for NT for MIPS and Alpha, Visual C++ 2.2 for Intel CPUs
#

# Nmake macros for building Windows NT applications
# To build with debug info use 'nmake debug=1'
!IFNDEF debug
NODEBUG=1
!ENDIF

!IF "$(CPU)" == ""  &&  "$(PROCESSOR_ARCHITECTURE)" == "x86"
CPU = i386
!ENDIF
!IF "$(CPU)" == ""  &&  "$(PROCESSOR_ARCHITECTURE)" == "MIPS"
CPU = $(PROCESSOR_ARCHITECTURE)
!ENDIF
!IF "$(CPU)" == ""  &&  "$(PROCESSOR_ARCHITECTURE)" == "ALPHA"
CPU = $(PROCESSOR_ARCHITECTURE)
!ENDIF
!IF "$(CPU)" == ""  &&  "$(PROCESSOR_ARCHITECTURE)" == "PPC"
CPU = $(PROCESSOR_ARCHITECTURE)
!ENDIF
!IF "$(CPU)" == ""
CPU = i386
!ENDIF

!include <ntwin32.mak>

# object files
OBJS1 = unzip.obj crc32.obj crctab.obj crypt.obj envargs.obj explode.obj
OBJS2 = extract.obj fileio.obj globals.obj inflate.obj list.obj match.obj
OBJS3 = process.obj ttyio.obj unreduce.obj unshrink.obj zipinfo.obj win32.obj
OBJS  = $(OBJS1) $(OBJS2) $(OBJS3)
OBJX1 = unzipsfx.obj crc32.obj crctab.obj crypt.obj extract_.obj fileio.obj
OBJX2 = globalsx.obj inflate.obj match.obj process_.obj ttyio.obj win32_.obj
OBJX  = $(OBJX1) $(OBJX2)
OBJF  = funzip.obj crc32.obj cryptf.obj globalsf.obj inflatef.obj ttyiof.obj

UNZIP_H = unzip.h unzpriv.h globals.h win32/w32cfg.h

# cvars = $(cvars) -nologo -DMSDOS -DNO_ASM -J
#cvars = $(cvars) -nologo -DNO_ASM -J -DREENTRANT
cvars = $(cvars) -nologo -DNO_ASM -J

# Some optimization (if not debugging)
# [GRR 950816:  Steve suggests changing optimization flags to -O2, except for
#  UnZipSFX (-01)]
!IFDEF NODEBUG
!IF "$(CPU)" == "i386"
cdebug = -Ogityb2
!ELSE
cdebug = -Ogitb2
!ENDIF
!ENDIF

# How to compile sources
.c.obj:
	$(cc) $(cdebug) $(cflags) $(cvars) $<

# How to link
.obj.exe:
	$(link) $(ldebug) $(conflags) $(conlibs) $** -out:$@


# Default target is all the executables
unzips : unzip.exe funzip.exe unzipsfx.exe


unzip.exe:	$(OBJS)
unzipsfx.exe:	$(OBJX)
funzip.exe:	$(OBJF)

crc32.obj:	crc32.c $(UNZIP_H) zip.h
crctab.obj:	crctab.c $(UNZIP_H) zip.h
crypt.obj:	crypt.c $(UNZIP_H) zip.h crypt.h ttyio.h
envargs.obj:	envargs.c $(UNZIP_H)
explode.obj:	explode.c $(UNZIP_H)
extract.obj:	extract.c $(UNZIP_H) crypt.h
fileio.obj:	fileio.c $(UNZIP_H) crypt.h ttyio.h ebcdic.h
funzip.obj:	funzip.c $(UNZIP_H) crypt.h ttyio.h tables.h
globals.obj:	globals.c $(UNZIP_H)
inflate.obj:	inflate.c inflate.h $(UNZIP_H)
match.obj:	match.c $(UNZIP_H)
process.obj:	process.c $(UNZIP_H)
ttyio.obj:	ttyio.c $(UNZIP_H) zip.h crypt.h ttyio.h
unreduce.obj:	unreduce.c $(UNZIP_H)
unshrink.obj:	unshrink.c $(UNZIP_H)
unzip.obj:	unzip.c $(UNZIP_H) crypt.h version.h consts.h
zipinfo.obj:	zipinfo.c $(UNZIP_H)

win32.obj:	win32\win32.c $(UNZIP_H)
	$(cc) $(cdebug) $(cflags) $(cvars) -I. win32\win32.c

win32_.obj:	win32\win32.c $(UNZIP_H)			# unzipsfx only
	$(cc) $(cdebug) $(cflags) $(cvars) -I. -DSFX win32\win32.c -Fo$@

cryptf.obj:	crypt.c $(UNZIP_H) zip.h crypt.h ttyio.h	# funzip only
	$(cc) $(cdebug) $(cflags) $(cvars) -DFUNZIP crypt.c -Fo$@

extract_.obj:	extract.c $(UNZIP_H) crypt.h			# unzipsfx only
	$(cc) $(cdebug) $(cflags) $(cvars) -DSFX extract.c -Fo$@

globalsx.obj:	globals.c $(UNZIP_H)				# unzipsfx only
	$(cc) $(cdebug) $(cflags) $(cvars) -DSFX globals.c -Fo$@

globalsf.obj:	globals.c $(UNZIP_H)				# funzip only
	$(cc) $(cdebug) $(cflags) $(cvars) -DFUNZIP globals.c -Fo$@

inflatef.obj:	inflate.c inflate.h $(UNZIP_H) crypt.h		# funzip only
	$(cc) $(cdebug) $(cflags) $(cvars) -DFUNZIP inflate.c -Fo$@

process_.obj:	process.c $(UNZIP_H)				# unzipsfx only
	$(cc) $(cdebug) $(cflags) $(cvars) -DSFX process.c -Fo$@

ttyiof.obj:	ttyio.c $(UNZIP_H) zip.h crypt.h ttyio.h	# funzip only
	$(cc) $(cdebug) $(cflags) $(cvars) -DFUNZIP ttyio.c -Fo$@

unzipsfx.obj:	unzip.c $(UNZIP_H) crypt.h version.h consts.h	# unzipsfx only
	$(cc) $(cdebug) $(cflags) $(cvars) -DSFX unzip.c -Fo$@

clean:
	del /q $(OBJS) unzip.exe $(OBJF) funzip.exe $(OBJX) unzipsfx.exe

