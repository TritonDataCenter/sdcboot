$ ! LINK_UNZ.COM
$ !
$ !	Command procedure to (re)link the VMS versions of
$ !	UnZip/ZipInfo and UnZipSFX
$ !
$ !	last updated:  10 Oct 95
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
$	opts = ""
$	say "Linking on AXP using DECC"
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
$	  ARCH_CC_P = "''ARCH_PREF'DECC_"
$	  opts = ""
$	  say "Linking on VAX using DECC"
$       ELSE
$!	  We use VAXC (or GNU C):
$	  USE_DECC_VAX = 0
$	  opts = ",[.VMS]VAXCSHR.OPT/OPTIONS"
$	  if (.not.HAVE_VAXC_VAX .and. MAY_HAVE_GNUC) .or. (MAY_USE_GNUC)
$	  then
$		ARCH_CC_P = "''ARCH_PREF'GNUC_"
$		opts = ",GNU_CC:[000000]GCCLIB.OLB/LIB ''opts'"
$		say "Linking on VAX using GNUC"
$	  else
$		ARCH_CC_P = "''ARCH_PREF'VAXC_"
$		say "Linking on VAX using VAXC"
$	  endif
$	ENDIF
$ endif
$ LFLAGS = "/notrace"
$ if (opts .nes. "") .and. (f$search("[.vms]vaxcshr.opt") .eqs. "")
$ then	create [.vms]vaxcshr.opt
$	open/append tmp [.vms]vaxcshr.opt
$	write tmp "SYS$SHARE:VAXCRTL.EXE/SHARE"
$	close tmp
$ endif
$ tmp = f$ver(1)	! Turn echo on to see what's happening
$ !
$ link'LFLAGS'/exe=unzip.'ARCH_CC_P'exe -
	unzip.'ARCH_CC_P'olb;/incl=(unzip)/lib -
	'opts', [.VMS]unzip.opt/opt
$ !
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
