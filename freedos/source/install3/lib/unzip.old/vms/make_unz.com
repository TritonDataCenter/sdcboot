$ ! MAKE_UNZ.COM
$ !
$ !	"Makefile" for VMS versions of UnZip/ZipInfo and UnZipSFX
$ !
$ !	last revised:  11 Feb 96
$ !
$ !	To define additional options, define the global symbol
$ !	LOCAL_UNZIP prior to executing MAKE_UNZ.COM:
$ !
$ !		$ LOCAL_UNZIP == "VMSCLI,RETURN_CODES,"
$ !		$ @MAKE_UNZ
$ !
$ !	The trailing "," may be omitted.  Valid VMS-specific options
$ !	include VMSCLI, VMSWILD and RETURN_CODES; see the INSTALL file
$ !	for other options (e.g., CHECK_VERSIONS).
$ !
$ !
$ on error then goto error
$ on control_y then goto error
$ OLD_VERIFY = f$verify (0)
$!
$ say := write sys$output
$!##################### Customizing section #############################
$!
$ MAY_USE_DECC = 1
$ MAY_USE_GNUC = 0
$!
$! Process command line parameters requesting optional features:
$ arg_cnt = 1
$ argloop:
$  current_arg_name = "P''arg_cnt'"
$  curr_arg = f$edit('current_arg_name',"UPCASE")
$  IF curr_arg .eqs. "" THEN GOTO argloop_out
$  IF curr_arg .eqs. "VAXC"
$  THEN MAY_USE_DECC = 0
$    MAY_USE_GNUC = 0
$  ENDIF
$  IF curr_arg .eqs. "DECC"
$  THEN MAY_USE_DECC = 1
$    MAY_USE_GNUC = 0
$  ENDIF
$  IF curr_arg .eqs. "GNUC"
$  THEN MAY_USE_DECC = 0
$    MAY_USE_GNUC = 1
$  ENDIF
$  arg_cnt = arg_cnt + 1
$ GOTO argloop
$ argloop_out:
$!
$!#######################################################################
$!
$ ! Find out current disk, directory, compiler and options
$ !
$ my_name = f$env("procedure")
$ workdir = f$env("default")
$ here = f$parse(workdir,,,"device") + f$parse(workdir,,,"directory")
$ if f$type(LOCAL_UNZIP).eqs.""
$ then
$	local_unzip = ""
$ else	! Trim blanks and append comma if missing
$	local_unzip = f$edit(local_unzip, "TRIM")
$	if f$extract(f$length(local_unzip)-1, 1, local_unzip).nes."," then -
		local_unzip = local_unzip + ","
$ endif
$ axp = f$getsyi("HW_MODEL").ge.1024
$ if axp
$ then
$	! Alpha AXP
$	ARCH_NAME == "Alpha"
$	ARCH_PREF = "AXP_"
$	HAVE_DECC_VAX = 0
$	USE_DECC_VAX = 0
$       if MAY_USE_GNUC
$	then say "GNU C has not yet been ported to OpenVMS AXP."
$	     say "You must use DEC C to build Zip."
$	     goto error
$	endif
$	ARCH_CC_P = ARCH_PREF
$	cc = "cc/standard=vaxc/prefix=all/ansi"
$	defs = "''local_unzip'MODERN"
$	opts = ""
$	say "Compiling on AXP using DECC"
$ else
$	! VAX
$	ARCH_NAME == "VAX"
$	ARCH_PREF = "VAX_"
$       HAVE_DECC_VAX = (F$TRNLNM("DECC$LIBRARY_INCLUDE") .NES. "")
$	HAVE_VAXC_VAX = (f$search("SYS$SYSTEM:VAXC.EXE").nes."")
$       MAY_HAVE_GNUC = (f$trnlnm("GNU_CC").nes."")
$	IF HAVE_DECC_VAX .AND. MAY_USE_DECC
$	THEN
$!	  We use DECC:
$	  USE_DECC_VAX = 1
$	  cc = "cc/decc/standard=vaxc/prefix=all"
$	  ARCH_CC_P = "''ARCH_PREF'DECC_"
$	  defs = "''local_unzip'MODERN"
$	  opts = ""
$	  say "Compiling on VAX using DECC"
$       ELSE
$!	  We use VAXC (or GNU C):
$	  USE_DECC_VAX = 0
$	  defs = "''local_unzip'VMS"
$	  opts = ",[.VMS]VAXCSHR.OPT/OPTIONS"
$	  if (.not.HAVE_VAXC_VAX .and. MAY_HAVE_GNUC) .or. (MAY_USE_GNUC)
$	  then
$		ARCH_CC_P = "''ARCH_PREF'GNUC_"
$		cc = "gcc"
$		opts = ",GNU_CC:[000000]GCCLIB.OLB/LIB ''opts'"
$		say "Compiling on VAX using GNUC"
$	  else
$		ARCH_CC_P = "''ARCH_PREF'VAXC_"
$		if HAVE_DECC_VAX
$		then
$	  	    cc = "cc/vaxc"
$		else
$		    cc = "cc"
$		endif
$		say "Compiling on VAX using VAXC"
$	  endif
$	ENDIF
$ endif
$ def = "/define=(''defs')"
$ LFLAGS = "/notrace"
$ if (opts .nes. "") .and. (f$search("[.vms]vaxcshr.opt") .eqs. "")
$ then	create [.vms]vaxcshr.opt
$	open/append tmp [.vms]vaxcshr.opt
$	write tmp "SYS$SHARE:VAXCRTL.EXE/SHARE"
$	close tmp
$ endif
$ !
$ ! Currently, the following section is not needed, as vms.c does no longer
$ ! include any of the headers from SYS$LIB_C.TLB.
$ ! The commented section is solely maintained for reference.
$ ! In case system headers from SYS$LIB_C.TLB are needed again,
$ ! just append "'c'" to the respective source file specification.
$! x = f$search("SYS$LIBRARY:SYS$LIB_C.TLB")
$! if x .nes. "" then x = "+" + x
$ !
$ tmp = f$ver(1)	! Turn echo on to see what's happening
$ !
$ cc/NOLIST'DEF' /OBJ=unzip.'ARCH_CC_P'obj unzip.c
$ cc/NOLIST'DEF' /OBJ=crc32.'ARCH_CC_P'obj crc32.c
$ cc/NOLIST'DEF' /OBJ=crctab.'ARCH_CC_P'obj crctab.c
$ cc/NOLIST'DEF' /OBJ=crypt.'ARCH_CC_P'obj crypt.c
$ cc/NOLIST'DEF' /OBJ=envargs.'ARCH_CC_P'obj envargs.c
$ cc/NOLIST'DEF' /OBJ=explode.'ARCH_CC_P'obj explode.c
$ cc/NOLIST'DEF' /OBJ=extract.'ARCH_CC_P'obj extract.c
$ cc/NOLIST'DEF' /OBJ=fileio.'ARCH_CC_P'obj fileio.c
$ cc/NOLIST'DEF' /OBJ=globals.'ARCH_CC_P'obj globals.c
$ cc/NOLIST'DEF' /OBJ=inflate.'ARCH_CC_P'obj inflate.c
$ cc/NOLIST'DEF' /OBJ=list.'ARCH_CC_P'obj list.c
$ cc/NOLIST'DEF' /OBJ=match.'ARCH_CC_P'obj match.c
$ cc/NOLIST'DEF' /OBJ=process.'ARCH_CC_P'obj process.c
$ cc/NOLIST'DEF' /OBJ=ttyio.'ARCH_CC_P'obj ttyio.c
$ cc/NOLIST'DEF' /OBJ=unreduce.'ARCH_CC_P'obj unreduce.c
$ cc/NOLIST'DEF' /OBJ=unshrink.'ARCH_CC_P'obj unshrink.c
$ cc/NOLIST'DEF' /OBJ=zipinfo.'ARCH_CC_P'obj zipinfo.c
$ cc/INCLUDE=SYS$DISK:[]'DEF' /OBJ=vms.'ARCH_CC_P'obj; [.vms]vms.c
$ !
$ if f$locate("VMSCLI",local_unzip).ne.f$length(local_unzip)
$ then
$	cc/INCLUDE=SYS$DISK:[]'DEF' /OBJ=cmdline.'ARCH_CC_P'obj; -
		[.vms]cmdline.c
$	cc/INCLUDE=SYS$DISK:[]/DEF=('DEFS',SFX) -
		/OBJ=cmdline_.'ARCH_CC_P'obj; -
		[.vms]cmdline.c
$	set command/obj=unz_cli.'ARCH_CC_P'obj [.vms]unz_cli.cld
$	cliobjs = ",cmdline.'ARCH_CC_P'obj, unz_cli.'ARCH_CC_P'obj"
$	cliobjx = ",cmdline_.'ARCH_CC_P'obj, unz_cli.'ARCH_CC_P'obj"
$	set default [.vms]
$	edit/tpu/nosection/nodisplay/command=cvthelp.tpu unzip_cli.help
$	set default [-]
$	runoff/out=unzip.hlp [.vms]unzip_cli.rnh
$ else
$	cliobjs = ""
$	cliobjx = ""
$	runoff/out=unzip.hlp [.vms]unzip_def.rnh
$ endif
$ !
$ IF F$SEARCH("unzip.''ARCH_CC_P'olb") .EQS. "" THEN -
	lib/obj/create unzip.'ARCH_CC_P'olb
$ lib/obj/replace unzip.'ARCH_CC_P'olb -
	unzip.'ARCH_CC_P'obj;, crc32.'ARCH_CC_P'obj;, -
	crctab.'ARCH_CC_P'obj;, crypt.'ARCH_CC_P'obj;, -
	envargs.'ARCH_CC_P'obj;, explode.'ARCH_CC_P'obj;, -
	extract.'ARCH_CC_P'obj;, fileio.'ARCH_CC_P'obj;, -
	globals.'ARCH_CC_P'obj;, inflate.'ARCH_CC_P'obj;, -
	list.'ARCH_CC_P'obj;, match.'ARCH_CC_P'obj;, -
	process.'ARCH_CC_P'obj;, ttyio.'ARCH_CC_P'obj;, -
	unreduce.'ARCH_CC_P'obj;, unshrink.'ARCH_CC_P'obj;, -
	zipinfo.'ARCH_CC_P'obj;, vms.'ARCH_CC_P'obj; 'cliobjs'
$ link'LFLAGS'/exe=unzip.'ARCH_CC_P'exe -
	unzip.'ARCH_CC_P'olb;/incl=(unzip)/lib -
	'opts', [.VMS]unzip.opt/opt
$ !
$ cc/DEF=('DEFS',SFX)/NOLIST /OBJ=unzipsfx.'ARCH_CC_P'obj unzip.c
$ cc/DEF=('DEFS',SFX)/NOLIST /OBJ=extract_.'ARCH_CC_P'obj extract.c
$ cc/DEF=('DEFS',SFX)/NOLIST /OBJ=process_.'ARCH_CC_P'obj process.c
$ cc/DEF=('DEFS',SFX)/INCLUDE=SYS$DISK:[] /OBJ=vms_.'ARCH_CC_P'obj; -
	[.vms]vms.c
$ IF F$SEARCH("unzipsfx.''ARCH_CC_P'olb") .EQS. "" THEN -
	lib/obj/create unzipsfx.'ARCH_CC_P'olb
$ lib/obj/replace unzipsfx.'ARCH_CC_P'olb -
	unzipsfx.'ARCH_CC_P'obj, crc32.'ARCH_CC_P'obj, -
	crctab.'ARCH_CC_P'obj, crypt.'ARCH_CC_P'obj, -
	extract_.'ARCH_CC_P'obj, fileio.'ARCH_CC_P'obj, -
	globals.'ARCH_CC_P'obj, inflate.'ARCH_CC_P'obj, -
	match.'ARCH_CC_P'obj, process_.'ARCH_CC_P'obj, -
	ttyio.'ARCH_CC_P'obj, vms_.'ARCH_CC_P'obj 'cliobjx'
$ link'LFLAGS'/exe=unzipsfx.'ARCH_CC_P'exe -
	unzipsfx.'ARCH_CC_P'olb;/lib/incl=unzip -
	'opts', [.VMS]unzipsfx.opt/opt
$ !
$ ! Next line:  put similar lines (full pathname for unzip.'ARCH_CC_P'exe) in
$ ! login.com.  Remember to include the leading "$" before disk name.
$ !
$! unzip == "$''here'unzip.'ARCH_CC_P'exe"		! command symbol for unzip
$! zipinfo == "$''here'unzip.'ARCH_CC_P'exe ""-Z"""	! command symbol for zipinfo
$ !
$error:
$ tmp = f$ver(old_verify)
$ exit
