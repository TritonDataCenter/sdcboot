# WMAKE makefile for Windows 95 and Windows NT (Intel only)
# using Watcom C/C++ v10.5, by Paul Kienitz, last revised 20 Apr 96.
# Makes UnZip.exe, fUnZip.exe, and UnZipSFX.exe.
#
# Invoke from UnZip source dir with "WMAKE -F WIN32\MAKEFILE.WAT [targets]"
# To build with debug info use "WMAKE DEBUG=1 ..."
# To build with no assembly modules use "WMAKE NOASM=1 ..."
#
# Other options to be fed to the compiler can be specified in an environment
# variable called LOCAL_UNZIP.

variation = $(%LOCAL_UNZIP)

# Stifle annoying "Delete this file?" questions when errors occur:
.ERASE

.EXTENSIONS:
.EXTENSIONS: .exe .obj .obx .c .h .asm

# We maintain multiple sets of object files in different directories so that
# we can compile msdos, dos/4gw or pmode/w, and win32 versions of UnZip without
# their object files interacting.  The following var must be a directory name
# ending with a backslash.  All object file names must include this macro
# at the beginning, for example "$(O)foo.obj".

O = ob32w\  # comment here so backslash won't continue the line

# The assembly hot-spot code in crc_i386.asm is optional.  This section
# controls its usage.

!ifdef NOASM
crcob = $(O)crc32.obj
!else   # !NOASM
cvars = $+$(cvars)$- -DASM_CRC
# "$+$(foo)$-" means expand foo as it has been defined up to now; normally,
# this Make defers inner expansion until the outer macro is expanded.
crcob = $(O)crc_i386.obj
!endif

# Our object files.  OBJS is for UnZip, OBJX for UnZipSFX, OBJF for fUnZip:

OBJS1 = $(O)unzip.obj $(crcob) $(O)crctab.obj $(O)crypt.obj $(O)envargs.obj
OBJS2 = $(O)explode.obj $(O)extract.obj $(O)fileio.obj $(O)globals.obj
OBJS3 = $(O)inflate.obj $(O)list.obj $(O)match.obj $(O)process.obj
OBJS4 = $(O)ttyio.obj $(O)unreduce.obj $(O)unshrink.obj $(O)zipinfo.obj
OBJS  = $(OBJS1) $(OBJS2) $(OBJS3) $(OBJS4) $(O)win32.obj

OBJX1 = $(O)unzip.obx $(crcob) $(O)crctab.obx $(O)crypt.obx $(O)extract.obx
OBJX2 = $(O)fileio.obx $(O)globals.obx $(O)inflate.obx $(O)match.obx
OBJX3 = $(O)process.obx $(O)ttyio.obx
OBJX  = $(OBJX1) $(OBJX2) $(OBJX3) $(O)win32.obx

OBJF1 = $(O)funzip.obj $(crcob) $(O)cryptf.obj $(O)globalsf.obj
OBJF  = $(OBJF1) $(O)inflatef.obj $(O)ttyiof.obj

UNZIP_H = unzip.h unzpriv.h globals.h win32\w32cfg.h

# Now we have to pick out the proper compiler and options for it.

cc     = wcc386
link   = wlink
asm    = wasm
# Use Pentium timings, register args, static strings in code, max strictness:
cflags = -bt=NT -5r -zt -zq -wx -we
aflags = -bt=NT -mf -3 -zq
lflags = sys NT
cvars  = $+$(cvars)$- -DWIN32 $(variation)
avars  = $+$(avars)$- $(variation)

# Specify optimizations, or a nonoptimized debugging version:

!ifdef DEBUG
cdebug = -od -d2
cdebux = -od -d2
ldebug = d w all op symf
!else
cdebug = -s -oeilrt -zp4
cdebux = -s -oeilrs
# note: -ol+ does not help.  -oa helps slightly but might be dangerous.
ldebug = op el
!endif

# How to compile sources:
.c.obx:
	$(cc) $(cdebux) $(cflags) $(cvars) -DSFX $< -fo=$@

.c.obj:
	$(cc) $(cdebug) $(cflags) $(cvars) $< -fo=$@

# Here we go!  By default, make all targets:
all: UnZip.exe fUnZip.exe UnZipSFX.exe

# Convenient shorthand options for single targets:
u:   UnZip.exe     .SYMBOLIC
f:   fUnZip.exe    .SYMBOLIC
x:   UnZipSFX.exe  .SYMBOLIC

UnZip.exe:	$(OBJS)
	$(link) $(lflags) $(ldebug) name $@ file {$(OBJS)}

UnZipSFX.exe:	$(OBJX)
	$(link) $(lflags) $(ldebug) name $@ file {$(OBJX)}

fUnZip.exe:	$(OBJF)
	$(link) $(lflags) $(ldebug) name $@ file {$(OBJF)}

# Source dependencies:

#       generic (UnZip, fUnZip):

$(O)crc32.obj:    crc32.c $(UNZIP_H) zip.h
$(O)crctab.obj:   crctab.c $(UNZIP_H) zip.h
$(O)crypt.obj:    crypt.c $(UNZIP_H) zip.h crypt.h ttyio.h
$(O)envargs.obj:  envargs.c $(UNZIP_H)
$(O)explode.obj:  explode.c $(UNZIP_H)
$(O)extract.obj:  extract.c $(UNZIP_H) crypt.h
$(O)fileio.obj:   fileio.c $(UNZIP_H) crypt.h ttyio.h ebcdic.h
$(O)funzip.obj:   funzip.c $(UNZIP_H) crypt.h ttyio.h tables.h
$(O)globals.obj:  globals.c $(UNZIP_H)
$(O)inflate.obj:  inflate.c inflate.h $(UNZIP_H)
$(O)list.obj:     list.c $(UNZIP_H)
$(O)match.obj:    match.c $(UNZIP_H)
$(O)process.obj:  process.c $(UNZIP_H)
$(O)ttyio.obj:    ttyio.c $(UNZIP_H) zip.h crypt.h ttyio.h
$(O)unreduce.obj: unreduce.c $(UNZIP_H)
$(O)unshrink.obj: unshrink.c $(UNZIP_H)
$(O)unzip.obj:    unzip.c $(UNZIP_H) crypt.h version.h consts.h
$(O)zipinfo.obj:  zipinfo.c $(UNZIP_H)

#       UnZipSFX variants:

$(O)crc32.obx:    crc32.c $(UNZIP_H) zip.h
$(O)crctab.obx:   crctab.c $(UNZIP_H) zip.h
$(O)crypt.obx:    crypt.c $(UNZIP_H) zip.h crypt.h ttyio.h
$(O)extract.obx:  extract.c $(UNZIP_H) crypt.h
$(O)fileio.obx:   fileio.c $(UNZIP_H) crypt.h ttyio.h ebcdic.h
$(O)globals.obx:  globals.c $(UNZIP_H)
$(O)inflate.obx:  inflate.c inflate.h $(UNZIP_H)
$(O)match.obx:    match.c $(UNZIP_H)
$(O)process.obx:  process.c $(UNZIP_H)
$(O)ttyio.obx:    ttyio.c $(UNZIP_H) zip.h crypt.h ttyio.h
$(O)unzip.obx:    unzip.c $(UNZIP_H) crypt.h version.h consts.h

# Special case object files:

$(O)win32.obj:    win32\win32.c $(UNZIP_H)
	$(cc) $(cdebug) $(cflags) $(cvars) win32\win32.c -fo=$@

$(O)win32.obx:    win32\win32.c $(UNZIP_H)
	$(cc) $(cdebux) $(cflags) $(cvars) -DSFX win32\win32.c -fo=$@

$(O)crc_i386.obj: win32\crc_i386.asm
	$(asm) $(aflags) $(avars) win32\crc_i386.asm -fo=$@

# Variant object files for fUnZip:

$(O)cryptf.obj:   crypt.c $(UNZIP_H) zip.h crypt.h ttyio.h
	$(cc) $(cdebug) $(cflags) $(cvars) -DFUNZIP crypt.c -fo=$@

$(O)globalsf.obj: globals.c $(UNZIP_H)
	$(cc) $(cdebug) $(cflags) $(cvars) -DFUNZIP globals.c -fo=$@

$(O)inflatef.obj: inflate.c inflate.h $(UNZIP_H) crypt.h
	$(cc) $(cdebug) $(cflags) $(cvars) -DFUNZIP inflate.c -fo=$@

$(O)ttyiof.obj:   ttyio.c $(UNZIP_H) zip.h crypt.h ttyio.h
	$(cc) $(cdebug) $(cflags) $(cvars) -DFUNZIP ttyio.c -fo=$@

# Unwanted file removal:

clean:     .SYMBOLIC
	del $(O)*.ob?

cleaner:   clean  .SYMBOLIC
	del UnZip.exe
	del fUnZip.exe
	del UnZipSFX.exe

te.c inflate.h $(UNZIP_H)
$(O)list.obj:     list.c $(UNZIP_H)
$(O)match.obj:    match.c $(UNZIP_H)
$(O)process.obj:  process.c $(UNZIP_H)
$(O)timezone.obj: timezone.c $(UNZIP_H) zip.h timezone.h
$(O)ttyio.obj:    ttyio.c $(UNZIP_H) zip.h crypt.h ttyio.h
$(O)unreduce.obj: unreduce.c $(UNZIP_H)
$(O)unshrink.obj: unshrink.c $(UNZIP_H)
$(O)unzip.obj:    unzip.c $(UNZIP_H) crypt.h unzvers.h consts.h
$(O)zipinfo.obj:  zipinfo.c $(UNZIP_H)

#       UnZipSFX variants:

$(O)crc32.obx:    crc32.c $(UNZIP_H) zip.h
$(O)crctab.obx:   crctab.c $(UNZIP_H) zip.h
$(O)crypt.obx:    crypt.c $(UNZIP_H) zip.h crypt.h ttyio.h
$(O)extract.obx:  extract.c $(UNZIP_H) crypt.h
$(O)fileio.obx:   fileio.c $(UNZIP_H) crypt.h ttyio.h ebcdic.h
$(O)globals.obx:  globals.c $(UNZIP_H)
$(O)inflate.obx:  inflate.c inflate.h $(UNZIP_H)
$(O)match.obx:    match.c $(UNZIP_H)
$(O)process.obx:  process.c $(UNZIP_H)
$(O)timezone.obx: timezone.c $(UNZIP_H) zip.h timezone.h
$(O)ttyio.obx:    ttyio.c $(UNZIP_H) zip.h crypt.h ttyio.h
$(O)unzip.obx:    unzip.c $(UNZIP_H) crypt.h unzvers.h consts.h

# Special case object files:

$(O)win32.obj:    win32\win32.c $(UNZIP_H) win32\nt.h
	$(cc) $(cdebug) $(cflags) $(cvars) win32\win32.c -fo=$@

$(O)win32.obx:    win32\win32.c $(UNZIP_H) win32\nt.h
	$(cc) $(cdebux) $(cflags) $(cvars) -DSFX win32\win32.c -fo=$@

$(O)nt.obj:    win32\nt.c $(UNZIP_H) win32\nt.h
	$(cc) $(cdebug) $(cflags) $(cvars) win32\nt.c -fo=$@

$(O)nt.obx:    win32\nt.c $(UNZIP_H) win32\nt.h
	$(cc) $(cdebux) $(cflags) $(cvars) -DSFX win32\nt.c -fo=$@

$(O)crc_i386.obj: win32\crc_i386.asm
	$(asm) $(aflags) $(avars) win32\crc_i386.asm -fo=$@

# Variant object files for fUnZip:

$(O)cryptf.obj:   crypt.c $(UNZIP_H) zip.h crypt.h ttyio.h
	$(cc) $(CFLAGS_FU) crypt.c -fo=$@

$(O)globalsf.obj: globals.c $(UNZIP_H)
	$(cc) $(CFLAGS_FU) globals.c -fo=$@

$(O)inflatef.obj: inflate.c inflate.h $(UNZIP_H) crypt.h
	$(cc) $(CFLAGS_FU) inflate.c -fo=$@

$(O)ttyiof.obj:   ttyio.c $(UNZIP_H) zip.h crypt.h ttyio.h
	$(cc) $(CFLAGS_FU) ttyio.c -fo=$@

$(O)win32f.obj:   win32\win32.c $(UNZIP_H)
	$(cc) $(CFLAGS_FU) win32\win32.c -fo=$@

# DLL compilation section
$(O)api.obj:      api.c $(UNZIP_H) $(WINDLL_H) unzvers.h
	$(cc) $(CFLAGS_DL) api.c -fo=$@
$(O)crc32l.obj:   crc32.c $(UNZIP_H) zip.h
	$(cc) $(CFLAGS_DL) crc32.c -fo=$@
$(O)crctabl.obj:  crctab.c $(UNZIP_H) zip.h
	$(cc) $(CFLAGS_DL) crctab.c -fo=$@
$(O)cryptl.obj:   crypt.c $(UNZIP_H) zip.h crypt.h ttyio.h
	$(cc) $(CFLAGS_DL) crypt.c -fo=$@
$(O)explodel.obj: explode.c $(UNZIP_H)
	$(cc) $(CFLAGS_DL) explode.c -fo=$@
$(O)extractl.obj: extract.c $(UNZIP_H) $(WINDLL_H) crypt.h
	$(cc) $(CFLAGS_DL) extract.c -fo=$@
$(O)fileiol.obj:  fileio.c $(UNZIP_H) $(WINDLL_H) crypt.h ttyio.h ebcdic.h
	$(cc) $(CFLAGS_DL) fileio.c -fo=$@
$(O)globalsl.obj: globals.c $(UNZIP_H)
	$(cc) $(CFLAGS_DL) globals.c -fo=$@
$(O)inflatel.obj: inflate.c inflate.h $(UNZIP_H)
	$(cc) $(CFLAGS_DL) inflate.c -fo=$@
$(O)listl.obj:	  list.c $(UNZIP_H) $(WINDLL_H)
	$(cc) $(CFLAGS_DL) list.c -fo=$@
$(O)matchl.obj:	  match.c $(UNZIP_H)
	$(cc) $(CFLAGS_DL) match.c -fo=$@
$(O)processl.obj: process.c $(UNZIP_H) $(WINDLL_H)
	$(cc) $(CFLAGS_DL) process.c -fo=$@
$(O)timezonl.obj: timezone.c $(UNZIP_H) zip.h timezone.h
	$(cc) $(CFLAGS_DL) timezone.c -fo=$@
$(O)unreducl.obj: unreduce.c $(UNZIP_H)
	$(cc) $(CFLAGS_DL) unreduce.c -fo=$@
$(O)unshrnkl.obj: unshrink.c $(UNZIP_H)
	$(cc) $(CFLAGS_DL) unshrink.c -fo=$@
$(O)zipinfol.obj: zipinfo.c $(UNZIP_H)
	$(cc) $(CFLAGS_DL) zipinfo.c -fo=$@

$(O)win32l.obj:	  win32\win32.c $(UNZIP_H) win32\nt.h
	$(cc) $(CFLAGS_DL) -I. win32\win32.c -fo=$@

$(O)ntl.obj:      win32\nt.c $(UNZIP_H) win32\nt.h
	$(cc) $(CFLAGS_DL) -I. win32\nt.c -fo=$@

$(O)windll.obj:   windll\windll.c $(UNZIP_H) $(WINDLL_H)
$(O)windll.obj:   crypt.h unzvers.h consts.h
	$(cc) $(CFLAGS_DL) -I. windll\windll.c -fo=$@

windll.res:	windll\windll.rc unzvers.h
	$(rc) /l 0x409 /fo$@ /i windll /d WIN32 windll\windll.rc

# Windll command line example:

$(O)uzexampl.obj: windll/uzexampl.c windll/uzexampl.h
	$(cc) $(cdebug) $(cflags) $(cvars) windll\uzexampl.c -fo=$@

# Creation of subdirectory for intermediate files
$(OBDIR):
	-mkdir $@

# Unwanted file removal:

clean:     .SYMBOLIC
	del $(O)*.ob?

cleaner:   clean  .SYMBOLIC
	del UnZip.exe
	del fUnZip.exe
	del UnZipSFX.exe
        del UnZip32.dll
