!==========================================================================
! MMS description file for UnZip/UnZipSFX 5.20 or later           11 Feb 96
!==========================================================================
!
! To build UnZip that uses shared libraries, edit the USER CUSTOMIZATION
! lines below to taste, then do
!	mms
! or
!	mmk
! if you use Matt's Make (free MMS-compatible make utility).
!
! (One-time users will find it easier to use the MAKE_UNZ.COM command file,
! which generates both UnZip and UnZipSFX.  Just type "@[.VMS]MAKE_UNZ", or
! "@[.VMS]MAKE_UNZ GCC" if you want to use GNU C.)

! Only when you try to build the default target
!   "all executables (linked against shareable images), and help file"
! you can simply type "mmk" or "mms".
! (You have to copy the description file to your working directory for MMS,
! with MMK you can alternatively use the /DESCR=[.VMS] qualifier.
!
! In all other cases where you want to explicitely specify a makefile target,
! you have to specify your compiling environment, too. These are:
!
!	$ MMS/MACRO=(__ALPHA__=1)		! Alpha AXP, (DEC C)
!	$ MMS/MACRO=(__DECC__=1)		! VAX, using DEC C
!	$ MMS/MACRO=(__FORCE_VAXC__=1)		! VAX, prefering VAXC over DECC
!	$ MMS/MACRO=(__VAXC__=1)		! VAX, where VAXC is default
!	$ MMS/MACRO=(__GNUC__=1)		! VAX, using GNU C
!

! To build UnZip without shared libraries,
!	mms noshare

! To delete all .OBJ, .OLB, .EXE and .HLP files,
!	mms clean

DO_THE_BUILD :
	@ decc = f$search("SYS$SYSTEM:DECC$COMPILER.EXE").nes.""
	@ axp = f$getsyi("HW_MODEL").ge.1024
	@ macro = "/MACRO=("
	@ if decc then macro = macro + "__DECC__=1,"
	@ if axp then macro = macro + "__ALPHA__=1,"
	@ if .not.(axp.or.decc) then macro = macro + "__VAXC__=1,"
	@ macro = f$extract(0,f$length(macro)-1,macro)+ ")"
	$(MMS)$(MMSQUALIFIERS)'macro' default

.IFDEF __ALPHA__
E = .AXP_EXE
O = .AXP_OBJ
A = .AXP_OLB
.ELSE
.IFDEF __DECC__
E = .VAX_DECC_EXE
O = .VAX_DECC_OBJ
A = .VAX_DECC_OLB
.ENDIF
.IFDEF __FORCE_VAXC__
__VAXC__ = 1
.ENDIF
.IFDEF __VAXC__
E = .VAX_VAXC_EXE
O = .VAX_VAXC_OBJ
A = .VAX_VAXC_OLB
.ENDIF
.IFDEF __GNUC__
E = .VAX_GNUC_EXE
O = .VAX_GNUC_OBJ
A = .VAX_GNUC_OLB
.ENDIF
.ENDIF

.IFDEF O
.ELSE
!If EXE and OBJ extentions aren't defined, define them
E = .EXE
O = .OBJ
A = .OLB
.ENDIF

!!!!!!!!!!!!!!!!!!!!!!!!!!! USER CUSTOMIZATION !!!!!!!!!!!!!!!!!!!!!!!!!!!!
! uncomment the following line if you want the VMS CLI$ interface:
!VMSCLI = VMSCLI,

! add VMSWILD, RETURN_CODES, and/or any other optional
! macros (except VMSCLI, above) to the following line for a custom version:
COMMON_DEFS =
!!!!!!!!!!!!!!!!!!!!!!!! END OF USER CUSTOMIZATION !!!!!!!!!!!!!!!!!!!!!!!!

.IFDEF __GNUC__
CC = gcc
LIBS = ,GNU_CC:[000000]GCCLIB.OLB/LIB
.ELSE
CC = cc
LIBS =
.ENDIF

CFLAGS = /NOLIST/INCL=(SYS$DISK:[])

OPTFILE = [.vms]vaxcshr.opt

.IFDEF __ALPHA__
CC_OPTIONS = /STANDARD=VAXC/PREFIX=ALL/ANSI
CC_DEFS = MODERN,
OPTFILE_LIST =
OPTIONS = $(LIBS)
NOSHARE_OPTS = $(LIBS)/NOSYSSHR
.ELSE
.IFDEF __DECC__
CC_OPTIONS = /DECC/STANDARD=VAXC/PREFIX=ALL
CC_DEFS = MODERN,
OPTFILE_LIST =
OPTIONS = $(LIBS)
NOSHARE_OPTS = $(LIBS),SYS$LIBRARY:DECCRTL.OLB/LIB/INCL=(CMA$TIS)/NOSYSSHR
.ELSE
.IFDEF __FORCE_VAXC__		!Select VAXC on systems where DEC C exists
CC_OPTIONS = /VAXC
.ELSE				!No flag allowed/needed on a pure VAXC system
CC_OPTIONS =
.ENDIF
CC_DEFS =
OPTFILE_LIST = ,$(OPTFILE)
OPTIONS = $(LIBS),$(OPTFILE)/OPTIONS
NOSHARE_OPTS = $(LIBS),SYS$LIBRARY:VAXCRTL.OLB/LIB/NOSYSSHR
.ENDIF
.ENDIF

.IFDEF __DEBUG__
CDEB = /DEBUG/NOOPTIMIZE
LDEB = /DEBUG
.ELSE
CDEB =
LDEB = /NOTRACE
.ENDIF

CFLAGS_SFX  = $(CC_OPTIONS) $(CFLAGS) $(CDEB) -
              /def=($(CC_DEFS) $(COMMON_DEFS) $(VMSCLI) SFX, VMS)
CFLAGS_ALL  = $(CC_OPTIONS) $(CFLAGS) $(CDEB) -
              /def=($(CC_DEFS) $(COMMON_DEFS) $(VMSCLI) VMS)

LINKFLAGS   = $(LDEB)


COMMON_OBJS1 =	unzip$(O),crc32$(O),crctab$(O),crypt$(O),envargs$(O),-
		explode$(O),extract$(O),fileio$(O),globals$(O)
COMMON_OBJS2 =	inflate$(O),list$(O),match$(O),process$(O),ttyio$(O),-
		unreduce$(O),unshrink$(O),zipinfo$(O),-
		vms$(O)
COMMON_OBJS =	$(COMMON_OBJS1),$(COMMON_OBJS2)

COMMON_OBJX1 =	UNZIP=unzipsfx$(O),-
		crc32$(O),crctab$(O),crypt$(O),-
		EXTRACT=extract_$(O),-
		fileio$(O),globals$(O)
COMMON_OBJX2 =	inflate$(O),match$(O),-
		PROCESS=process_$(O),-
		ttyio$(O),-
		VMS=vms_$(O)
COMMON_OBJX =	$(COMMON_OBJX1),$(COMMON_OBJX2)

.IFDEF VMSCLI
CLI_OBJS = VMS_UNZIP_CLD=unz_cli$(O),-
	VMS_UNZIP_CMDLINE=cmdline$(O)
OBJS =	$(COMMON_OBJS),$(CLI_OBJS)
CLI_OBJX = VMS_UNZIP_CLD=unz_cli$(O),-
	VMS_UNZIP_CMDLINE=cmdline_$(O)
OBJX =	$(COMMON_OBJX),$(CLI_OBJX)
.ELSE
CLI_OBJS =
OBJS =	$(COMMON_OBJS)
OBJX =	$(COMMON_OBJX)
.ENDIF

UNZIP_H = unzip.h unzpriv.h globals.h

!!!!!!!!!!!!!!!!!!! override default rules: !!!!!!!!!!!!!!!!!!!
.suffixes :
.suffixes : .ANL $(E) $(A) .MLB .HLB .TLB .FLB $(O) -
	    .FORM .BLI .B32 .C .COB -
	    .FOR .BAS .B16 .PLI .PEN .PAS .MAC .MAR .M64 .CLD .MSG .COR .DBL -
	    .RPG .SCN .IFDL .RBA .RC .RCO .RFO .RPA .SC .SCO .SFO .SPA .SPL -
	    .SQLADA .SQLMOD .RGK .RGC .MEM .RNO .HLP .RNH .L32 .REQ .R32 -
	    .L16 .R16 .TXT .H .FRM .MMS .DDL .COM .DAT .OPT .CDO .SDML .ADF -
	    .GDF .LDF .MDF .RDF .TDF

$(O)$(E) :
	$(LINK) $(LINKFLAGS) /EXE=$(MMS$TARGET) $(MMS$SOURCE)

$(O)$(A) :
	If "''F$Search("$(MMS$TARGET)")'" .EQS. "" Then $(LIBR)/Create $(MMS$TARGET)
	$(LIBR)$(LIBRFLAGS) $(MMS$TARGET) $(MMS$SOURCE)

.CLD$(O) :
	SET COMMAND /OBJECT=$(MMS$TARGET) $(CLDFLAGS) $(MMS$SOURCE)

.C$(O) :
	$(CC) $(CFLAGS_ALL) /OBJ=$(MMS$TARGET) $(MMS$SOURCE)

!!!!!!!!!!!!!!!!!! here starts the unzip specific part !!!!!!!!!!!

default	:   	unzip$(E) unzipsfx$(E) unzip.hlp
	@	!	Do nothing.

noshare :	unzip_noshare$(E) unzipsfx_noshare$(E) unzip.hlp
	@	!	Do nothing.

unzip$(E) :	UNZIP$(A)($(OBJS))$(OPTFILE_LIST)
	$(LINK)$(LINKFLAGS) /EXE=$(MMS$TARGET) -
	UNZIP$(A)/INCLUDE=UNZIP/LIBRARY$(OPTIONS), -
	[.vms]unzip.opt/OPT

unzipsfx$(E) :	UNZIPSFX$(A)($(OBJX))$(OPTFILE_LIST)
	$(LINK)$(LINKFLAGS) /EXE=$(MMS$TARGET) -
	UNZIPSFX$(A)/INCLUDE=UNZIP/LIBRARY$(OPTIONS), -
	[.vms]unzipsfx.opt/OPT

unzip_noshare$(E) :	UNZIP$(A)($(OBJS))
	$(LINK)$(LINKFLAGS) /EXE=$(MMS$TARGET) -
	UNZIP$(A)/INCLUDE=UNZIP/LIBRARY$(NOSHARE_OPTS), -
	sys$disk:[.vms]unzip.opt/OPT

unzipsfx_noshare$(E) :	UNZIPSFX$(A)($(OBJX))
	$(LINK)$(LINKFLAGS) /EXE=$(MMS$TARGET) -
	UNZIPSFX$(A)/INCLUDE=UNZIP/LIBRARY$(NOSHARE_OPTS), -
	sys$disk:[.vms]unzipsfx.opt/OPT

$(OPTFILE) :
	@ open/write tmp $(MMS$TARGET)
	@ write tmp "SYS$SHARE:VAXCRTL.EXE/SHARE"
	@ close tmp

clean.com :
	@ open/write tmp $(MMS$TARGET)
	@ write tmp "$!"
	@ write tmp "$!	Clean.com --	procedure to delete files. It always returns success"
	@ write tmp "$!			status despite any error or warnings. Also it extracts"
	@ write tmp "$!			filename from MMS ""module=file"" format."
	@ write tmp "$!"
	@ write tmp "$ on control_y then goto ctly"
	@ write tmp "$ if p1.eqs."""" then exit 1"
	@ write tmp "$ i = -1"
	@ write tmp "$scan_list:"
	@ write tmp "$	i = i+1"
	@ write tmp "$	item = f$elem(i,"","",p1)"
	@ write tmp "$	if item.eqs."""" then goto scan_list"
	@ write tmp "$	if item.eqs."","" then goto done		! End of list"
	@ write tmp "$	item = f$edit(item,""trim"")		! Clean of blanks"
	@ write tmp "$	wild = f$elem(1,""="",item)"
	@ write tmp "$	show sym wild"
	@ write tmp "$	if wild.eqs.""="" then wild = f$elem(0,""="",item)"
	@ write tmp "$	vers = f$parse(wild,,,""version"",""syntax_only"")"
	@ write tmp "$	if vers.eqs."";"" then wild = wild - "";"" + "";*"""
	@ write tmp "$scan:"
	@ write tmp "$		f = f$search(wild)"
	@ write tmp "$		if f.eqs."""" then goto scan_list"
	@ write tmp "$		on error then goto err"
	@ write tmp "$		on warning then goto warn"
	@ write tmp "$		delete/log 'f'"
	@ write tmp "$warn:"
	@ write tmp "$err:"
	@ write tmp "$		goto scan"
	@ write tmp "$done:"
	@ write tmp "$ctly:"
	@ write tmp "$	exit 1"
	@ close tmp

clean : clean.com
	! delete *$(O);*, *$(A);*, unzip$(exe);*, unzipsfx$(exe);*, -
	!  unzip.hlp;*, [.vms]unzip.rnh;*
!	@clean "$(OBJS)"
	@clean "$(COMMON_OBJS1)"
	@clean "$(COMMON_OBJS2)"
	@clean "$(CLI_OBJS)"
!	@clean "$(OBJX)"
	@clean "$(COMMON_OBJX1)"
	@clean "$(COMMON_OBJX2)"
	@clean "$(CLI_OBJX)"
        @clean "$(OPTFILE)"
	@clean unzip$(A),unzipsfx$(A)
	@clean unzip$(E),unzipsfx$(E)
	@clean unzip_noshare$(E),unzipsfx_noshare$(E)
	@clean unzip.hlp,[.vms]unzip.rnh
	- delete/noconfirm/nolog clean.com;*

crc32$(O)		: crc32.c $(UNZIP_H) zip.h
crctab$(O)		: crctab.c $(UNZIP_H) zip.h
crypt$(O)		: crypt.c $(UNZIP_H) zip.h crypt.h ttyio.h
envargs$(O)		: envargs.c $(UNZIP_H)
explode$(O)		: explode.c $(UNZIP_H)
extract$(O)		: extract.c $(UNZIP_H) crypt.h
fileio$(O)		: fileio.c $(UNZIP_H) crypt.h ttyio.h ebcdic.h
globals$(O)		: globals.c $(UNZIP_H)
inflate$(O)		: inflate.c inflate.h $(UNZIP_H)
list$(O)		: list.c $(UNZIP_H)
match$(O)		: match.c $(UNZIP_H)
process$(O)		: process.c $(UNZIP_H)
ttyio$(O)		: ttyio.c $(UNZIP_H) zip.h crypt.h ttyio.h
unreduce$(O)		: unreduce.c $(UNZIP_H)
unshrink$(O)		: unshrink.c $(UNZIP_H)
unzip$(O)		: unzip.c $(UNZIP_H) crypt.h version.h consts.h
unzip.hlp		: [.vms]unzip.rnh
zipinfo$(O)		: zipinfo.c $(UNZIP_H)
cmdline$(O)		: [.vms]cmdline.c $(UNZIP_H) version.h
unz_cli$(O)		: [.vms]unz_cli.cld

cmdline_$(O)		: [.vms]cmdline.c $(UNZIP_H) version.h
	$(CC) $(CFLAGS_SFX) /OBJ=$(MMS$TARGET) [.vms]cmdline.c

extract_$(O)		: extract.c $(UNZIP_H) crypt.h
	$(CC) $(CFLAGS_SFX) /OBJ=$(MMS$TARGET) extract.c

process_$(O)		: process.c $(UNZIP_H)
	$(CC) $(CFLAGS_SFX) /OBJ=$(MMS$TARGET) process.c

unzipsfx$(O)		: unzip.c $(UNZIP_H) crypt.h version.h consts.h
	$(CC) $(CFLAGS_SFX) /OBJ=$(MMS$TARGET) unzip.c

vms$(O)			: [.vms]vms.c [.vms]vms.h [.vms]vmsdefs.h $(UNZIP_H)
!	@ x = ""
!	@ if f$search("SYS$LIBRARY:SYS$LIB_C.TLB").nes."" then x = "+SYS$LIBRARY:SYS$LIB_C.TLB/LIBRARY"
	$(CC) $(CFLAGS_ALL) /OBJ=$(MMS$TARGET) [.vms]vms.c

vms_$(O)		: [.vms]vms.c [.vms]vms.h [.vms]vmsdefs.h $(UNZIP_H)
!	@ x = ""
!	@ if f$search("SYS$LIBRARY:SYS$LIB_C.TLB").nes."" then x = "+SYS$LIBRARY:SYS$LIB_C.TLB/LIBRARY"
	$(CC) $(CFLAGS_SFX) /OBJ=$(MMS$TARGET) [.vms]vms.c


.IFDEF VMSCLI

[.vms]unzip.rnh		: [.vms]unzip_cli.help
	@ set default [.vms]
	edit/tpu/nosection/nodisplay/command=cvthelp.tpu unzip_cli.help
	rename unzip_cli.rnh unzip.rnh
	@ set default [-]

.ELSE

[.vms]unzip.rnh		: [.vms]unzip_def.rnh
	copy [.vms]unzip_def.rnh [.vms]unzip.rnh

.ENDIF
