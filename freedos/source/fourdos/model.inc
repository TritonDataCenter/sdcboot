include  util.inc
% IFIDNI <_MODEL>,<TINY>
__TINY__	equ	-1
__SMALL__	equ	0
__MEDIUM__	equ	0
__COMPACT__	equ	0
__LARGE__	equ	0
__HUGE__	equ	0
__FLAT__	equ	0
% ELSEIFIDNI <_MODEL>,<SMALL>
__TINY__	equ	0
__SMALL__	equ	-1
__MEDIUM__	equ	0
__COMPACT__	equ	0
__LARGE__	equ	0
__HUGE__	equ	0
__FLAT__	equ	0
% ELSEIFIDNI <_MODEL>,<MEDIUM>
__TINY__	equ	0
__SMALL__	equ	0
__MEDIUM__	equ	-1
__COMPACT__	equ	0
__LARGE__	equ	0
__HUGE__	equ	0
__FLAT__	equ	0
% ELSEIFIDNI <_MODEL>,<COMPACT>
__TINY__	equ	0
__SMALL__	equ	0
__MEDIUM__	equ	0
__COMPACT__	equ	-1
__LARGE__	equ	0
__HUGE__	equ	0
__FLAT__	equ	0
% ELSEIFIDNI <_MODEL>,<LARGE>
__TINY__	equ	0
__SMALL__	equ	0
__MEDIUM__	equ	0
__COMPACT__	equ	0
__LARGE__	equ	-1
__HUGE__	equ	0
__FLAT__	equ	0
% ELSEIFIDNI <_MODEL>,<HUGE>
__TINY__	equ	0
__SMALL__	equ	0
__MEDIUM__	equ	0
__COMPACT__	equ	0
__LARGE__	equ	0
__HUGE__	equ	-1
__FLAT__	equ	0
% ELSEIFIDNI <_MODEL>,<FLAT>
__TINY__	equ	0
__SMALL__	equ	0
__MEDIUM__	equ	0
__COMPACT__	equ	0
__LARGE__	equ	0
__HUGE__	equ	0
__FLAT__	equ	-1
        .386
ELSE
.error	<No model specified or unrecognized _MODEL option>
__TINY__	equ	0
__SMALL__	equ	-1
__MEDIUM__	equ	0
__COMPACT__	equ	0
__LARGE__	equ	0
__HUGE__	equ	0
__FLAT__	equ	0
ENDIF
IF __FLAT__
xax     equ     eax
xbx     equ     ebx
xcx     equ     ecx
jxcxz   equ     jecxz
xdx     equ     edx
xsi     equ     esi
xdi     equ     edi
xbp     equ     ebp
xsp     equ     esp
ELSE
xax     equ     ax
xbx     equ     bx
xcx     equ     cx
jxcxz   equ     jcxz
xdx     equ     dx
xsi     equ     si
xdi     equ     di
xbp     equ     bp
xsp     equ     sp
ENDIF
IF __FLAT__
__SMALL_CODE__	equ	0
__LARGE_CODE__	equ	0
__FLAT_CODE__   equ     -1
AUTO		equ	<NEAR>
AUTO_IPTR	equ	<DWORD PTR>
ELSEIF __TINY__ OR __SMALL__ OR __COMPACT__
__SMALL_CODE__	equ	-1
__LARGE_CODE__	equ	0
__FLAT_CODE__   equ     0
AUTO		equ	<NEAR>
AUTO_IPTR	equ	<WORD PTR>
ELSE
__SMALL_CODE__	equ	0
__LARGE_CODE__	equ	-1
__FLAT_CODE__   equ     0
AUTO		equ	<FAR>
AUTO_IPTR	equ	<DWORD PTR>
ENDIF
IF __FLAT__
__SMALL_DATA__	equ	0
__LARGE_DATA__	equ	0
__FLAT_DATA__	equ	-1
ELSEIF __TINY__ OR __SMALL__ OR __MEDIUM__
__SMALL_DATA__	equ	-1
__LARGE_DATA__	equ	0
__FLAT_DATA__	equ	0
ELSE
__SMALL_DATA__	equ	0
__LARGE_DATA__	equ	-1
__FLAT_DATA__	equ	0
ENDIF
IF __FLAT__
$CODE_ALIGN	equ	<DWORD>
ELSE
$CODE_ALIGN	equ	<BYTE>
ENDIF
IF __FLAT__
$CONST_ALIGN	equ	<DWORD>
ELSE
$CONST_ALIGN	equ	<WORD>
ENDIF
IF __FLAT__
$DATA_ALIGN	equ	<DWORD>
ELSE
$DATA_ALIGN	equ	<WORD>
ENDIF
IF __FLAT__
$BSS_ALIGN	equ	<DWORD>
ELSE
$BSS_ALIGN	equ	<WORD>
ENDIF
IF __FLAT__
$FDATA_ALIGN	equ	<DWORD>
ELSE
$FDATA_ALIGN	equ	<WORD>
ENDIF
IF __FLAT__
$FBSS_ALIGN	equ	<DWORD>
ELSE
$FBSS_ALIGN	equ	<WORD>
ENDIF
$STACK_ALIGN	equ	<PARA>
comment }
.codeseg	macro	sname,align
  IFB <sname>
% .codeseg	<@filename>,sname
    exitm
  ELSEIFIDNI <sname>,<byte>
% .codeseg	<@filename>,sname
    exitm
  ELSEIFIDNI <sname>,<word>
% .codeseg	<@filename>,sname
    exitm
  ELSEIFIDNI <sname>,<dword>
% .codeseg	<@filename>,sname
    exitm
  ELSEIFIDNI <sname>,<para>
% .codeseg	<@filename>,sname
    exitm
  ELSEIFIDNI <sname>,<page>
% .codeseg	<@filename>,sname
    exitm
  ENDIF
  IFNB <align>
$CODE_ALIGN	equ	<align>
  ENDIF
  IF __SMALL_CODE__
    IF __TINY__
% _TEXT		segment $CODE_ALIGN public 'CODE'
DGROUP		group	_TEXT
@CODESEG	equ	<DGROUP>
@CODESEG&sname	equ	<DGROUP>
    ELSE
% _TEXT		segment $CODE_ALIGN public 'CODE'
@CODESEG	equ	<_TEXT>
@CODESEG&sname	equ	<_TEXT>
    ENDIF
  ELSEIFNB <sname>
%   IFIDNI <sname>,<lib>
% LIB_TEXT	segment $CODE_ALIGN public 'CODE'
@CODESEG	equ	<LIB_TEXT>
@CODESEGLIB	equ	<LIB_TEXT>
    ELSE
% sname&_TEXT	segment $CODE_ALIGN public 'CODE'
@CODESEG	equ	<sname&_TEXT>
@CODESEG&sname	equ	<sname&_TEXT>
    ENDIF
  ELSE
% .codeseg	@filename
  ENDIF
% assume	cs:@codeseg
endm
}
.constseg	macro	align
  IFNB <align>
$CONST_ALIGN	equ	<align>
  ENDIF
    IF __TASM__
      nowarn
    ENDIF
  IF __FLAT__
%   CONST32	segment $CONST_ALIGN USE32 public 'CONST'
    DGROUP	group   CONST32
  ELSE
%   CONST	segment $CONST_ALIGN public 'CONST'
    DGROUP	group   CONST
  ENDIF
    IF __TASM__
      warn
    ENDIF
  @CONSTSEG	equ	<DGROUP>
endm
.dataseg	macro	align
  IFNB <align>
$DATA_ALIGN	equ	<align>
  ENDIF
  IF __FLAT__
%   DATA32	segment $DATA_ALIGN USE32 public 'DATA'
    DGROUP	group   DATA32
  ELSE
%   _DATA	segment $DATA_ALIGN public 'DATA'
    DGROUP	group   _DATA
  ENDIF
@DATASEG	equ	<DGROUP>
endm
.dataseg?	macro	align
  IFNB <align>
$BSS_ALIGN	equ	<align>
  ENDIF
  IF __FLAT__
%   BSS32	segment $BSS_ALIGN USE32 public 'BSS'
    DGROUP	group   BSS32
  ELSE
%   _BSS	segment $BSS_ALIGN public 'BSS'
    DGROUP	group   _BSS
  ENDIF
@DATASEG?	equ	<DGROUP>
endm
.fardataseg	macro	sname,align
  IFB <sname>
.error	<.FARDATASEG must have a name argument>
  ENDIF
  IFNB <align>
$FDATA_ALIGN	equ	<align>
  ENDIF
% sname&_DATA	segment $FDATA_ALIGN public 'FAR_DATA'
@FARDATASEG&sname equ	<sname&_DATA>
endm
.fardataseg?	macro	sname,align
  IFB <sname>
.error	<.FARDATASEG? must have a name argument>
  ENDIF
  IFNB <align>
$FBSS_ALIGN	equ	<align>
  ENDIF
% sname&_BSS	segment $FBSS_ALIGN public 'FAR_BSS'
@FARDATASEG?&sname equ	<sname&_BSS>
endm
.stackseg	macro	align
  IFNB <align>
$STACK_ALIGN	equ	<align>
  ENDIF
    IF __TASM__
      nowarn
    ENDIF
  IF __FLAT__
%   STACK32	segment $STACK_ALIGN USE32 public 'STACK'
  ELSE
%   STACK	segment	$STACK_ALIGN stack 'STACK'
  ENDIF
    IF __TASM__
      warn
    ENDIF
  IF __FLAT_DATA__
DGROUP		group	STACK32
@STACKSEG	equ	<DGROUP>
  ELSEIF __SMALL_DATA__
DGROUP		group	STACK
@STACKSEG	equ	<DGROUP>
  ELSE
DGROUP		group	STACK
@STACKSEG	equ	<STACK>
  ENDIF
endm
.ends		macro
@Curseg		ends
endm
.public		macro	p0,p1,p2,p3,p4,p5,p6,p7,p8,p9,p10
  IFNB <p10>
.error	<.PUBLIC has more than 10 arguments>
  ENDIF
  irp $pub,<p0,p1,p2,p3,p4,p5,p6,p7,p8,p9>
    IFB <$pub>
      exitm
    ENDIF
    IF __TASM__
      nowarn pdc
    ENDIF
    IF1
      IFDEF _PREFIX
%       IFDIFI <_PREFIX>,<>
%         $pub catstr <_PREFIX>,<$pub>
        ENDIF
      ENDIF
    ENDIF
    IF __TASM__
      warn pdc
    ENDIF
% public	$pub
  endm
endm
.publicx		macro	p0
    IF1
      IFDEF _PREFIX
%       IFDIFI <_PREFIX>,<>
%         p0 catstr <_PREFIX>,<p0>
        ENDIF
      ENDIF
    ENDIF
% public	p0
endm
.proc		macro	procname, dist
$CURPROC	equ	<procname>
  IFNB <dist>
% procname	proc	dist
  ELSE
% procname	proc	auto
  ENDIF
endm
.endp		macro	procname
  IFNB <procname>
% procname	endp
  ELSE
% $CURPROC	endp
  ENDIF
endm
.label		macro	labelname, dist
  IFNB <dist>
% labelname	label	dist
  ELSE
% labelname	label	auto
  ENDIF
endm
.extrn		macro	e0,e1,e2,e3,e4,e5,e6,e7,e8,e9,e10
  IFNB <e10>
.error	<.EXTRN has more than 10 arguments>
  ENDIF
  irp $epair,<e0,e1,e2,e3,e4,e5,e6,e7,e8,e9>
    IFB <$epair>
      exitm
    ENDIF
    $$$pos instr <$epair>,<:>
    IFE $$$pos
.error	<.EXTRN argument requires a type: &$epair>
      exitm
    ENDIF
    $$$typ substr <$epair>,($$$pos+1)
    $$$nam substr <$epair>,1,($$$pos-1)
%   IFIDNI <$$$TYP>,<>
.error	<.EXTRN argument requires a type: &$epair>
      exitm
    ENDIF
    IF __TASM__
      nowarn pdc
    ENDIF
    IF1
      IFDEF _PREFIX
%       IFDIFI <_PREFIX>,<>
%         $$$nam catstr <_PREFIX>,<$$$nam>
        ENDIF
      ENDIF
    ENDIF
    IF __TASM__
      warn pdc
    ENDIF
% extrn		$$$nam : $$$typ
  endm
endm
.call		macro	procname
IF __LARGE_CODE__
		push	cs
ENDIF
%		call	near ptr procname
endm
.defcode macro sname,nogroup
  IFDEF _cseg
% .codeseg <_cseg>,byte,1,<nogroup>
  ELSE
% .codeseg <sname>
  ENDIF
endm
.codeseg	macro	sname,align,forceseg,nogroup
  IFB <sname>
% .codeseg	<@filename>,sname
    exitm
  ELSEIFIDNI <sname>,<byte>
% .codeseg	<@filename>,sname
    exitm
  ELSEIFIDNI <sname>,<word>
% .codeseg	<@filename>,sname
    exitm
  ELSEIFIDNI <sname>,<dword>
% .codeseg	<@filename>,sname
    exitm
  ELSEIFIDNI <sname>,<flat>
% .codeseg	<@filename>,sname
    exitm
  ELSEIFIDNI <sname>,<para>
% .codeseg	<@filename>,sname
    exitm
  ELSEIFIDNI <sname>,<page>
% .codeseg	<@filename>,sname
    exitm
  ENDIF
  IFNB <align>
$CODE_ALIGN	equ	<align>
  ENDIF
  IF __FLAT_CODE__
% CODE32        segment $CODE_ALIGN USE32 public 'CODE'
@CODESEG	equ	<CODE32>
  ELSEIF __SMALL_CODE__
    IF __TINY__
      IFB <forceseg>
% _TEXT		segment $CODE_ALIGN public 'CODE'
        IFB <nogroup>
DGROUP		group	_TEXT
@DATASEG	equ	<DGROUP>
        ENDIF
@CODESEG	equ	<_TEXT>
@CODESEG&sname	equ	<_TEXT>
      ELSE
% sname&_TEXT	segment $CODE_ALIGN public 'CODE'
        IFB <nogroup>
% sname&_DGROUP	group	sname&_TEXT
@DATASEG	equ     <sname&_DGROUP>
        ENDIF
@CODESEG	equ	<sname&_TEXT>
@CODESEG&sname	equ	<sname&_TEXT>
      ENDIF
    ELSE
      IFB <forceseg>
% _TEXT		segment $CODE_ALIGN public 'CODE'
@CODESEG	equ	<_TEXT>
@CODESEG&sname	equ	<_TEXT>
      ELSE
% sname&_TEXT	segment $CODE_ALIGN public 'CODE'
@CODESEG	equ	<sname&_TEXT>
@CODESEG&sname	equ	<sname&_TEXT>
      ENDIF
    ENDIF
  ELSEIFNB <sname>
%   IFIDNI <sname>,<lib>
% LIB_TEXT	segment $CODE_ALIGN public 'CODE'
@CODESEG	equ	<LIB_TEXT>
@CODESEGLIB	equ	<LIB_TEXT>
    ELSE
% sname&_TEXT	segment $CODE_ALIGN public 'CODE'
@CODESEG	equ	<sname&_TEXT>
@CODESEG&sname	equ	<sname&_TEXT>
    ENDIF
  ELSE
% .codeseg	@filename
  ENDIF
% assume	cs:@codeseg
endm
.setname	macro n0,n1,n2,n3,n4,n5,n6,n7,n8,n9
  irp $name,<n0,n1,n2,n3,n4,n5,n6,n7,n8,n9>
    IFB <$name>
      exitm
    ENDIF
    IF2
      IFDEF _PREFIX
%       IFDIFI <_PREFIX>,<>
%         $name catstr <_PREFIX>,<$name>
        ENDIF
      ENDIF
    ENDIF
  endm
endm
.extrnx		macro	e0
    $$$pos instr <e0>,<:>
    $$$typ substr <e0>,($$$pos+1)
    $$$nam substr <e0>,1,($$$pos-1)
    IF1
      IFDEF _PREFIX
%       IFDIFI <_PREFIX>,<>
%         $$$nam catstr <_PREFIX>,<$$$nam>
        ENDIF
      ENDIF
    ENDIF
% extrn		$$$nam:$$$typ
endm
