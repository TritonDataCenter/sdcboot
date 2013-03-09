#==============================================================================
# Makefile for UnZip, UnZipSFX and fUnZip:  Unix and MS-DOS ("real" makes only)
# Version:  5.20                                               11 February 1996
#==============================================================================


# INSTRUCTIONS (such as they are):
#
# "make vax"	-- makes UnZip on a generic Unix VAX in the current directory
# "make list"	-- lists all supported systems (targets)
# "make help"	-- provides pointers on what targets to try if problems occur
# "make wombat" -- chokes and dies if you haven't added the specifics for your
#		    Wombat 68000 (or whatever) to the systems list
#
# CF are flags for the C compiler.  LF are flags for the loader.  LF2 are more
# flags for the loader, if they need to be at the end of the line instead of at
# the beginning (for example, some libraries).  FL and FL2 are the corre-
# sponding flags for fUnZip.  LOCAL_UNZIP is an environment variable that can
# be used to add default C flags to your compile without editing the Makefile
# (e.g., -DDEBUG_STRUC, or -FPi87 on PCs using Microsoft C).
#
# Some versions of make do not define the macro "$(MAKE)"; this is rare, but
# if things don't work, try using "make" instead of "$(MAKE)" in your system's
# makerule.  Or try adding the following line to your .login file:
#	setenv MAKE "make"
# (That never works--makes which are too stupid to define MAKE are also too
# stupid to look in the environment--but try it anyway for kicks. :-) )
#
# Memcpy and memset are provided for those systems that don't have them; they
# are in fileio.c and will be used if -DZMEM is included in CF.  These days
# almost all systems have them.
#
# Be sure to test your new UnZip (and UnZipSFX and fUnZip); successful compila-
# tion does not always imply a working program.


#####################
# MACRO DEFINITIONS #
#####################

# Defaults most systems use (use LOCAL_UNZIP in environment to add flags,
# such as -DDOSWILD).

# UnZip flags
CC = gcc#	try using "gcc" target rather than changing this (if you do,
LD = $(CC)#	you MUST change LD, too--else "unresolved symbol:  ___main")
LOC = $(LOCAL_UNZIP)
CF = $(CFLAGS) $(LOC)
LF = -o unzip$E
LF2 = -s

# UnZipSFX flags
SL = -o unzipsfx$E
SL2 = $(LF2)

# fUnZip flags
FL = -o funzip$E
FL2 = $(LF2)

# general-purpose stuff
CP = ln -s
LN = ln
RM = rm -f
CHMOD = chmod
STRIP = strip
E = .ttp
O = .o
M = atari
SHELL = /bin/sh

# object files
OBJS1 = unzip$O crc32$O crctab$O crypt$O envargs$O explode$O
OBJS2 = extract$O fileio$O globals$O inflate$O list$O match$O
OBJS3 = process$O ttyio$O unreduce$O unshrink$O zipinfo$O
OBJS = $(OBJS1) $(OBJS2) $(OBJS3) $M$O
LOBJS = $(OBJS)
OBJSDLL = $(OBJS) api$O
OBJX = unzipsfx$O crc32$O crctab$O crypt$O extract_$O fileio$O globals$O \
	inflate$O match$O process_$O ttyio$O $M_$O
LOBJX = $(OBJX)
OBJF = funzip$O crc32$O crypt_$O globals_$O inflate_$O ttyio_$O
#OBJS_OS2 = $(OBJS1:.o=.obj) $(OBJS2:.o=.obj) os2.obj
#OBJF_OS2 = $(OBJF:.o=.obj)

UNZIP_H = unzip.h unzpriv.h globals.h

# installation
INSTALL = cp#	probably can change this to 'install' if you have it
# on some systems, manext=l and MANDIR=/usr/man/man$(manext) may be appropriate
manext = 1
prefix = /usr/local
BINDIR = $(prefix)/bin#			where to install executables
MANDIR = $(prefix)/man/man$(manext)#	where to install man pages
INSTALLEDBIN = $(BINDIR)/funzip$E $(BINDIR)/zipinfo$E $(BINDIR)/unzipsfx$E \
	$(BINDIR)/unzip$E
INSTALLEDMAN = $(MANDIR)/unzip.$(manext) $(MANDIR)/funzip.$(manext) \
	$(MANDIR)/unzipsfx.$(manext) $(MANDIR)/zipinfo.$(manext)
#
UNZIPS = unzip$E funzip$E unzipsfx$E
# this is a little ugly...well, no, it's a lot ugly:
MANS = unix/unzip.1 unix/unzipsfx.1 unix/zipinfo.1 unix/funzip.1
DOCS = unzip.doc unzipsfx.doc zipinfo.doc funzip.doc

# list of supported systems/targets in this version
SYSTEMS1 = 386i 3Bx 7300 7300_gcc aix aix_rt amdahl amdahl_eft apollo
SYSTEMS2 = aviion bcc_dos bsd bsd386 bull coherent convex cray cray_opt
SYSTEMS3 = cyber_sgi dec djgpp djgpp1 dnix encore eta gcc gcc_dos generic
SYSTEMS4 = generic2 generic3 gould hk68 hp hpux linux linux_dos linux_lib
SYSTEMS5 = linux_zlib minix mips msc_dos next next10 next2x next3x nextfat
SYSTEMS6 = osf1 pixel ptx pyramid realix regulus rs6000 sco sco_dos sco_sl
SYSTEMS7 = sco_x286 sequent sgi solaris stardent stellar sun sysv sysv_gcc
SYSTEMS8 = sysv6300 tahoe ti_sysv ultrix vax v7 wombat xenix xos


####################
# DEFAULT HANDLING #
####################

# By default, print help on which makefile targets to try.  (The SYSTEM
# variable is no longer supported; use "make <target>" instead.)

help:
	@echo ""
	@echo\
 "  If you're not sure about the characteristics of your system, try typing"
	@echo\
 '  "make generic".  If the compiler barfs and says something unpleasant about'
	@echo\
 '  "timezone redefined," try typing "make clean" followed by "make generic2".'
	@echo\
 '  If, on the other hand, it complains about an undefined symbol _ftime, try'
	@echo\
 '  typing "make clean" followed by "make generic3".  One of these actions'
	@echo\
 '  should produce a working copy of unzip on most Unix systems.  If you know'
	@echo\
 '  a bit more about the machine on which you work, you might try "make list"'
	@echo\
 '  for a list of the specific systems supported herein.  (Many of them do'
	@echo\
 "  exactly the same thing, so don't agonize too much over which to pick if"
	@echo\
 '  two or more sound equally likely.)  Also check out the INSTALL file for'
	@echo\
 '  notes on compiling various targets.  As a last resort, feel free to read'
	@echo\
 '  the numerous comments within the Makefile itself.  Note that to compile'
	@echo\
 '  the decryption version of UnZip, you must obtain the full versions of'
	@echo\
 '  crypt.c and crypt.h (see the "Where" file for ftp and mail-server sites).'
	@echo\
 '  Have a mostly pretty good day.'
	@echo ""

list:
	@echo ""
	@echo\
 'Type "make <system>", where <system> is one of the following:'
	@echo ""
	@echo  "	$(SYSTEMS1)"
	@echo ""
	@echo  "	$(SYSTEMS2)"
	@echo  "	$(SYSTEMS3)"
	@echo  "	$(SYSTEMS4)"
	@echo  "	$(SYSTEMS5)"
	@echo  "	$(SYSTEMS6)"
	@echo  "	$(SYSTEMS7)"
	@echo  "	$(SYSTEMS8)"
#	@echo ""
#	@echo\
# 'Targets for related utilities (ZipInfo and fUnZip) include:'
#	@echo ""
#	@echo  "	$(SYS_UTIL1)"
#	@echo  "	$(SYS_UTIL2)"
	@echo ""
	@echo\
 'For further (very useful) information, please read the comments in Makefile.'
	@echo ""

generic_msg:
	@echo ""
	@echo\
 '  Attempting "make generic" now.  If this fails for some reason, type'
	@echo\
 '  "make help" and/or "make list" for suggestions.'
	@echo ""


###############################################
# BASIC COMPILE INSTRUCTIONS AND DEPENDENCIES #
###############################################

# this is for GNU make; comment out and notify zip-bugs if it causes errors
.SUFFIXES:	.c .o .obj

# yes, we should be able to use the $O macro to combine these two, but it
# fails on some brain-damaged makes (e.g., AIX's)...no big deal
.c.o:
	$(CC) -c $(CF) $*.c

.c.obj:
	$(CC) -c $(CF) $*.c

# this doesn't work...directories are always a pain with implicit rules
#.1.doc:		unix/$<
#	nroff -Tman -man $< | col -b | uniq | \
#	 sed 's/Sun Release ..../Info-ZIP        /' > $@


# these rules are specific to Suns and are really intended only for the
# authors' use in creating non-Unix documentation files (which are pro-
# vided with both source and binary distributions).  We should probably
# add a ".1.man" rule for more generic systems...

unzip.doc:	unix/unzip.1
	nroff -Tman -man unix/unzip.1 | col -b | uniq | \
	 sed 's/Sun Release ..../Info-ZIP        /' > $@

unzipsfx.doc:	unix/unzipsfx.1
	nroff -Tman -man unix/unzipsfx.1 | col -b | uniq | \
	 sed 's/Sun Release ..../Info-ZIP        /' > $@

zipinfo.doc:	unix/zipinfo.1
	nroff -Tman -man unix/zipinfo.1 | col -b | uniq | \
	 sed 's/Sun Release ..../Info-ZIP        /' > $@

funzip.doc:	unix/funzip.1
	nroff -Tman -man unix/funzip.1 | col -b | uniq | \
	 sed 's/Sun Release ..../Info-ZIP        /' > $@


all:		generic_msg generic
unzips:		$(UNZIPS)
objs:		$(OBJS)
objsdll:	$(OBJSDLL)
docs:		$(DOCS)
unzipsman:	unzips docs
unzipsdocs:	unzips docs


# EDIT HERE FOR PARALLEL MAKES on Sequent (and others?)--screws up MS-DOS
# make utilities if default:  change "unzip$E:" to "unzip$E:&"

unzip$E:	$(OBJS)			# add `&' for parallel makes
	$(LD) $(LF) $(LOBJS) $(LF2)

unzipsfx$E:	$(OBJX)			# add `&' for parallel makes
	$(LD) $(SL) $(LOBJX) $(SL2)

funzip$E:	$(OBJF)			# add `&' for parallel makes
	$(LD) $(FL) $(OBJF) $(FL2)

zipinfo$E:	unzip$E			# `&' is pointless here...
	@echo\
 '  This is a Unix-specific target.  ZipInfo is not enabled in some MS-DOS'
	@echo\
 '  versions of UnZip; if it is in yours, copy unzip.exe to zipinfo.exe'
	@echo\
 '  or else invoke as "unzip -Z" (in a batch file, for example).'
	$(LN) unzip$E zipinfo$E


crc32$O:	crc32.c $(UNZIP_H) zip.h
crctab$O:	crctab.c $(UNZIP_H) zip.h
crypt$O:	crypt.c $(UNZIP_H) zip.h crypt.h ttyio.h
envargs$O:	envargs.c $(UNZIP_H)
explode$O:	explode.c $(UNZIP_H)
extract$O:	extract.c $(UNZIP_H) crypt.h
fileio$O:	fileio.c $(UNZIP_H) crypt.h ttyio.h ebcdic.h
funzip$O:	funzip.c $(UNZIP_H) crypt.h ttyio.h tables.h
globals$O:	globals.c $(UNZIP_H)
inflate$O:	inflate.c inflate.h $(UNZIP_H)
list$O:		list.c $(UNZIP_H)
match$O:	match.c $(UNZIP_H)
process$O:	process.c $(UNZIP_H)
ttyio$O:	ttyio.c $(UNZIP_H) zip.h crypt.h ttyio.h
unreduce$O:	unreduce.c $(UNZIP_H)
unshrink$O:	unshrink.c $(UNZIP_H)
unzip$O:	unzip.c $(UNZIP_H) crypt.h version.h consts.h
zipinfo$O:	zipinfo.c $(UNZIP_H)

crypt_$O:	crypt.c $(UNZIP_H) zip.h crypt.h ttyio.h	# funzip only
	$(CP) crypt.c crypt_.c
	$(CC) -c $(CF) -DFUNZIP crypt_.c
	$(RM) crypt_.c

extract_$O:	extract.c $(UNZIP_H) crypt.h			# unzipsfx only
	$(CP) extract.c extract_.c
	$(CC) -c $(CF) -DSFX extract_.c
	$(RM) extract_.c

globals_$O:	globals.c $(UNZIP_H)				# funzip only
	$(CP) globals.c globals_.c
	$(CC) -c $(CF) -DFUNZIP globals_.c
	$(RM) globals_.c

inflate_$O:	inflate.c inflate.h $(UNZIP_H) crypt.h		# funzip only
	$(CP) inflate.c inflate_.c
	$(CC) -c $(CF) -DFUNZIP inflate_.c
	$(RM) inflate_.c

ttyio_$O:	ttyio.c $(UNZIP_H) zip.h crypt.h ttyio.h	# funzip only
	$(CP) ttyio.c ttyio_.c
	$(CC) -c $(CF) -DFUNZIP ttyio_.c
	$(RM) ttyio_.c

msdos$O:	msdos/msdos.c $(UNZIP_H) version.h		# DOS only
	$(CC) -c $(CF) msdos/msdos.c

# version() not used by unzipsfx, so no version.h dependency
msdos_$O:	msdos/msdos.c $(UNZIP_H)			# DOS unzipsfx
	$(CP) msdos\msdos.c msdos_.c > nul
	$(CC) -c $(CF) -DSFX msdos_.c
	$(RM) msdos_.c

#os2$O:		os2/os2.c $(UNZIP_H)				# OS/2 only
#	$(CC) -c $(CF) os2/os2.c


process_$O:	process.c $(UNZIP_H)				# unzipsfx only
	$(CP) process.c process_.c
	$(CC) -c $(CF) -DSFX process_.c
	$(RM) process_.c

unix$O:		unix/unix.c $(UNZIP_H) version.h		# Unix only
	$(CC) -c $(CF) unix/unix.c

# version() not used by unzipsfx, so no version.h dependency
unix_$O:	unix/unix.c $(UNZIP_H)				# Unix unzipsfx
	$(CP) unix/unix.c unix_.c
	$(CC) -c $(CF) -DSFX unix_.c
	$(RM) unix_.c

atari_$O:	atari.c $(UNZIP_H)
	$(CP) atari.c atari_.c
	$(CC) -c $(CF) -DSFX atari_.c
	$(RM) atari_.c

unzipsfx$O:	unzip.c $(UNZIP_H) crypt.h version.h consts.h	# unzipsfx only
	$(CP) unzip.c unzipsfx.c
	$(CC) -c $(CF) -DSFX unzipsfx.c
	$(RM) unzipsfx.c


unix_make:
#	@echo\
# '(Ignore any errors from `make'"' due to the following command; it's harmless.)"
	-@2>&1 $(LN) unix/Makefile . > /dev/null || echo > /dev/null

# this really only works for Unix targets, unless specify E and O on cmd line
clean:
	@echo ""
	@echo '         This is a Unix-specific target.  (Just so you know.)'
	@echo ""
	rm -f $(OBJS) api$O apihelp$O unzipstb$O $(OBJF) $(OBJX) $(UNZIPS)

install:	$(UNZIPS) $(MANS)
	$(INSTALL) $(UNZIPS) $(BINDIR)
	$(RM) $(BINDIR)/zipinfo$E
	$(LN) $(BINDIR)/unzip$E $(BINDIR)/zipinfo$E
	$(INSTALL) unix/unzip.1 $(MANDIR)/unzip.$(manext)
	$(INSTALL) unix/unzipsfx.1 $(MANDIR)/unzipsfx.$(manext)
	$(INSTALL) unix/zipinfo.1 $(MANDIR)/zipinfo.$(manext)
	$(INSTALL) unix/funzip.1 $(MANDIR)/funzip.$(manext)
	$(CHMOD) 755  $(INSTALLEDBIN)
	$(CHMOD) 644  $(INSTALLEDMAN)

# alternatively, could use zip method:  -cd $(BINDIR); rm -f $(UNZIPS)  [etc.]
uninstall:
	rm -f $(INSTALLEDBIN) $(INSTALLEDMAN)


TESTZIP = testmake.zip	# the test zipfile

# test some basic features of the build
test:		check

check:	unzips
	@echo '  This is a Unix-specific target.  (Just so you know.)'
	if test ! -f $(TESTZIP); then \
		echo "  error:  can't find test file $(TESTZIP)"; exit 1; fi
#
	echo "  testing extraction"
	./unzip -b $(TESTZIP) testmake.zipinfo
	if test $? ; then \
	    echo "  error:  file extraction from $(TESTZIP) failed"; exit 1; fi
#
	echo '  testing zipinfo (unzip -Z)'
	./unzip -Z $(TESTZIP) > testmake.unzip-Z
	if diff testmake.unzip-Z testmake.zipinfo; then ;; else \
	    echo '  error:  zipinfo output doesn't match stored version'; fi
	$(RM) testmake.unzip-Z testmake.zipinfo
#
	echo '  testing unzip -d exdir option'
	./unzip -b $(TESTZIP) -d testun
	cat testun/notes
#
	echo '  testing unzip -o and funzip (ignore funzip warning)'
	./unzip -boq $(TESTZIP) notes -d testun
	./funzip < $(TESTZIP) > testun/notes2
	if diff testun/notes testun/notes2; then ;; else \
	    echo 'error:  funzip output disagrees with unzip'; fi
#
	echo '  testing unzipsfx (self-extractor)'
	cat unzipsfx $(TESTZIP) > testsfx
	$(CHMOD) 0700 testsfx
	./testsfx -b notes
	if diff notes testun/notes; then ;; else \
	    echo '  error:  unzipsfx file disagrees with unzip'; fi
	$(RM) testsfx notes testun/notes testun/notes2
	rmdir testun
#
	echo '  testing complete.'


################################
# INDIVIDUAL MACHINE MAKERULES #
################################

#----------------------------------------------------------------------------
#  Generic targets (can't assume make utility groks "$(MAKE)")
#----------------------------------------------------------------------------

generic:	unzips	   # first try if unknown

generic2:	unix_make  # second try if unknown:  hope make is called "make"
	make -f unix/Makefile unzips CF="$(CF) -DBSD"

generic3:	unix_make  # third try if unknown:  hope make is called "make"
	make -f unix/Makefile unzips CF="$(CF) -DSYSV"

#----------------------------------------------------------------------------
#  "Normal" group (BSD vs. SysV may be set in unzip.h via predefined macros):
#----------------------------------------------------------------------------

386i:		unzips	# sun386i, SunOS 4.0.2
3Bx:		unzips	# AT&T 3B2/1000-80; should work on any WE32XXX machine
#aix_rt:		unzips	# IBM RT 6150 under AIX 2.2.1
apollo:		unzips	# Apollo Domain/OS machines
bull:		unzips	# Bull DPX/2, BOS 2.00.45 (doesn't require -Xk switch)
convex:		unzips	# Convex C-120 and C-210 (-O is enough; -ext is default)
cray:		unzips	# Cray-2 and Y-MP, using default (possibly old) compiler
dec:		unzips	# DEC 5820 (MIPS RISC), test version of Ultrix v4.0
encore:		unzips	# Multimax
eta:		unzips	# ETA-10P*, hybrid SysV with BSD 4.3 enhancements
gould:		unzips	# Gould PN9000 running UTX/32 2.1Bu01
hp:		unzips	# HP 9000 series (68020), 4.3BSD or HP-UX A.B3.10 Ver D
hpux:		unzips	# (to match zip's makefile entry)
mips:		unzips	# MIPS M120-5(?), SysV.3 [error in sys/param.h file?]
next10:		unzips	# NeXT (generic; use next2x or next3x for better opt.)
osf1:		unzips	# DECstation, including Alpha-based; DEC OSF/1 v1.x
pyr_:		unzips	# [failsafe target for pyramid target below]
pyr_ucb:	unzips	# Pyramids running BSD universe by default (see below)
realix:		unzips	# Modcomp Real/IX (SysV.3); note "gcc" = GLS C, not GNU
sco:		unzips	# Xenix/386 (tested on 2.3.1); SCO Unix 3.2.0.
sgi:		unzips	# Silicon Graphics; Irix 3.3.2, 4.0.x, 5.2, etc.
stellar:	unzips	# gs-2000
sun:		unzips	# old target; no good with solaris...use "sunos" now
sunos:		unzips	# Sun 3, 4; SunOS 4.x (SOME SYSTEMS ARE SYSTEM V!)
tahoe:		unzips	# tahoe (CCI Power6/32), 4.3BSD
ultrix:		unzips	# VAXen, DEC 58x0 (MIPS guts), DECstation 2100; v4.x
vax:		unzips	# general-purpose VAX target (not counting VMS)

#----------------------------------------------------------------------------
#  BSD group (for timezone structs [struct timeb]):
#----------------------------------------------------------------------------

bsd:		_bsd	# generic BSD (BSD 4.2 & Ultrix handled in unzip.h)
bsd386:		_bsd	# BSDI BSD/386 version 1.0

_bsd:		unix_make
	$(MAKE) unzips CF="$(CF) -DBSD"

#----------------------------------------------------------------------------
#  SysV group (for extern long timezone and ioctl.h instead of sgtty.h):
#----------------------------------------------------------------------------

aix_rt:		_sysv	# IBM RT 6150 under AIX 2.2.1
amdahl:		_sysv	# Amdahl (IBM) mainframe, UTS (SysV) 1.2.4 and 2.0.1
aviion:		_sysv	# Data General AViiONs, DG/UX 4.3x
pyr_att:	_sysv	# Pyramids running AT&T (SysV) universe by default
solaris:	_sysv	# Sun SPARC & x86, Solaris 2.x
stardent:	_sysv	# Stardent ...
sysv:		_sysv	# generic System V Unix (Xenix handled in unzip.h)
xos:		_sysv	# Olivetti LSX-3005..3045, X/OS 2.3 and 2.4

_sysv:		unix_make
	$(MAKE) unzips CF="$(CF) -DSYSV"

#----------------------------------------------------------------------------
#  Version 7 group (old/obsolescent):
#----------------------------------------------------------------------------

pixel:		_v7	# Pixel 80, 100 (68000-based, V7/mostly BSD4.1 compat.)
v7:		_v7	# generic Unix Version 7 box (prob. only Pixel...)

_v7:
	make -f unix/Makefile unzips \
	 CF="$(CF) -DV7 -DNO_PARAM_H -DSHORT_NAMES -DBSD -DZMEM"

#----------------------------------------------------------------------------
#  "Unique" group (require non-standard options):
#----------------------------------------------------------------------------

# AT&T 7300 (M68000/SysV.3) (add -DSYSV? -DNO_LIMITS?)
7300:		unix_make
	$(MAKE) unzips CF="$(CF) -DNO_DIR -DNO_MKDIR -DNO_STRNICMP"

7300_gcc:	unix_make
	$(MAKE) unzips CC=gcc LD=gcc LF2="" \
	 CF="-O2 -I. -DNO_DIR -DNO_MKDIR -DNO_STDLIB_H -DNO_STRNICMP $(LOC)"
	$(STRIP) $(UNZIPS)

# IBM AIX 3.x on an RS/6000:  see rs6000 target below
aix:		rs6000

# Amdahl UTS 2.1.4 with "extended file types" filesystem (aarrrggghhhh...)
amdahl_eft:	unix_make
	$(MAKE) unzips CF="$(CF) -eft -DSYSV"

# MS-DOS:  Borland C++ 3.0 (can change UnZip memory model to small for more
# speed but no ZipInfo support [-ml -> -ms in _bcc_dos], but may run out of
# memory when inflating--should not be true anymore in 5.11)
bcc_dos:	_bcc_dos fu_bcc sfx_bcc

_bcc_dos:	bcc_rsp		#  v-- change to -ml for large model
	$(MAKE) unzip.exe CF="-w -ms -O2 -I. $(LOC)" CC=bcc LD=bcc E=.exe\
	 O=.obj M=msdos LOBJS="" LF="@bcc_rsp" LF2=""
	del bcc_rsp

bcc_rsp:
	echo $(OBJS1:.o=.obj) > bcc_rsp
	echo msdos.obj $(OBJS2:.o=.obj) >> bcc_rsp

fu_bcc:
	$(MAKE) funzip.exe CF="-w -ms -O2 -I. $(LOC)" CC=bcc LD=bcc E=.exe\
	 O=.obj FL="" FL2="" CP=copy RM=del

sfx_bcc:
	$(MAKE) unzipsfx.exe CF="-w -ms -O2 -I. $(LOC)" CC=bcc LD=bcc E=.exe\
	 O=.obj SL="" SL2="" CP=copy RM=del

# Coherent 3.x/4.x, Mark Williams C.  ``For Coherent's CC, it needs either
# -T0 or -T150000 (or bigger) added to the CFLAGS, otherwise the compiler
# runs out of memory and dies in zipinfo.c.'' [Fred "Fredex" Smith, 940719]
coherent:	unix_make
	$(MAKE) unzips CF="$(CF) -T0"

# Cray-2, Y-MP or C90, running Unicos 5.x to 8.x (SysV + BSD enhancements)
# and Standard (ANSI) C compiler 3.0 or later.
cray_opt:	unix_make
	$(MAKE) unzips CF="$(CF) -h scalar3 -h vector3"

# The unzip41 build on a Cyber 910/SGI running Irix v3.3.3 was successful
# with the following change to Makefile:
cyber_sgi:	unix_make
	$(MAKE) unzips CF="$(CF) -I/usr/include/bsd"\
	 LF="-lbsd $(LF)" SL="-lbsd $(SL)"

# 680x0, DIAB dnix 5.2/5.3 (a Swedish System V clone)
#
# Options for the dnix cc:
#  -X7 = cc is strict ANSI C
#  -X9 = warnings if a function is used without a declaration
#
dnix:		unix_make
	$(MAKE) unzips CF="$(CF) -X7 -X9 -DDNIX"

# Generic BSDish Unix gcc.  ``The -O2 only works with the latest version of
# gcc; you may have to use -O only for earlier versions.  I have no idea why
# -s causes this bug in gcc.''  [Bug:  "nm: unzip: no name list", "collect:
# /usr/bin/nm returned 1 exit status".]  If you don't have strip, don't
# worry about it (it just makes the executable smaller and can be replaced
# with "echo" instead).
#
gcc:		unix_make
	$(MAKE) unzips CC=gcc LD=gcc CF="-O2 -I. $(LOC)" LF2=""
	$(STRIP) $(UNZIPS)

# MS-DOS with D.J. Delorie's djgpp 1.12.  Note that earlier versions of go32
# (< 1.11) don't support DOS function 0x38 (necessary for "unzip -v foobar").
# Note also that this set of targets has work-arounds for three bugs in the
# older version (3.69) of GNU make formerly distributed with djgpp:  (1) it
# sets the MAKE variable incorrectly for spawning with COMMAND.COM (forward
# slashes instead of backslashes--fixed in 3.71 by not spawning COMMAND.COM),
# so the usual $(MAKE) has been replaced by "make"; (2) it doesn't handle
# makefile continuation lines correctly, most likely because of the MS-DOS
# CR-LF line-endings (seems to be fixed in 3.71?), so long lines are used;
# and (3) it doesn't handle quotes as in CF="-O2 -Wall etc." correctly, so
# these macros have been changed to "CF=-O2 -Wall etc." where necessary.
# The newer GNU make (version 3.71) does not handle quotes correctly, but
# prepending "command /c" to each make line is reported by E-Yen Tan
# <e-yen.tan@brasenose.oxford.ac.uk> to be a successful work-around.  The
# emx version of GNU make 3.71 which is maintained by Kai Uwe Rommel has
# other fatal bugs involving the shell (necessary for recursive targets
# like this one).  GRR 940430, 940723, 940814, 951230
#
djgpp1:		_gcc_dos1 fu_gcc1 sfx_gcc1	# djgpp v1.x with GO32 extender
gcc_dos1:	_gcc_dos1 fu_gcc1 sfx_gcc1	# djgpp v1.x with GO32 extender

_gcc_dos1: gcc_rsp
# this line works with GNU make 3.69:
#	make unzip CC=gcc LD=gcc M=msdos "CF=-O2 -Wall -I. $(LOC)" LOBJS="" "LF=-o unzip @gcc_rsp" LF2=""
#
# this line should work with GNU make 3.71:
	command /c make unzip CC=gcc LD=gcc M=msdos "CF=-O2 -Wall -I. $(LOC)" LOBJS="" "LF=-o unzip @gcc_rsp" LF2=""
#
# these lines don't work at all, to our knowledge:
#	$(MAKE) unzip CC=gcc LD=gcc M=msdos "CF=-O2 -Wall -I. $(LOC)"\
#	 LOBJS="" LF="-o unzip @gcc_rsp" LF2=""
#
	$(STRIP) unzip
	coff2exe unzip
#	use this instead if you want to create a stand-alone executable (bigger;
#	may require full path to go32.exe):
#	coff2exe -s go32.exe unzip
	stubedit unzip.exe globbing=no
	del gcc_rsp
	del unzip
#
	coff2exe -g zipinfo
	stubedit zipinfo.exe runfile=unzip.exe globbing=no
	del zipinfo

fu_gcc1:
# this line works with GNU make 3.69:
#	make funzip CC=gcc LD=gcc "CF=-Wall -O2 -I. $(LOC)" FL2="" CP=copy RM=del
#
# this line should work with GNU make 3.71:
	command /c make funzip CC=gcc LD=gcc "CF=-Wall -O2 -I. $(LOC)" FL2="" CP=copy RM=del
#
# these lines don't work at all, to our knowledge:
#	$(MAKE) funzip CC=gcc LD=gcc "CF=-Wall -O2 -I. $(LOC)" FL2=""\
#	 CP=copy RM=del
#
	$(STRIP) funzip
	coff2exe funzip
#	use this instead if you want to create a stand-alone executable (bigger;
#	may require full path to go32.exe):
#	coff2exe -s go32.exe funzip
	del funzip

sfx_gcc1:
# this line works with GNU make 3.69:
#	make unzipsfx CC=gcc LD=gcc M=msdos "CF=-Wall -O2 -I. $(LOC)" SL2="" CP=copy RM=del
#
# this line should work with GNU make 3.71:
	command /c make unzipsfx CC=gcc LD=gcc M=msdos "CF=-Wall -O2 -I. $(LOC)" SL2="" CP=copy RM=del
#
# these lines don't work at all, to our knowledge:
#	$(MAKE) unzipsfx CC=gcc LD=gcc M=msdos "CF=-Wall -O2 -I. $(LOC)"\
#	 SL2="" CP=copy RM=del
#
	$(STRIP) unzipsfx
	coff2exe unzipsfx
#	use this instead if you want to create a stand-alone executable (bigger;
#	may require full path to go32.exe):
#	coff2exe -s go32.exe unzipsfx
	stubedit unzipsfx.exe globbing=no
	del unzipsfx

gcc_rsp:
	echo $(OBJS1) > gcc_rsp
	echo $(OBJS2) msdos.o >> gcc_rsp

# MS-DOS with D.J. Delorie's djgpp 2.0.  See above for comments about make
# utilities (may or may not still be relevant).  All executables are now
# stand-alone, but can provide Charles W. Sandmann's (alternate?) DPMI server
# if needed via "stubedit unzip.exe dpmi=cwsdpmi.exe" (for example).  950816

djgpp:		_gcc_dos fu_gcc sfx_gcc		# djgpp v2.x
djgpp2:		_gcc_dos fu_gcc sfx_gcc		# djgpp v2.x
gcc_dos:	_gcc_dos fu_gcc sfx_gcc		# djgpp v2.x

_gcc_dos: gcc_rsp
	make unzip CC=gcc LD=gcc M=msdos "CF=-O2 -Wall -I. $(LOC)" LOBJS="" "LF=-o unzip @gcc_rsp" LF2=""
#	$(MAKE) unzip CC=gcc LD=gcc M=msdos "CF=-O2 -Wall -I. $(LOC)"\
#	 LOBJS="" LF="-o unzip @gcc_rsp" LF2=""
	$(STRIP) unzip
	stubify unzip
	del gcc_rsp
	del unzip
	stubify -g zipinfo.exe
	stubedit zipinfo.exe runfile=unzip argv0=zipinfo

fu_gcc:
	make funzip CC=gcc LD=gcc "CF=-Wall -O2 -I. $(LOC)" FL2="" CP=copy RM=del
#	$(MAKE) funzip CC=gcc LD=gcc "CF=-Wall -O2 -I. $(LOC)" FL2=""\
#	 CP=copy RM=del
	$(STRIP) funzip
	stubify funzip
	del funzip

sfx_gcc:
	make unzipsfx CC=gcc LD=gcc M=msdos "CF=-Wall -O2 -I. $(LOC)" SL2="" CP=copy RM=del
#	$(MAKE) unzipsfx CC=gcc LD=gcc M=msdos "CF=-Wall -O2 -I. $(LOC)"\
#	 SL2="" CP=copy RM=del
	$(STRIP) unzipsfx
	stubify unzipsfx
	del unzipsfx

# Heurikon HK68 (68010), UniPlus+ System V 5.0, Green Hills C-68000
hk68:		unix_make
	$(MAKE) unzips CC="gcc" LD="gcc"\
	 LF="-n $(LF)" SL="-n $(SL)" FL="-n $(FL)"\
	 CF="-ga -X138 -I. $(LOC) -Dlocaltime=localti -Dtimezone=timezon"

# Linux (Posix, approximately SysV):  virtually any version since before 0.96.
linux:		unix_make
	$(MAKE) unzips CC=gcc LD=gcc

# Linux host with go32 (djgpp) cross-compiler (go32crs.tgz) for 32-bit DOS.
# Change msdos_$O target (near line 303 above) from msdos\msdos to msdos/msdos.
linux_dos:	unix_make
	$(MAKE) unzips CC=go32gcc LD=go32gcc M=msdos CF="-Wall -O2 -I. $(LOC)"
#	go32-strip unzip
#	Due to limitations of the cross-compiling package, this has to be
#	done manually:
	@echo Copy $(UNZIPS) to your DOS partition and use coff2exe.

# Linux ELF shared library (ooo, it's so easy).  This is a test target for
# now, and it only makes the UnZip/ZipInfo stuff (not fUnZip or UnZipSFX).
# Do "make clean" before using this target the first time, eh?  The version
# number may change to match the UnZip version.  Or not.  Whatever.  Also
# do "setenv LD_LIBRARY_PATH `pwd`" or similar to test the DLL in place
# (with unzip, which is now the version linked with the shared library).
#
linux_shlib:	unix_make
	$(MAKE) objsdll CC=gcc CF="$(CF) -DDLL -fPIC"
	gcc -shared -Wl,-soname,libunzip.so.0 -o libunzip.so.0.2 $(OBJSDLL)
	ln -sf libunzip.so.0.2 libunzip.so.0
	gcc -c -O unzipstb.c
	gcc -o unzip unzipstb.o -L. -lunzip

# Linux ELF shared library, as above, but using inflate() from zlib (libz.so)
# instead of the original UnZip version.  (libz was libgz prior to 0.94)
linux_shlibz:	unix_make
	$(MAKE) objsdll CC=gcc CF="$(CF) -DDLL -DUSE_ZLIB -fPIC"
	gcc -shared -Wl,-soname,libunzip.so.0 -o libunzip.so.0.2 $(OBJSDLL)
	ln -sf libunzip.so.0.2 libunzip.so.0
	gcc -c -O unzipstb.c
	gcc -o unzip unzipstb.o -L. -lunzip -lz

# Minix 1.5 PC for the 386.  Invoke as is to use default cc, or as "make
# minix CC=gcc" to use gcc.  Try "make linux" if you have a working termios.h.
minix:		unix_make
	$(MAKE) unzips CF="$(CF) -DMINIX -DSHORT_NAMES" CC=$(CC) LD=$(CC)

# MS-DOS:  Microsoft C 6.0 and NMAKE.  "nmake msc_dos" works fine, aside
# from an irrelevant message (possibly) about the creation of a temporary
# file.  Environment variable LOCAL_UNZIP (or CL) should be set via "SET
# LOCAL_UNZIP=-FPi87" if you use the 80x87 library; also add -G2 or -G3 if
# using a 286/386/486.  Note that setting too many things via LOCAL_UNZIP
# may make the command line too long for the default COMMAND.COM shell, so
# use CL instead in that case.  With VC++ Pro, -O2 instead of -Oait seems to
# work OK and makes the executables slightly faster.  Use at your own risk.
#
msc_dos:	_msc_dos fu_msc sfx_msc

# can change UnZip memory model (-AS) to large (-AL) if necessary, but should
# no longer be necessary in UnZip 5.11 and later
_msc_dos:	msc_rsp
	$(MAKE) unzip.exe CF="-AS -W3 -Oait -Gs -nologo -I. $(LOC)" CC=cl\
	 LD=link E=.exe O=.obj M=msdos LOBJS="" LF="@msc_rsp" LF2=""
	del msc_rsp

msc_rsp:
	echo $(OBJS1:.o=.obj) + > msc_rsp
	echo msdos.obj $(OBJS2:.o=.obj)/noi/e/st:0x0c00; >> msc_rsp

fu_msc:
	set CL=-AS -Oait -Gs -nologo -I.
	$(MAKE) funzip.exe CF="$(LOC)" CC=cl LD=link E=.exe O=.obj\
	 FL="/noi/nol/e" FL2=",funzip;" RM=del CP=copy

sfx_msc:
	set CL=-AS -Oait -Gs -nologo -I.
	$(MAKE) unzipsfx.exe CF="$(LOC)" CC=cl LD=link E=.exe O=.obj M=msdos\
	 SL="/noi/nol/e" SL2=",unzipsfx;" RM=del CP=copy

# $(NOD):  intended to be used as   SET NOD=-link /nod:slibcep   to allow the
#          use of default library names (slibce.lib) instead of protected-mode
#          names (slibcep.lib), but it fails:  MSC adds its own /nod qualifier,
#          and there seems to be no way to override this.  Typical...
#
#    THIS TARGET RETAINED AS AN EXAMPLE ONLY.  USE MAKEFILE.OS2.
#
#msc_os2:		# 16-bit OS/2 (1.x) with MSC 6.00 (use makefile.os2)
#	$(MAKE) -nologo unzips CC=cl LD=cl E=.exe O=.obj\
#	 OBJS="$(OBJS_OS2)" OBJZ="$(OBJZ_OS2)"\
#	 CF="-nologo -AC -Ocegit -G2s -DOS2 -DMSC $(LOC)"\
#	 LF="-nologo -AC $(LOC) -Lp -F 2000"\
#	 LF2="unzip.def -o unzip.exe $(NOD)" CP=copy RM=del\
#	 ZL="-nologo -AC $(LOC) -Lp -Fb" ZL2="zipinfo.def -o zipinfo.exe"

# NeXT info.
next:
	@echo
	@echo\
 '  Please pick a specific NeXT target:  "make next10" will create a generic'
	@echo\
 '  NeXT executable; "make next2x" will create a smaller executable (for'
	@echo\
 '  NeXTstep 2.0 and higher); "make next3x" will create a small executable'
	@echo\
 '  with significantly better optimization (NeXTstep 3.0 and higher only);'
	@echo\
 '  "make nextfat" will create a fat, multi-architecture (NeXT plus Intel)'
	@echo\
 '  executable (NeXTstep 3.1 and higher only).'
	@echo

# 68030 BSD 4.3+Mach.  NeXT 2.x: make the executable smaller.
next2x:		unix_make
	$(MAKE) unzips LF2="-object -s"

# NeXT 3.x: as above, plus better optimization.
next3x:		unix_make
	$(MAKE) unzips CF="-O2 -I. $(LOC)" LF2="-object -s"

# NeXT 3.1+: make the executable fat (multi-architecture binary [MAB],
# for "black" [NeXT] and "white" [x86] hardware, so far).
nextfat:	unix_make
	$(MAKE) unzips CF="-O2 -I. $(LOC) -arch i386 -arch m68k" \
	 LF2="-arch i386 -arch m68k -object -s"

# Sequent Symmetry running Dynix/ptx (sort of SysV.3):  needs to link
# with libseq to get symlink().
ptx:		unix_make
	$(MAKE) unzips CF="$(CF) -DSYSV -DTERMIO -DPTX" LF2="$(LF2) -lseq"

# Pyramid 90X (probably all) under >= OSx4.1, either universe.  (This is an
# experimental target!  If it fails, use either pyr_ucb or pyr_att instead.)
# The make in the BSD half is too stupid to understand $(MAKE), sigh...
pyramid:	unix_make
	-make -f unix/Makefile pyr_`universe`

# REGULUS:  68040-based, "real-time" SysV.3 mutant; uses gcc, with "REGULUS"
# predefined.
regulus:	unix_make
	$(MAKE) unzips CF="$(CF) -traditional -DSYSV -DNO_MKDIR"

# IBM RS/6000 under AIX 3.2
rs6000:		unix_make
	$(MAKE) unzips CF="$(CF) -DBSD -D_BSD -DUNIX" LF2="-lbsd"

# SCO cross compile from Unix to DOS. Tested with Xenix/386 and OpenDeskTop.
# Should work with Xenix/286 as well. (davidsen)  Note that you *must* remove
# the Unix objects and executable before doing this!  (Piet Plomp:  gcc won't
# recognize the -M0 flag which forces 8086 code.)  (GRR:  may need to reduce
# stack to 0c00h if using 286/small-model code...?)
sco_dos:	unix_make
	$(MAKE) unzips CF="-O -I. $(LOC) -dos -M0" M=msdos\
	 LF="-dos -F 2000" LF2="-o unzip.exe"\
	 FL="-dos" FL2="-o funzip.exe" SL="-dos" SL2="-o unzipsfx.exe"

# SCO UNIX with shared libraries and no international support.  If you are
# not using a USA-style keyboard and display, you may want to remove -nointl
# to get support.  It adds quite a bit to the size of the executable.
sco_sl:		unix_make
	$(MAKE) unzips LF="$(LF) -nointl" LF2="$(LF2) -lc_s"\
	 SL="$(SL) -nointl" FL="$(FL) -nointl"

# SCO Xenix/286 2.3.3 with 2.2.1 development system
sco_x286:	unix_make
	$(MAKE) unzips CF="$(CF) -Mel2 -LARGE -DTERMIO" LF="$(LF) -Mel2 -LARGE"

# Sequent Symmetry with Dynix.  (386, but needs -DZMEM)
# This should also work on Balance but I can't test it just yet.
sequent:	unix_make
	$(MAKE) unzips CF="$(CF) -DBSD -DZMEM"

# Generic System V + GNU C
sysv_gcc:	unix_make
	$(MAKE) unzips CC=gcc LD=gcc CF="-O2 -I. -DSYSV $(LOC)" LF2=""
	$(STRIP) $(UNZIPS)

# AT&T 6300+, System V.2 Unix:  run-time out-of-memory error if don't use -Ml;
# also compile-time error if work arrays dimensioned at HSIZE+2 (>32K)
sysv6300:	unix_make
	$(MAKE) unzips CF="$(CF) -Ml -DSYSV" LF="$(LF) -Ml"\
	 SL="$(SL) -Ml" FL="$(FL) -Ml"

# Texas Instruments System V.3 (running on HP 9000-1500)
ti_sysv:	unix_make
	$(MAKE) unzips CF="$(CF) -DSYSV -DNO_UID_GID -DUID_USHORT"

# SCO Xenix (Joe Foster 950508:  "unzip needs to be linked with -lx [for the
# opendir(), readdir(), telldir(), rewinddir(), and closedir() calls]")
xenix:		 unix_make
	$(MAKE) unzips LF2="$(LF2) -lx"

# Wombat 68000 (or whatever).
# I didn't do this.  I swear.  No, really.
wombat:		unix_make
	@echo
	@echo  '	Ha ha!  Just kidding.'
	@echo
