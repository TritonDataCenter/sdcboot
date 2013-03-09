

;  Permission is hereby granted, free of charge, to any person obtaining a copy
;  of this software and associated documentation files (the "Software"), to deal
;  in the Software without restriction, including without limitation the rights
;  to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
;  copies of the Software, and to permit persons to whom the Software is
;  furnished to do so, subject to the following conditions:
;
;  (1) The above copyright notice and this permission notice shall be included in all
;  copies or substantial portions of the Software.
;
;  (2) The Software, or any portion of it, may not be compiled for use on any
;  operating system OTHER than FreeDOS without written permission from Rex Conn
;  <rconn@jpsoft.com>
;
;  (3) The Software, or any portion of it, may not be used in any commercial
;  product without written permission from Rex Conn <rconn@jpsoft.com>
;
;  (4) THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
;  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
;  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
;  AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
;  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
;  OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
;  SOFTWARE.


          title     4DLINIT - Initialize low-memory loader & server
          ;
          page      ,132                ;make wide listing
          ;
          comment   }

          Copyright 1988 - 1999, J.P. Software, All Rights Reserved

          Author:  Tom Rawson  11/17/88
                   Partially rewritten 12/89 - 2/90
                   Modified substantially for single EXE file 11/90
                   Completely reorganized 6/91

          This routine initializes 4DOS.  Information it obtains is
          stored in global variables defined in 4DOS.ASM, or is stored in
          the transient portion in global variables defined in SERVER.ASM.
          This routine is separate from 4DOS.ASM so it can be thrown away
          to reduce memory requirements once its work is done.

          } end description
          ;
; =========================================================================
          ;
          ;
          ; Includes
          ;
          include   product.asm         ;product / platform definitions
          include   trmac.asm           ;general macros
          .cseg     LOAD                ;set loader segment if not defined
                                        ;  externally
          include   model.inc           ;Spontaneous Assembly memory models
          include   4dlparms.asm        ;loader / server parameters
          include   4dlstruc.asm        ;loader / server data structures
          include   inistruc.asm        ;INI file structures
          include   keyvalue.asm        ;key names
          ;
          ;
          ; Debugging macros
          ;
Status    macro     level,code
;          local     tptr, skiploc       ;;local vars
;          if        DEBUG ge 2          ;;only assemble in debugging mode
;          push      cs:tptr             ;;save message address
;          jmp       short skiploc       ;;skip around message
;tptr      dw        offset $+2          ;;point to message
;          db        level               ;;store level
;          db        mtext               ;;store message text
;          db        "$"                 ;;end of message
;skiploc:  call      DebugMsg            ;;call message display
;          else                          ;;if not debugging do short version
          push      ax                  ;;save AX
$$C1      =         ((code shr 8) - 20h) and 03Fh               ;;first char
$$C2      =         (((code and 0FFh) - 20h) and 03Fh) shl 6    ;;second
          mov       ax,(level shl 12) + $$C1 + $$C2             ;;make code
          call      DebugMsg            ;;display the message
;          endif                         ;;only assemble in debugging mode
          endm                          ;;that's all
          ;
          ;
          ; Public labels
          ;
          public    Init
          public    NextTokP, TokListP, KeyParsP
          ;
          ; External procedure references
          ;
          ;         Set external declarations for routines which are
          ;         assembled for both loader and transient portion.
          ;
          .extrn    DosUMB:near, Reloc:near, ErrMsg:far
          ;
          ;         Standard external declarations
          ;
          extrn     Startup:near        ;force link of startup code
          extrn     FindEMS:near, FindUReg:near
          extrn     INIParse:near, INIClear:near, INILine:near, INIStr:near
          extrn     NextTok:far, TokList:far, __KeyParse:far
          extrn     SrchEnv:near
          extrn     FNCheck:near, FNorm:near, FNType:near, AZLen:near
          extrn     DecOutBU:near, DecOutWU:near, DecOutW:near
          extrn     InitErrs:byte
          ;
          ; External declarations for Spontaneous Assembly routines
          ;
          .extrnx   MEM_CPY:near
          .extrnx   MEM_MOV:near
          .extrn    GET_CHR:near, PUT_NEWLINE:near, PUT_STR:near
          .extrn    STR_CSPN:near, STR_LEN:near, STR_PBRKN:near
          ;
          ; External references to 4DOS.ASM code and data
          ;
          ;         Variables defined in LOADDATA.ASM (INCLUDed in 4DOS.ASM)
          ;
          extrn     LocStack:near, StackTop:near, ShellNum:byte
          extrn     LMFlags:byte, LMFlag2:byte, LMHandle:word
          extrn     DiskSig:byte, DSigTime:byte, OldInt:word, PSPSeg:word
          extrn     ExecSig:byte, ServLenW:word, ServChk:dword, ReOpenP:word
          extrn     ShellInf:byte, CodeReg:byte, CodeDev:byte, Err24Hdl:word
          ;
          ;         Variables defined in 4DOS.ASM
          ;
          extrn     Prompt:byte
          extrn     ServFake:word, ServSeg:word, ServBrk:word, ServI2E:word
          extrn     SigBuf:byte, TPLenP:word, SwapInP:word
          extrn     RODP:word, ROAccess:byte
          extrn     ShellDP:word, ShellPtr:word
          extrn     CMIPtr:word, CMHPtr:word, PEPtr:word
          extrn     EMOffset:word, EMSeg:word
          extrn     FrameSeg:word, XMSDrvr:word, XMSMove:byte
          extrn     IntServ:near, LMExec:near, ExecExit:near
          extrn     Int23:near, Int24:near
          extrn     ResErrP:word
          ;
          ;         Variables defined in MODULE.ASM
          ;
          extrn     TempStor:near, ModArea:near, ModGuard:near, BigTOS:near
          ;
          ;
          ; "Slider" parameters
          ;
SlStkLen  equ       256                 ;slider stack length
SlStkTop  equ       SlStkLen            ;stack at beginning of slider seg
SlideLoc  equ       SlStkTop            ;put code above stack
SlideLen  equ       (SlCodLen + 1) shr 1  ;total code length in words
SlidePar  equ       ((SlideLen + 7) shr 3) + ((SlStkLen + 0Fh) shr 4)
                                        ;paragraphs in slider segment
          ;
          ; Other segments accessed, with external declarations
          ;
SERV_TEXT segment   para public 'CODE'  ;transient portion server segment
          extrn     SwapOne:far
          extrn     SDataLoc:byte, SFBreak:far, I23Break:far, I2EHdlr:far
          extrn     SLocDev:byte, SLocCDev:byte
SERV_TEXT ends
          ;
_TEXT     segment   para public 'CODE'  ;transient portion text segment
          extrn     __astart:far
_TEXT     ends
          ;
ASM_TEXT  segment   para public 'CODE'  ;transient portion text segment
          extrn     SetUReg:far, FreeUReg:far
ASM_TEXT  ends
          ;
_DATA     segment   para public 'DATA'  ;transient portion data segment
          extrn     _END:far, _edata:far, _STKHQQ:far
          extrn     _gszCmdline:far, _gszFindDesc:far
_DATA     ends
          ;
STACK     segment   para stack 'STACK'  ;transient portion stack segment
STACK     ends
          ;
DGROUP    group     _DATA, STACK        ;transient portion DS base
          ;
          ;
          .defcode  ,1                  ;set up code segment, no DGROUP
          ;
          assume    cs:@curseg, ds:nothing, es:nothing, ss:nothing
          ;
          page
          ;
          ;
          errset    ErrHandl            ;set error handler name
          ;
; =========================================================================
          ;
          ;
          ; Initialization data
          ;
          ;         First include data which will be moved to the server
          ;         segment.  Once this data is moved it is used by the
          ;         server in high memory; the copy here is thrown away.
          ;
INSERVER  equ       0                   ;show we are not in server
          include   servdata.asm
          ;
          ; ------- Data which is local to this source file -------
          ;
DBLevel   db        -1                  ;debug message level
InitFlg2  db        0                   ;initialization flags byte 2 (byte 1,
                                        ;  InitFlag, is in SERVDATA.ASM)
CurDrive  db        ?                   ;current disk drive
CurDisk   db        ?                   ;current disk number (0 = A, ...)
MaxDrive  db        ?                   ;maximum drive allowed (0 = A, ...)
BootDrv   db        ?                   ;boot drive with ":\", null-term.
          db        ":\", 0
SwitChar  db        '/'                 ;DOS switch character
PrvShNum	db	0		;previous shell's shell number
DVVer     dw        0                   ;DESQView version
WinMode   db        WINNONE             ;Windows flag
;NovVer    dw        0                   ;Novell Netware version
TXStart   dw        ?                   ;tail start offset without whitespace
TXLen     dw        ?                   ;tail length without whitespace
SHScnLim  dw        ?                   ;SHELL= scan length limit
CSPecLen  dw        0                   ;COMSPEC length
TailPtr	dd	?		;pointer to passed command tail
StackSeg  dw        ?                   ;initial stack segment for C code
StackOff  dw        ?                   ;initial stack offset for C code
NewStack  dw        ?                   ;new transient stack size
SavStack  dw        ?                   ;saved local stack
OldEnvSz  dw        ?                   ;old environment size, bytes
DREnvLen  dw        0                   ;DR-DOS passed env length, bytes
DREnvLoc  dw        ?                   ;DR-DOS passed env segment
          ;
LLRetry	db	0		;retry flag for LenLoc
ExtraPar  dw        ?                   ;extra paragraphs for data added to
                                        ;  data segment
          ;
ResSize   dw        ?                   ;resident loader paragraphs
ServLenP  dw        ?                   ;transient server paragraph count
CodeLenP  dw        ?                   ;transient code paragraph count
DataLenP  dw        ?                   ;transient DGROUP paragraph count
DataLenB  dw        ?                   ;transient DGROUP byte count
INILenB   dw        ?                   ;INI file data length in bytes
TranMovP  dw        ?                   ;transient move paragraph count
SwapLenP  dw        ?                   ;swap size in paragraphs
SwapLenK  dw        ?                   ;swap size in K
          ;
CodeOffP  dw        ?                   ;code segment offset within transient
DataOffP  dw        ?                   ;data segment offset within transient
INIOffP   dw        ?                   ;INI file data offset within DGROUP,
                                        ;  paragraphs
          ;
TranAddr  dw        ?                   ;transient portion initial load addr
THighAdr  dw        ?                   ;transient portion high address
TranSeg   dw        ?                   ;transient portion final address
TranCur   dw        ?                   ;transient portion current address
TranDCur  dw        ?                   ;transient portion data seg current
                                        ;  address
TCLoc     dd        ?                   ;location of routine for TranCall
TCRet     dw        ?                   ;saved return address for TranCall
TCAX      dw        ?                   ;saved AX for TranCall
LAHSeg    dw        ?                   ;current local alias / func / cmd history /
                                        ;  dir history, DGROUP-relative seg
          ;
LRPtr     dw        offset LRList       ;pointer for low-memory reserve list
ShiftAdd  dw        SUM8                ;checksum shift and add counts
SwapPLen  dw        ?                   ;length of full swap name in SwapPath
Int2EPtr  dw        ?                   ;address where INT 2E code was loaded
CritPtr   dw        ?                   ;address where critical error text
                                        ;  was loaded
NWCSPtr   dw        ?                   ;address where Netware names were
                                        ;  loaded
          ;
NextTokP  dw        offset _TEXT:NextTok  ;offset of next token routine
          dw        ?                   ;segment
TokListP  dw        offset _TEXT:TokList  ;offset of token list routine
          dw        ?                   ;segment
KeyParsP  dw        offset _TEXT:__KeyParse  ;offset of key parsing routine
          dw        ?                   ;segment
          ;
IEntry    farptr    <>                  ;4DOS initial entry point
          ;
INIKLen   dw        ?                   ;total length of INI file keys
IFData    INIData   <offset INIStDat>   ;INI file data, first item is pointer
                                        ;  to strings area
          ;
AlsData   AHData    <>                  ;collected alias list data
FuncData  AHData    <>                  ;collected function list data
HistData  AHData    <>                  ;collected cmd history list data
DirData   AHData    <>                  ;collected dir history list data
          ;
          ;         Equivalent names for some INI file data items
          ;
SwMethod  equ       IFData.I_SwMeth     ;swap method
EnvFree   equ       IFData.I_EnvFr      ;minimum free environment, bytes
StkSize   equ       IFData.I_Stack      ;stack size
ShLevel   equ       IFData.I_ShLev      ;shell level
          ;
          ;         Bits in ResUMB status byte
          ;
RUREGERR  equ       80h                 ;region error
RUDOSUMB  equ       40h                 ;DOS UMB reserved
          ;
          ;         CAUTION:  The following three blocks are copied from one
          ;         shell to another in a single copy operation, and hence
          ;         must appear together and in this order.
          ;
OldShell  ShellDat  <>                  ;previous shell's shell info
OldCReg   RegSCB    <>                  ;previous shell's code swap region
OldCDev   DevSCB    <>                  ;previous shell's code swap device
          ;
CodeTemp  RegSCB    <>                  ;region SCB for temporary code swap
                                        ;  out and checksum
InhReg    RegSCB    <>                  ;region SCB for inheriting INI,
                                        ;  aliases, cmd history, and dir
                                        ;  history
InhDev    DevSCB    <>                  ;device SCB for inheritance
          ;
          ;
          ; ------- Local I/O buffers -------
          ;
CSPath    path      <>                  ;path from COMSPEC
TempPath  db        TAILMAX dup (?)     ;generic temporary path / filename /
                                        ;  parameter buffer
;TailBuf   db        TAILMAX dup (?)     ;buffer for command tails
          ;
INIStDat  db        INISMAX dup (?)     ;area for strings from INI file
INIDKCnt  equ       4                   ;3 default key mappings
INIKeys   dw        K_CtlBS             ;Ctrl-Backspace for delete word right
          dw        K_Tab               ;Tab for next file name
          dw        K_ShfTab            ;Shift-Tab for prev file name
          dw        K_CtlTab		;^Tab for popup file name
          dw        K_CtlR + (MAP_GEN shl 8)  ;Ctrl-R, del word rt, general
          dw        K_F9 + (MAP_EDIT shl 8)   ;F9, next file, editing
          dw        K_F8 + (MAP_EDIT shl 8)   ;F8, prev file, editing
          dw        K_F7 + (MAP_EDIT shl 8)   ;F7, popup file name
          dw        (2 * INIKMAX - 8) dup (?)  ;keystroke substitution table
          ;
          ;
          ; ------- Local string constants -------
          ;
RelocMrk  db        "ENDP"              ;mark before relocation table
RlMrkLen  equ       $ - RelocMrk        ;length of mark
          ;
ShPath    db        "x:\"               ;path for default Netware COMSPEC
ShName    db        "4DOS.COM"          ;name to look for in SHELL=
ShNameL   equ       $ - ShName          ;length of above
ShPathL   equ       $ - ShPath          ;length of above with path
          ;
ComSpec   db        "COMSPEC"           ;environment string name
CmdLine	db        "CMDLINE"           ;environment string name
AutoExec  db        "x:\AUTOEXEC.BAT", 0  ;AUTOEXEC.BAT name
AutoLen   equ       $ - AutoExec        ;length of above with terminator
          ;
AEXDir    db        "AutoExecPath="     ;directive used by /A and /P
AEXDLen   equ       $ - AEXDir
          ;
AEXParm   db        "AutoExecParms="    ;directive used by /A and /P
AEXPLen   equ       $ - AEXParm
          ;
AEBreak   db        " ", TAB, " ", 0    ;break table for /A and /P parsing
                                        ;  (second space is replaced by '"'
                                        ;  when appropriate)
          ;
SwDeflt   db        "EMS XMS "          ;try XMS, then EMS
SwDefD    db        " :\", 0            ;then disk (drive gets filled in, or
                                        ;  set to null on floppy systems to
                                        ;  terminate string at this point)
SwapBrk   db        " ,;", 0            ;break table for parsing Swapping=
          ;
SwapTLst  db        3, 0                ;3 items, no fixed bytes
          DefStr    "EMS"               ;EMS name
          DefStr    "XMS"               ;XMS name
          DefStr    "None"              ;no swapping name
          ;
EMSName   db        "4DOS:000"          ;EMS block name
          ;
SwapName  db        "4DOSSWAP.000"      ;swap file name
SwapNL    equ       $ - SwapName        ;swap file name length
          ;
USwExt    db        ".4SW"              ;extension for unique swap file name
USwExtL   equ       $ - USwExt          ;length of extension
          ;
KSwName1  db        "4DOSSWAP.*"        ;first swap file name to kill
KSw1Len   equ       $ - KSwName1        ;length
          ;
KSwName2  db        "*.4SW"             ;second swap file name to kill
KSw2Len   equ       $ - KSwName2        ;length
          ;
	if	_RT
ININame   db        "4DOSRT.INI",0      ;runtime INI file name
	else
ININame   db        "4DOS.INI",0        ;INI file name
	endif
ININamL   equ       $ - ININame         ;name length including null
          ;
WhiteLst  db        ' ', TAB, CR, LF    ;white space list
WhiteCnt  equ       ($ - WhiteLst)      ;length of WhiteLst
          db        0                   ;terminate table so we can use it for
                                        ;  STR_CSPN and STR_PBRKN
          ;
NNData    path      <>                  ;COMSPEC value for Netware to clobber
          db        "_:/", 0, "_______.___", 0  ;pipe name 1
          db        "_:/", 0, "_______.___", 0  ;pipe name 2
NNLen     equ       $ - NNData          ;length of Netware names data
          ;
          ;
          ; ------- Local messages -------
          ;
_MsgType  equ       1                   ;assemble initialization messages
          ifdef     ENGLISH
          include   4DLMSG.ASM
          endif
          ;
          ifdef     GERMAN
          include   4DLMSG.ASD
          endif
          ;
          ;
          ; ------- Internal tables --------
          ;
          ;         Loadable module table
          ;
ModCount  =         0                   ;module count, will be incremented
                                        ;  by DefMod macro
          ;
ModTable  label     byte                ;start of the table
          DefMod    XMS, SwapInP        ;XMS swap in
          DefMod    EMS, SwapInP        ;EMS swap in
          DefMod    Disk, SwapInP       ;disk swap in
          DefMod    RODat, RODP         ;swap file reopen data
          DefMod    ReOpn, ReOpenP      ;disk swap file reopen for Netware
          DefMod    ShDat, ShellDP      ;shell table data
          DefMod    ShMan, ShellPtr     ;shell number manager
          DefMod    I2ES, Int2EPtr      ;standard INT 2E handler
          DefMod    I2EF, Int2EPtr      ;full INT 2E handler
          DefMod    Crit, CritPtr       ;critical error text
          DefMod    CMIni, CMIPtr       ;COMMAND.COM message handler init
          DefMod    CMHdl, CMHPtr       ;COMMAND.COM message handler
          DefMod    PErr, PEPtr         ;parse error table for CC msg hdlr
          ;
          ;         INI file bit data table
          ;
          ;
IBitCnt   =         0                   ;flag count, will be incremented
                                        ;  by INIBit macro
IBitTab   label     byte                ;start table
          IniBit    TPARES,0,Flags,TPARes         ;ReserveTPA
          IniBit    NOREDUCE,<IBREV+IBNOCLR>,Flags,Reduce  ;Reduce (reversed)
          IniBit    AUTOFAIL,IBNOCLR,LMFlag2,CFail  ;CritFail
          IniBit    XINHERIT,<IBREV+IBNOCLR>,InitFlag,Inhrit  ;Inherit (rev.)
          IniBit    NOCCMSG,IBREV,LMFlag2,CCMsg   ;MessageServer (reversed)
          IniBit    DREOPEN,0,LMFlag2,Reopen      ;SwapReopen
          IniBit    DVCLEAN,0,LMFlag2,DVCln       ;DVCleanup
          IniBit    LOADUMB,0,Flags,UMBLd         ;UMBLoad
          ;
          ;
          ;
          ; END of local data
          ;
; =========================================================================
          ;
          ;
          ; This code is entered by 4DOSTART after all resident and
          ; initialization code and data has been moved down and CS
          ; has been reset to point to the PSP.  On entry DS is undefined,
          ; and ES points to the PSP (= CS).  See 4DOSTART for details.
          ;
          ; Set up segment registers and initialize loader stack
          ;
Init:     
          if        DEBUG               ;;only assemble in debugging mode
          jmp       $$001
$dump     db        "After move   CS="
$CSVal    db        6 dup (' ')
          db        "DS="
$DSVal    db        6 dup (' ')
          db        "ES="
$ESVal    db        6 dup (' ')
          db        "Env="
$EnvVal   db        6 dup (' ')
          db        CR, LF, '$'
$$001:
  extrn     HexOutW:near
  pushm     ax,bx,cx,dx,ds,es
  mov       bx,ds
  mov       cx,es
  mov       dx,es:[PSP_ENV]
  loadseg   ds,cs
  loadseg   es,cs
  mov       di,offset $CSVal
  mov       ax,cs
  call      HexOutW
  mov       di,offset $DSVal
  mov       ax,bx
  call      HexOutW
  mov       di,offset $ESVal
  mov       ax,cx
  call      HexOutW
  mov       di,offset $EnvVal
  mov       ax,dx
  call      HexOutW
  mov       dx,offset $dump
  calldos   MESSAGE
  popm      es,ds,dx,cx,bx,ax

          endif

          mov       ax,cs               ;copy code segment
          mov       ds,ax               ;data seg = code seg
          assume    cs:@curseg, ds:@curseg  ;fix assumes
          mov       StackSeg,ss         ;save SS
          mov       StackOff,sp         ;save SP
          cli                           ;hold interrupts
          mov       ss,ax               ;stack = same as everything else
          mov       sp,offset StackTop  ;set local stack
          sti                           ;allow interrupts
          mov       EMSeg,cs            ;set segment for any ErrMsg call
                                        ;  during initialization
          ;
          ; Save PSP information
          ;
          mov       PSPSeg,es           ;save our PSP segment
          mov       SrvPSP,es           ;save for server also
          ;
          call      ChkDebug            ;check for debugging switch, set
                                        ;  debug level
          ;
          ; Fill stack with a constant.  Since the stack is the first thing
          ; after the PSP this will satisfy programs like PCED that use this
          ; area to compare to the primary shell and see if another copy of
          ; the command interpreter is loaded.  Of course this only works if
          ; the stack doesn't get close to full, but it probably doesn't.
          ; Note this code assumes the stack is empty when it runs!
          ;
FillStk:
          Status    0,'IS'              ;(Initialize stack & variables)
          mov       di,offset LocStack  ;point to stack
          loadseg   es,cs               ;point to local segment
          mov       cx,(STACKSIZ / 2)   ;get stack size in words
          mov       ax,4D4Dh            ;get stack fill constant
          cld                           ;go forward
          rep       stosw               ;fill stack
          ;
          ; Initialize variables
          ;
          and       InitFlag,DBMSG      ;clear init flags except debugging
          mov       LMSeg,cs            ;set our segment
          calldos   GETDRV              ;get current disk drive
          mov       dl,al               ;copy to dl for SETDRV call
          mov       CurDisk,al          ;save for later
          add       al,'A'              ;make it ASCII
          mov       CurDrive,al         ;save drive
          calldos   SETDRV              ;set drive (no-op, returns max drive)
          mov       MaxDrive,al         ;save max drive
          calldos   VERSION             ;get DOS version
          mov       DosMajor,al         ;save major version
          mov       DosMinor,ah         ;and minor
          ;
          mov       dl,CurDrive         ;get current drive
          cmp       al,4                ;DOS 4 or above?
           jb       SaveBoot            ;no, use current drive
          mov       ax,03305h           ;undoc DOS 4+ call to get boot drive
          calldos                       ;get it
          add       dl,('A' - 1)        ;make it alpha
          ;
SaveBoot: mov       BootDrv,dl          ;save boot drive
          ;
          mov       si,SERV_TEXT        ;get server addr = start of transient
                                        ;  portion
          mov       TranCur,si          ;save it (used by TranCall)
          mov       ax,_TEXT            ;get segment for key parsing and
                                        ;  token routines
          mov       NextTokP+2,ax       ;save for NextTok call
          mov       TokListP+2,ax       ;save for TokList call
          mov       KeyParsP+2,ax       ;save for KeyParse call
          ;
          mov       ax,ds:[PSP_ENV]     ;get passed environment segment
          mov       PassEnv,ax          ;save it
          or        ax,ax               ;anything there?
           jz        SetRoot            ;if not we are the root shell
          cmp       DosMajor,6          ;DOS 6 or above?
           jb       CheckEnv		;if not go on (not root shell)
          cmp       DosMajor,10         ;OS/2?
           jae      CheckEnv		;if so go on
          ;
          ; DOS 6 or above -- passed environment is not reliable, use memory
          ; map to check for root shell
          ;
          calldos   LOL                 ;get list of lists address
          mov       ax,es:[bx-2]        ;get address of first MCB
          mov       bx,PSPSeg           ;get PSP segment
          dec       bx                  ;get our MCB segment
          ;
MCBLoop:  cmp       ax,bx               ;up to our MCB?
           jae      SetRoot             ;if so, we are root shell
          mov       es,ax               ;move to next MCB
          mov       cx,wptr es:[1]      ;get owner field
           jcxz     NextMCB             ;if free skip it
          cmp       cx,PSPSeg           ;is it ours?
           je       NextMCB             ;if so skip it
          cmp       cx,8                ;owned by DOS?
           jne      CheckEnv		;if not we can't be root, get out
          ;
NextMCB:  inc       ax                  ;skip this MCB
          add       ax,wptr es:[3]      ;skip data to get to next MCB
          jmp       short MCBLoop       ;and keep looking
          ;
SetRoot:  bset      InitFlag,PERMLOAD   ;force /P in primary shell
          bset      LMFlags,ROOTFLAG    ;set root shell flag
          ;
          ; Figure out the size of the passed environment
          ;
CheckEnv:
          Status    0,'EZ'              ;(Environment size)
	mov       ax,PassEnv          ;get our environment segment
          or        ax,ax               ;check if any environment
           jz       ClosHand		;no, go on
          mov       es,ax               ;set environment segment
          xor       di,di               ;start at beginning
          mov       cx,0FFF0h           ;scan the whole segment (almost)
          xor       al,al               ;clear byte for scan
          cld                           ;go forward
          ;
ScanEnv:  repne     scasb               ;scan for end of one string
           jne      SetELen		;if segment is full give up
          cmp       bptr es:[di],0      ;two nulls in a row?
           jne      ScanEnv             ;if not keep scanning
          inc       di                  ;bump for real size
	;
SetELen:	mov	OldEnvSz,di	;save passed environment size
          ;
ClosHand: call      CloseHdl            ;close passed file handles
          ;
          Status    0,'FU'              ;(Find UMB regions)
          mov       cx,MAXREG           ;get max number of regions
          mov       si,offset URInfo    ;get region table address
          call      FindUReg            ;call it
          mov       URCount,ax          ;save count
          ;
          Status    0,'GP'              ;(Get info from parent)
          call      PrvShell            ;get info from previous shell
          ;
          Status    0,'CK'              ;(Check system info)
          call      CheckXMS            ;check for XMS
          call      CheckEMS            ;check for EMS
          call      CheckWin            ;check for Windows
          call      CheckDV             ;check for DESQView
;          call      CheckNov            ;check for Novell Netware
          call      Find4DOS		;find our location
          ;
          Status    0,'IT'              ;(Process INI file & command tail)
          call      InitINI             ;initialize INI data and load any
                                        ;  passed data
          call      PrevINI             ;process any INI file passed from
                                        ;  previous shell
          ;
DoTail:   call      ParsTail            ;parse the command tail, process the
                                        ;  default INI file in the primary
                                        ;  shell
          call      FixupINI            ;fix up INI data
          ;
          call      SetupNN             ;set up Netware Names
          ;
DoLists:  Status    0,'AJ'              ;(Adjust alias / history / environment)
          call      AdjLists            ;adjust environment, alias, func, cmd
                                        ;  history, and dir history sizes
          ;
          Status    0,'ID'              ;(Set up initialization data)
          call      LenLoc              ;set up transient portion lengths
                                        ;  and locations
	 jnc	GReloc		;continue if all OK
	 			;if not, data segment is too large
          mov       IFData.I_Alias,ALSDEF    ;force aliases to default
          mov       IFData.I_Func,ALSDEF    ;force functions to default
          mov       IFData.I_Hist,HISTDEF    ;force history to default
          mov       IFData.I_DHist,DHISTDEF  ;force directory history to default
	bset	InitFlag,XINHERIT	;kill inheritance
	jmp	DoLists		;and try again
	;
GReloc:
	call      GetReloc            ;get relocation table
          ;
          Status    0,'SW'              ;(Set up swapping)
          call      SwapSet             ;set up swapping
          cmp       SwMethod,SWAPNONE   ;are we swapping?
           jne      DoSwap              ;if so go on
          call      LoadRes             ;not swapping, load resident (never
                                        ;  returns)
          ;
DoSwap:
          ;
          Status    2,'RS'              ;(Relocate for swapping)
          mov       ax,THighAdr         ;get new load segment for adjustment
          mov       OldRBase,ax         ;save as our relocation base (will
                                        ;  be modified by swap control block
                                        ;  setup code if we are using reduced
                                        ;  swapping)
          call      DoReloc             ;do the relocation
          ;
          ;
          Status    2,'DV'              ;(Check DESQView mode)
          cmp       DVVer,0             ;check DV version
           je       ChkWin              ;if not there go on
          mov       dx,offset DVMsg     ;point to DV message
          call      StatMsg             ;display DV message
          ;
ChkWin:
          Status    2,'WN'              ;(Check Windows mode)
          cmp       WinMode,WINNONE     ;Windows missing?
           je       DoDREnv             ;if so go on
          mov       dx,offset WinOMsg   ;point to OS/2 message
          cmp       WinMode,WINOS2      ;OS/2 mode?
           je       WinDisp             ;if so display message
          mov       dx,offset WinEMsg   ;point to enhanced mode message
          cmp       WinMode,WIN386      ;386 enhanced mode?
           je       WinDisp             ;if so display message
          cmp       WinMode,WIN95	;Windows 95/98?
           jl       WinStRl		;if not,go on
          mov       dx,offset Win9XMsg  ;point to Win9X message
	call      StatMsg             ;display it
          mov       dx,offset Win95Msg  ;point to Win95 message
          cmp       WinMode,WIN95	;Windows 95?
           je       WinDisp		;if so display it
          mov       dx,offset Win98Msg  ;point to Win98 message
          cmp       WinMode,WIN98	;Windows 98?
           je       WinDisp		;if so display it
          mov       dx,offset WinMEMsg  ;point to WinME message
	jmp	WinDisp		;display that
	;
WinStRl:	bset      Flags,TPARES        ;must be real or standard, force
                                        ;  reserve of high memory
          mov       dx,offset WinSMsg   ;point to std / real mode message
          ;
WinDisp:  call      StatMsg             ;display Windows message
	test	InitFlg2,W95DSWP	;disk swap inh killed by Win95?
	 jz	DoDREnv		;if not go on
          mov       dx,offset W95DSMsg  ;point to Win95 disk swap message
	calldos	MESSAGE		;display error
          ;
DoDREnv:  call      GetDREnv            ;get any DR-DOS env variables and
                                        ;  copy to temporary storage
          ;
          Status    0,'MT'              ;(Move transient portion)
          call      MoveTran            ;move the transient portion to its
                                        ;  final address
          ;
          call      MvDREnv             ;move DR-DOS variables to transient
          ;
          call      LoadMod             ;set up loadable modules
          ;
          Status    0,'SC'              ;(Set up control blocks)
          call      SetupSCB            ;set up swap control blocks
          call      SetupICB            ;set up inheritance control blocks
          ;
          Status    0,'MF'              ;(Move code & data to final locations)
          test      Flags,LOADUMB       ;UMB load requested?
           jz       SetupDB             ;if not go on
          call      MoveUMB             ;move resident portion to UMB
          ;
SetupDB:  call      DataBlks            ;set up data blocks (UMBs or low
                                        ;  memory requests for env,aliases,
                                        ;  functions, cmd history, dir history)
          ;
DoChains: call      SetChain            ;set up the chains etc.
          mov       ax,TranSeg          ;get segment for server data
          call      InitTran            ;initialize transient portion
          ;
          ; Switch to transient stack as Netware Names may kill part of our
          ; stack
          ;
          cli                           ;hold interrupts
          mov       ss,TDataSeg         ;switch to transient portion's stack
          mov       sp,StackOff         ;... and set SP
          sti                           ;allow interrupts
          ;
          ; Copy Netware Names to low memory
          ;
          Status    1,'NC'              ;(Copy Netware Names)
          cmp       IFData.I_NName,0    ;load Netware dummy names?
           je       TranRdy             ;if not go on
          mov       di,ResSize          ;get resident size (paragraphs)
          shln      di,4,cl             ;make it bytes to get xfer offset
          mov       cx,NNLen            ;get Netware Names module length
          loadseg   es,ds               ;set destination segment
          cld                           ;go forward
          mov       si,offset NNData    ;get start address
          rep       movsb               ;copy names to end of low memory area
          add       di,0Fh              ;bump final offset for roundoff
          shrn      di,4,cl             ;make it a paragraph count
          mov       ResSize,di          ;increase resident size
          ;
          ; Clean things up and start transient code
          ;
TranRdy:  Status    1,'RR'              ;(Reduce resident memory)
          mov       bx,ResSize          ;get resident size
          loadseg   es,cs               ;get PSP address
          calldos   REALLOC,MEMDA       ;reduce memory usage
          ;
          ;
          ; NOTE:  Code below this point is running in unreserved memory!
          ;
          ; Set up registers and start 4DOS
          ;
          Status    0,'ST'              ;(Start transient portion",CR,L)
          cli                           ;hold interrupts
          push      TCodeSeg            ;save startup segment for RETF
          mov       ax,offset _TEXT:__astart  ;get startup code offset
          push      ax                  ;save as RETF offset
          mov       ax,DataLenB         ;get DGROUP length
          loadseg   ds,ss               ;set DS = SS = TDataSeg
          loadseg   es,cs               ;set ES = 4DOS PSP = our psp
          assume    ds:nothing, es:nothing   ;fix assumes
          ;
          public    Go4DOS              ;for debugging
Go4DOS:   retf                          ;start 4DOS
          ;
          ;
          ;
          ; *************************************************************
          ; *                                                           *
          ; *                     Major Subroutines                     *
          ; *                                                           *
          ; *************************************************************
          ;
          ;
          ; =================================================================
          ;
          ;
          ; CHKDEBUG - Check for debugging switch in command tail
          ;
          ; On entry:
          ;         No requirements
          ;
          ; On exit:
          ;         AX, BX, CX, DX, SI, DI destroyed
          ;         Other registers and interrupt state unchanged
          ;
          entry     ChkDebug,noframe,,local  ;check for /Z
          assume    ds:@curseg          ;fix assumes
          ;
          loadseg   es,ds               ;set ES for ChkWhite
          mov       si,PSP_TLEN         ;point to tail length byte
          cld                           ;go forward
          lodsb                         ;load length
          mov       cl,al               ;copy for loop
          xor       ch,ch               ;clear high byte
           jcxz     CDExit              ;if nothing there go on
          ;
DBSLoop:  call      SkpWhite            ;skip white space
           jc       CDExit        	;if all white go on
	cmp       al,'/'              ;slash?
           je       GotSw               ;if so we have switch
          cmp       al,'-'              ;dash?
           jne      CDExit              ;if not go on
          ;
GotSw:    lodsb                         ;get next byte
          call      UpShift             ;make it upper
	cmp	al,'C'		;is it /C?
	 je	CDExit		;if so, ignore rest of tail
	cmp	al,'K'		;is it /K?
	 je	CDExit		;if so, ignore rest of tail
          cmp       al,'R'              ;root shell switch?
           jne      CDCheckZ            ;if not go on
          bset      LMFlags,ROOTFLAG    ;force root shell
          jmp       short DBSLoop       ;if not, loop
          ;
CDCheckZ: cmp       al,'Z'              ;debug switch?
           jne      CDExit              ;if not go on
          call      SetDBLev            ;set debug message level
          ;
CDExit:   exit                          ;all done
          ;
          ; =================================================================
          ;
          ; CLOSEHDL - Close passed file handles
          ;
          ; On entry:
          ;         No requirements
          ;
          ; On exit:
          ;         AX, BX, CX, DX, SI, DI destroyed
          ;         Other registers and interrupt state unchanged
          ;
          entry     CloseHdl,noframe,,local  ;close file handles
          assume    ds:@curseg          ;fix assumes
          ;
          Status    2,'HC'              ;(Close handles)
          mov       cx,15               ;number of handles to close (all in
                                        ;  the standard DOS PSP file handle
                                        ;  table)
          mov       bx,5                ;start with handle 5
          ;
ClsLoop:  calldos   CLOSE               ;close it
          inc       bx                  ;get next handle
          loop      ClsLoop             ;and loop back
          ;
          exit                          ;all done
          ;
          ; =================================================================
          ;
          ; PrvShell - Retrieve information from previous shell
          ;
          ; On entry:
          ;         No requirements
          ;
          ; On exit:
          ;         AX, BX, CX, DX, SI, DI destroyed
          ;         Other registers and interrupt state unchanged
          ;
          entry     PrvShell,noframe,,local  ;get previous shell info
          assume    ds:@curseg          ;fix assumes
          ;
          mov       al,SERVINT          ;get server interrupt number
          calldos   GETINT              ;get current value
          mov       ax,es               ;copy segment
          or        ax,bx               ;segment and offset both 0?
           jz       NoPrvShl            ;if so interrupt not hooked, can't be
                                        ; any previous shell
          ;
          ; Find out if a previous 4DOS shell is there
          ;
          mov       ax,SERVSIG          ;get 4DOS server signature
          mov       bh,SERVINQ          ;get inquiry function
          int       SERVINT             ;call previous shell
          cmp       ax,SERVRSIG         ;did we get back return signature?
           jne      NoPrvShl            ;no, go on -- we are shell zero
          mov       PrevVer,bl          ;yes -- save previous shell's version
          cmp       bl,4                ;4DOS 4.0 or above?
           jae      NewShel4            ;if so do standard processing
          ;
          ; Get a new shell number from 4DOS 3.0 or below
          ;
          mov       ax,SERVSIG          ;get 4DOS server signature
          mov       bh,SERVNEW          ;get 4DOS ver. 2/3 new shell function
          int       SERVINT             ;call previous shell
          mov       ShellNum,bl         ;shell number is in bx
          jmp       short NoPrvShl      ;ignore everything else as previous
                                        ;  shell is not 4DOS 4.0 or above
          ;
          ; Get a new shell number from 4DOS 4.0 or above
          ;
NewShel4:	mov	PrvShNum,dl	;save previous shell's shell number
	push      bx                  ;save old version
          mov       ax,SERVSIG          ;get 4DOS server signature
          mov       bh,SERVSEC          ;get secondary shell function
          xor       bl,bl               ;request a new shell number
          int       SERVINT             ;call previous shell
          mov       ShellNum,bl         ;yes, shell number is in bx
          pop       bx                  ;get back old version
          cmp       bx,VERSION          ;same version?
           jne      NoPrvShl            ;if not get out
          ;
          ; Get previous shell's code segment swap and alias / function /
          ; cmd history / dir history / INI data swap info
          ;
          mov       ax,SERVSIG          ;get 4DOS server signature
          mov       bh,SERVSWAP         ;get swap info function
          int       SERVINT             ;call previous shell
          push      ds                  ;save DS
          loadseg   ds,es               ;point DS to previous shell seg
          pop       es                  ;get back our segment
          moveb     ,bx,,<offset OldShell>,<(size OldShell) + (size OldCReg) + (size OldCDev)>
                                        ;copy previous shell's info
          loadseg   ds,cs               ;reset local segment
          cmp       OldShell.ShIVer,INTVER  ;internal version match?
           jne      NoPrvShl            ;if not, show no previous shell
          ;
          ; Previous shell is there, but we can't use its swap space if it
          ; doesn't have one!
          ;
          cmp       OldShell.DSwpType,SWAPNONE  ;prev shell swap disabled?
           jne      PSExit              ;if not go on
          jmp       short PSNoRed       ;otherwise just kill reduced swap
          ;
          ; No previous shell of this same version -- disable use of
          ; information from previous shell
          ;
NoPrvShl: bset      InitFlag,NOPREV+XINHERIT  ;kill inheritance
          ;
PSNoRed:  bset      Flags,NOREDUCE      ;disable reduced swapping
          ;
          ;
PSExit:   exit                          ;all done
          ;
          ; =================================================================
          ;
          ; CheckXMS - Check XMS availability
          ;
          ; On entry:
          ;         No requirements
          ;
          ; On exit:
          ;         AX, BX, CX, DX, SI, DI destroyed
          ;         Other registers and interrupt state unchanged
          ;
          entry     CheckXMS,noframe,,local  ;check for XMS
          assume    ds:@curseg          ;fix assumes
          ;
          Status    1,'LX'              ;(Look for XMS)
          mov       al,I_XMS            ;get XMS interrupt number
          calldos   GETINT              ;get current value
          mov       ax,es               ;copy segment
          or        ax,bx               ;segment and offset both 0?
           jz       CXExit              ;if so skip XMS test
          ;
          mov       ax,XMSTEST          ;get test value
          int       I_XMS               ;call XMS
          cmp       al,XMSFLAG          ;is XMS installed?
           jne      CXExit              ;if not go on
          Status    2,'FX'              ;(Found XMS)
          mov       ax,XMSADDR          ;get XMS get address value
          int       I_XMS               ;call XMS
          mov       XMSDrvr.foff,bx     ;save offset
          mov       XMSDrvr.fseg,es     ;save segment
          mov       SrvXDrvr.foff,bx    ;save offset for derver
          mov       SrvXDrvr.fseg,es    ;save segment for server
          bset      InitFlag,XMSAVAIL   ;set active bit
          ;
CXExit:   exit                          ;all done
          ;
          ; =================================================================
          ;
          ; CheckEMS - Check EMS availability
          ;
          ; On entry:
          ;         No requirements
          ;
          ; On exit:
          ;         AX, BX, CX, DX, SI, DI destroyed
          ;         Other registers and interrupt state unchanged
          ;
          entry     CheckEMS,noframe,,local  ;check for EMS
          assume    ds:@curseg          ;fix assumes
          ;
          Status    1,'LE'              ;(Look for EMS)
          call      FindEMS             ;is EMS present?
           jc       CEExit              ;if not go on
          Status    2,'FE'              ;(Found EMS)
          mov       FrameSeg,bx         ;save EMS page frame segment
          mov       SrvFrame,bx         ;save for server as well
          bset      InitFlag,EMSAVAIL   ;set active bit
          ;
CEExit:   exit                          ;all done
          ;
          ; =================================================================
          ;
          ; CheckWin - Look for Windows and get its mode
          ;
          ; On entry:
          ;         No requirements
          ;
          ; On exit:
          ;         AX, BX, CX, DX, SI, DI destroyed
          ;         Other registers and interrupt state unchanged
          ;
          entry     CheckWin,noframe,,local  ;check for Windows
          assume    ds:@curseg          ;fix assumes
          ;
          Status    1,'LW'              ;(Look for Windows)
          cmp       DosMajor,30         ;Windows NT?
	 jae	CWExit		;if so, ignore it
	mov       al,I_WIN            ;get windows interrupt (2F)
          calldos   GETINT              ;get current value
          mov       ax,es               ;copy segment
          or        ax,bx               ;segment and offset both 0?
           jz       CWExit              ;if so interrupt not hooked, go on
          mov       ax,W_CHK386         ;see if we are in 386 mode
          int       I_WIN               ;call Windows
          and       al,7Fh              ;isolate right 7 bits
           jz       ChkStd              ;if zero check standard mode
          cmp       al,7Fh              ;Windows 2?
           je       GotWin2             ;if so save that
          cmp       al,1                ;Windows 2?
           je       GotWin2             ;if so save it
          cmp       DosMajor,7          ;maybe Win95?
           jb       GotWin3             ;if not must be Win3 386 enhanced
          cmp       DosMajor,20         ;OS/2 2.0 or later?
           jae      GotWOS2            	;if so go save that
	;
	; Could be Win95 -- if so mark it, and kill any attempt to
	; inherit primary shell disk swap handle
	;
	loadseg	es,ds		;set ES to dummy buffer
	lea	di,TempPath	;point to dummy buffer
	mov	cx,sizeof TempPath	;size of buffer
	lea	dx,BootDrv	;point to "x:\" (seg in DS)
	stc			;assume error
	calldos	95_VOL		;get volume information
	 jc	GotWin3		;must be Win3 on PC-DOS 7
	mov       WinMode,WIN95	;set Win95 mode
	xor	bx,bx		;clear BX
	mov	ax,W_CHK98	;get Win98 test code
	int	I_WIN		;check for Win98
	cmp	bx,040Ah		;is it Win98?
	 jl	RootTest		;if not continue
	mov	WinMode,WIN98	;if so, set the mode
	cmp	bx,045Ah		;is it Win ME?
	 jl	RootTest		;if not continue
	mov	WinMode,WINME	;if so, set the mode
	;
RootTest:	test      LMFlags,ROOTFLAG    ;are we the root shell?
	 jnz	CWExit		;if so we're done
	test	InitFlag,NOPREV	;any previous shell?
	 jz	CkW95DSw
	bset	InitFlg2,W95USWP	;running from the Win95 GUI, no
				;previous shell, so force unique
				;swap name
	jmp	CWExit		;all done
	;
CkW95DSw:	cmp       OldShell.DSwpType,SWAPDISK  ;prev shell swap to disk?
	 jne	CWExit		;if not we're done
          cmp       PrvShNum,0	;prev shell the primary?
	 jne	CWExit		;if not we're done
	bset      InitFlag,NOPREV+XINHERIT  ;kill inheritance
	bset      InitFlg2,W95DSWP	;show disk swap inheritance killed
	bset      Flags,NOREDUCE      ;disable reduced swapping
          jmp       CWExit		;all done
	;
GotWOS2:	mov       WinMode,WINOS2      ;set OS/2 mode
          jmp       short CWExit        ;all done
          ;
GotWin3:  mov       WinMode,WIN386      ;set enhanced mode
          jmp       short CWExit        ;all done
          ;
GotWin2:  mov       WinMode,WIN2        ;set Windows 2 mode
          jmp       short CWExit        ;all done
          ;
ChkStd:   mov       ax,W_CHKSTD         ;see if we are in real / std mode
          int       I_WIN               ;call Windows
          or        ax,ax               ;get zero back?
           jnz      CWExit              ;if not go on
          mov       WinMode,WINSTD      ;set real / standard mode
          ;
CWExit:   exit                          ;all done
          ;
          ; =================================================================
          ;
          ; CheckDV - Check for DESQView
          ;
          ; On entry:
          ;         No requirements
          ;
          ; On exit:
          ;         AX, BX, CX, DX, SI, DI destroyed
          ;         Other registers and interrupt state unchanged
          ;
          entry     CheckDV,noframe,,local  ;check for DESQView
          assume    ds:@curseg          ;fix assumes
          ;
          cmp       DosMajor,10         ;OS/2 or NT?
           jae      CDVExit             ;if so assume no DV
          mov       al,1                ;set AL
          mov       cx,'DE'             ;set CX value for DV
          mov       dx,'SQ'             ;set DX value
          calldos   SETDATE             ;fake a set date call
          inc       al                  ;return 0FFh -> invalid date?
          jz        CDVExit             ;if so get out
          mov       DVVer,bx            ;save DV version
          bset      LMFlag2,INDV        ;set DV flag
          ;
CDVExit:  exit                          ;all done
;          ;
;          ; =================================================================
;          ;
;          ; CheckNov - Check for Novell Netware
;          ;
;          ; On entry:
;          ;         No requirements
;          ;
;          ; On exit:
;          ;         AX, BX, CX, DX, SI, DI destroyed
;          ;         Other registers and interrupt state unchanged
;          ;
;          entry     CheckNov,varframe,,local  ;check for Novell Netware
;          assume    ds:@curseg          ;fix assumes
;          ;
;          var       NovReply,N_RBLEN    ;reply buffer
;          varend
;          ;
;          mov       ax,N_SHVER          ;look for Netware shell version
;          xor       bx,bx               ;BX must be 0
;          lea       di,NovReply         ;get reply buffer address
;          loadseg   es,ss               ;get reply buffer segment
;          int       I_NOVELL            ;try to call Netware
;          or        ah,ah               ;AH = 0 (DOS)?
;           jne      CNExit              ;if not get out
;          mov       NovVer,bx           ;save version number
;          ;
;CNExit:   exit                          ;all done
          ;
          ; =================================================================
          ;
          ; Find4DOS - Find the 4DOS directory
          ;
          ; On entry:
          ;         No requirements
          ;
          ; On exit:
          ;         AX, BX, CX, DX, SI, DI destroyed
          ;         Other registers and interrupt state unchanged
          ;
          entry     Find4DOS,noframe,,local  ;find our directory
          assume    ds:@curseg          ;fix assumes
          ;
          ; Find the 4DOS directory.  Three methods are tried:
	;
	; (1) If there is a passed envoronment (normally true only in
	; a secondary shell) we look at the end of the passed environment
	; for our own full path name, and use that directory.
	;
          ; (2) In a primary shell, we look for our own CONFIG.SYS SHELL=
          ; line in memory.  If it is found, we extract a COMSPEC path
          ; from it if possible.
          ;
          ; The scan method is to look for our program name, followed by
          ; white space, followed by our command tail.  If this is found,
          ; we scan backward to find a drive letter ("D:").  If the drive
          ; letter is found, we look at the preceding byte to see if it an
          ; "S", indicating the active SHELL= line.  If so, we found the
          ; SHELL line and can extract the path and test if it is valid.
          ; If any of the tests fail, the scan is abandoned and no automatic
          ; COMSPEC path will be set up.
          ;
          ; The scan length constants SCANOFFx and SCANMAXx (in 4DLPARMS.ASM)
          ; are set up so that there is plenty of room to continue within
          ; the segment being scanned if the first character is found at
          ; the end.
	;
	; (3) If both of the above fail, we look for a COMSPEC variable
	; and extract the path from that if possible.
          ;
          ;
          ; Look for our name at the end of the passed environment
          ;
          Status    1,'LC'              ;(Look for COMSPEC)
	mov       ax,PassEnv          ;get passed environment
	or	ax,ax		;anything there?
	 jz	F4Shell		;if not go on
	mov	si,OldEnvSz	;get length of old env as offset
          loadseg   es,ds               ;set es to current data segment
          push      ds                  ;save ds
          mov       ds,ax		;copy environment segment
          assume    ds:nothing          ;ds no good any more
	cmp	wptr [si],1	;is the string count at least 1?
	 jb	F4NoName		;if not, go on
	add	si,2		;skip to program name
          mov       di,offset CSPath    ;get place to put it
          call      CopyTok             ;copy it
          pop       ds                  ;restore ds
          assume    ds:@curseg          ;ds is back to our segment
          mov       si,offset CSPath    ;point to copied COMSPEC
          call      FNCheck             ;find path part
           jc       F4Shell		;if there's any error ignore the info
          mov       CSpecLen,dx         ;save length of path part
          bset      InitFlag,CSPSET     ;show COMSPEC path found
	jmp	F4Exit		;and go on
          ;
F4NoName:	pop	ds		;restore DS
          ;
          ; Set up for the SHELL= scan
          ;
F4Shell:	test      LMFlags,ROOTFLAG    ;are we the root shell?
           jz       F4CSpec		;if not go on
          Status    1,'LS'              ;(Look for SHELL= path)
          cld                           ;go forward
          mov       si,PSP_TLEN         ;point to passed command tail
          lodsb                         ;get tail length
          mov       cl,al               ;copy to CX
          xor       ch,ch               ;clear high byte
           jcxz     F4Setup             ;if nothing there go on
          ;
F4TWhite: lodsb                         ;get a byte of tail
          cmp       al,' '              ;blank?
           je       F4TSkip             ;if so skip it
          cmp       al,TAB              ;tab?
           jne      F4TSave             ;if not, save where we are
F4TSkip:  loop      F4TWhite            ;loop for more white space
          xor       cx,cx               ;all white, clear length
          ;
F4TSave:  mov       TXLen,cx            ;save length without white space
          dec       si                  ;back up to first non-white
          mov       TXStart,si          ;save start of non-white portion
          ;
F4Setup:  mov       ax,offset DGROUP:_end ;get end of loaded file
          add       ax,0Fh              ;add for paragraph roundoff
          shrn      ax,4,cl             ;make it paragraph count
          add       ax,DGROUP           ;add unrelocated DGROUP base to get
                                        ;  end of loaded DGROUP
          sub       ax,SCANOFFP         ;back up to start in mid-segment
          ;
          ; Scan a 64K segment for the program name and command tail
          ;
F4NewSeg: mov       es,ax               ;set segment
          add       ax,SCANOFFP         ;get real segment
          mov       bx,ds:[PSP_MEND]    ;get end of memory
          cmp       ax,bx               ;there already?
           jb       F4SegSet            ;if not go on
          jmp       F4CSpec		;if so forget it
          ;
F4SegSet: mov       di,SCANOFFB         ;start at beginning of segment
          mov       cx,SCANMAXB         ;assume full-length scan
          sub       bx,ax               ;get max length of scan
          cmp       bx,SCANMAXP         ;room for full scan?
           jae      F4SavLen            ;if so go on
          shln      bx,4,cl             ;not enough room, limit scan
          ;
F4SavLen: mov       SHScnLim,cx         ;save limit of scan in this segment
          ;
          ; Look for the program name
          ;
F4Loop:   mov       si,offset ShName    ;point to name
          lodsb                         ;get first byte
          repne     scasb               ;look for it
           jne      F4NxtSeg            ;if not found go on to next segment
          mov       dx,di               ;save start point + 1
          mov       cx,ShNameL - 1      ;get comparison length after 1st char
          repe      cmpsb               ;check for full name
           jne      F4More              ;if not found keep looking
          mov       bx,di               ;save end of name
          ;
          ; Skip white space (including nulls) after program name
          ;
F4Tail:   mov       al,es:[di]          ;get next byte
          or        al,al               ;null?
           je       F4Skip              ;if so skip it
          cmp       al,' '              ;blank?
           je       F4Skip              ;if so skip it
          cmp       al,TAB              ;tab?
           jne      F4Comp              ;if not, try to compare
          ;
F4Skip:   inc       di                  ;skip this byte
          loop      F4Tail              ;and try again
          ;
          ; Compare the command tails
          ;
F4Comp:   mov       cx,TXLen            ;get length for comparison
          jcxz      F4Drive             ;if nothing there, we found it
          mov       si,TXStart          ;get start for comparison
          repe      cmpsb               ;see if tails match
           jne      F4More              ;if not found go on
          ;
          ; Tails are the same, look for "d:" and check drive letter
          ; validity
          ;
F4Drive:  mov       di,dx               ;start looking for "d:" at front
          sub       di,2                ;back up to before name
          std                           ;now go backwards
          mov       cx,PATHMAX          ;get maximum distance to scan
          maxu      cx,di               ;don't go too far back
          mov       al,':'              ;character to look for
          repne     scasb               ;scan backwards for colon
          cld                           ;reset direction
           jne      F4More              ;if not found, give up
          mov       al,es:[di]          ;get drive letter
          call      UpShift             ;make it upper case
          cmp       al,'A'              ;check start
           jb       F4More              ;get out if too low
          cmp       al,'Z'              ;check end
           ja       F4More              ;get out if too high
          cmp       bptr es:[di-1],'S'  ;is it an active SHELL line?
           je       F4Found             ;if so we've got what we want
          ;
          ; Something didn't match, continue the scan in this segment
          ;
F4More:   mov       di,dx               ;restart scan after old first char
          mov       cx,ShScnLim         ;get segment scan limit in bytes
          sub       cx,di               ;don't go past end
          add       cx,SCANOFFB         ;adjust for offset added to DI at
                                        ;  start
           jbe      F4NxtSeg            ;if nothing left go to next segment
          jmp       short F4Loop        ;go try again
          ;
          ; Segment done, go on to the next segment
          ;
F4NxtSeg: mov       ax,es               ;get scan segment
          add       ax,(SCANMAXP - SCANOFFP)  ;add maximum scan in
                                              ;  paragraphs, less offset
          jmp       F4NewSeg            ;and go try next segment
          ;
          ; Found the SHELL line, copy the path and check if it's valid
          ;
F4Found:  mov       si,di               ;point to "d" in "d:"
          loadseg   ds,es               ;source is in high memory
          loadseg   es,cs               ;destination is down here
          mov       di,offset TempPath  ;point to path buffer
          push      di                  ;save for later
          mov       cx,bx               ;get end of name in high memory
          sub       cx,si               ;get length of name
          rep       movsb               ;move the path and name down
          loadseg   ds,cs               ;reset DS to here
          pop       si                  ;get back path buffer offset
          call      FNCheck             ;see if path is real
           jc       F4CSpec              ;if not real give up
          ;
          ; SHELL path is valid, copy it as the COMSPEC path
          ;
          Status    2,'FS'              ;(Found SHELL= path)
          mov       cx,dx               ;copy path length to CX
          call      CSCopy              ;go copy COMSPEC path
	jmp	F4Exit		;all done
          ;
          ; Get the COMSPEC, if any
          ;
F4CSPec:	mov       ax,PassEnv          ;get passed environment
	or	ax,ax		;anything there?
	 jz	F4Exit		;if not get out
	mov       es,ax		;copy passed environment to ES
          mov       si,offset ComSpec   ;point to "COMSPEC" string
          mov       cx,7                ;length of "COMSPEC"
          call      SrchEnv             ;search the environment
           jc       F4Exit		;if not found go on
          Status    2,'FC'              ;(Found COMSPEC)
          loadseg   es,ds               ;set es to current data segment
          push      ds                  ;save ds
          mov       ds,PassEnv          ;copy environment segment
          assume    ds:nothing          ;ds no good any more
          mov       di,offset CSPath    ;get place to put it
          call      CopyTok             ;copy it
          pop       ds                  ;restore ds
          assume    ds:@curseg          ;ds is back to our segment
          mov       si,offset CSPath    ;point to copied COMSPEC
          call      FNCheck             ;find path part
           jc       F4Exit		;if there's any error ignore COMSPEC
          mov       CSpecLen,dx         ;save length of path part
          bset      InitFlag,CSPSET     ;show COMSPEC path found
          ;
F4Exit:   exit                          ;all done
          ;
          ; =================================================================
          ;
          ; InitINI - Initialize the INI file data and load any passed data
          ;
          ; On entry:
          ;         No requirements
          ;
          ; On exit:
          ;         AX, BX, CX, DX, SI, DI destroyed
          ;         Other registers and interrupt state unchanged
          ;
          entry     InitINI,noframe,,local  ;initialize INI file
          assume    ds:@curseg          ;fix assumes
          ;
          Status    1,'II'              ;(Initialize INI file)
          ;
          push      es                  ;save ES
          test      InitFlag,NOPREV     ;is there a previous shell?
           jnz      IIClear             ;if not, clear the INI data
          xor       ax,ax               ;inherit from offset 0
          mov       bx,offset TempStor  ;point to temporary area
          add       bx,0Fh              ;add for roundoff
          and       bx,0FFF0h           ;round to nearest paragraph
          push      bx                  ;save rounded offset
          shrn      bx,4,cl             ;make it a paragraph value
          mov       dx,ds               ;get data segment
          add       bx,dx               ;make absolute segment offset
          mov       dx,OldShell.DSwpILen  ;get data length in bytes
          call      Inherit             ;inherit the INI file data
           jc       IIClear             ;if we can't, go clear it instead
          ;
          pop       si                  ;get source pointer
          cmp       [si].I_Sig,INISIG   ;test signature
           jne      IINoCopy		;if match fails don't copy the data
	cmp	[si].I_Build,INTVER	;also check internal build number
	 je	IICopy		;if both match we can copy
	;
IINoCopy:	bset      InitFlag,XINHERIT   ;disable all other inheritance
          bset      Flags,NOREDUCE      ;disable reduced swapping
          jmp       short IIClear       ;and go on
          ;
IICopy:   loadseg   es,ds               ;get destination segment
          mov       di,offset IFData    ;get destination offset
          mov       cx,(size INIData)   ;get data size in bytes
          rep       movsb               ;move INI file data
          mov       di,offset INIStDat  ;point to strings data
          mov       IFData.I_StrDat,di  ;set strings address
;          mov       cx,IFData.I_StrUse  ;get strings length
          mov       cx,IFData.I_StrMax  ;get strings length
          rep       movsb               ;copy the strings
          mov       di,offset INIKeys   ;point to keys data
          mov       IFData.I_Keys,di    ;set keys address
          mov       cx,IFData.I_KeyUse  ;get keys count
          shln      cx,2                ;convert to bytes (4 bytes/key)
          rep       movsb               ;copy the keys
          mov       ShLevel,1           ;set secondary shell as default,
                                        ;  don't inherit primary shell flag!
          jmp       short IIExit        ;and exit
          ;
IIClear:  mov       si,offset IFData    ;point to INI file data
          mov       [si].I_SecFlg,0               ;clear section flags
          mov       [si].I_StrDat,offset INIStDat ;set strings address
          mov       [si].I_StrMax,INISMAX         ;set strings max
          mov       [si].I_StrUse,0               ;clear strings used
          mov       [si].I_Keys,offset INIKeys    ;set keys address
          mov       [si].I_KeyMax,INIKMAX         ;set keys max
          mov       [si].I_KeyUse,INIDKCnt        ;set keys used (accounts
                                                  ;  for default keys hard-
                                                  ;  coded above)
          push      si                  ;data address on stack for INIClear
          call      INIClear            ;clear the INI file data
          mov       [si].I_Sig,INISIG   ;set the signature
          ;
	test	InitFlg2,W95USWP	;unique swap name forced in Win95?
	 jnz	IISetUSw		;if so go set unique swap item
          cmp       DosMajor,20         ;is this an OS/2 2.0 VDM?
           jb       IIExit              ;if not go on
	;
IISetUSw:	mov       [si].I_USwap,1      ;default to unique swap names
          ;
IIExit:   pop       es                  ;restore ES
          exit                          ;all done
          ;
          ; =================================================================
          ;
          ; PrevINI - Process previous INI file
          ;
          ; On entry:
          ;         No requirements
          ;
          ; On exit:
          ;         AX, BX, CX, DX, SI, DI destroyed
          ;         Other registers and interrupt state unchanged
          ;
          ; Notes:
          ;         Copies file name from INI file data area to TempPath.
          ;         This is because the file name is itself in the INI file
          ;         data area, and we call ProcINI which calls INIParse
          ;         which can move strings around in that area.  If the
          ;         file name string gets moved, then INIParse will print
          ;         bogus file names in any subsequent error messages.  So
          ;         we move the file name to TempPath, which is not affected
          ;         by INIParse.
          ;         
          ;
          entry     PrevINI,noframe,,local  ;process previous INI file
          assume    ds:@curseg          ;fix assumes
          ;
          test      InitFlag,NOPREV     ;any previous shell?
           jnz      PVDone              ;if not then go on
          mov       si,IFData.I_NxtINI  ;get "next INI" file name ptr
          cmp       si,EMPTYSTR         ;anything there?
           jnz      PVGo                ;if so go process it
	mov       si,IFData.I_PrmINI  ;get primary INI file name ptr
          cmp       si,EMPTYSTR         ;anything there?
           jz       PVDone		;if not, we're done here
          test      IFData.I_SecFlg,SB_SEC  ;if file is there, check for
          			    ; secondary section
           jnz      PVGo		;if it's there, process it
	cmp	WinMode,WIN95	;if not, check for 95/98/ME
	 jb	PVDone		;if not 95/98/ME, get out -- if it
	 			; is 95/98/ME, reprocess original INI
          ;
PVGo:     add       si,IFData.I_StrDat  ;get name data address
          call      STR_LEN             ;get length in CX
          inc       cx                  ;copy null as well
          mov       di,offset TempPath  ;point to new location
          mov       ax,di               ;save for ProcINI
          loadseg   es,ds               ;set segment for move
          cld                           ;go forward
          rep       movsb               ;copy string to temporary area
          call      ProcINI             ;reprocess primary INI file (ProcINI
                                        ;  will ignore [Primary] section) or
                                        ;  process user's "next" INI file
          ;
PVDone:   exit                          ;all done
          ;
          ; =================================================================
          ;
          ; ProcINI - Process an INI file
          ;
          ; On entry:
          ;         AX = address of ASCIIZ name of INI file
          ;
          ; On exit:
          ;         Carry clear if all OK, set if error
          ;         AX = Section flags for sections found (in AL), if OK
          ;         BX, CX, DX destroyed
          ;         Other registers and interrupt state unchanged
          ;
          entry     ProcINI,noframe,,local  ;process INI file
          assume    ds:@curseg          ;fix assumes
          ;
          pushm     si,di               ;save registers
          Status    1,'PI'              ;(Parse INI file)
          push      ax                  ;file name address on stack for later
          mov       SavStack,sp         ;save the local stack
          mov       sp,offset BigTOS    ;switch to temporary big stack
          push      ax                  ;file name addr on stack for INIParse
          mov       ax,offset IFData    ;point to INI file data
          push      ax                  ;data address on stack for INIParse
          mov       al,SB_SEC           ;assume ignore [secondary] section
          cmp       bptr ShellNum,0     ;are we shell 0?
           je       PIGo                ;if so ignore [secondary] section
          mov       al,SB_PRIM          ;otherwise ignore [primary] section
          ;
PIGo:     push      ax                  ;save ignore flags for INIParse
          call      INIParse            ;and parse the file
          mov       sp,SavStack         ;switch back to standard stack
          pop       si                  ;get back file name addr
          or        ax,ax               ;check for error
           jbe       INIGood            ;if no error or internal error, exit
	stc                           ;show file I/O error
          jmp       short INIExit	;and get out
          ;
INIGood:  bset      InitFlg2,INIDONE    ;show file processed
          test      InitFlag,NOPREV     ;are we the first shell?
           jz       INIGRet             ;if not we're done
          call      FNorm               ;normalize the file name
          call      STR_LEN             ;get length of name string in CX
          mov       di,offset IFData    ;point to INI file data
          mov       bx,I_PrmINI         ;get offset for string address
          call      INIStr              ;store name string in INI file
                                        ;  string area
           jnc      INIGRet             ;if OK go on
          mov       dx,offset INIStrOv  ;get error offset
          call      StatMsg             ;holler
          ;
INIGRet:  clc                           ;show no error
          ;
INIExit:  popm      di,si               ;restore registers
          exit                          ;all done
          ;
          ; =================================================================
          ;
          ; DefINI - Process the default INI file in the primary shell
          ;
          ; On entry:
          ;         No requirements
          ;
          ; On exit:
          ;         AX, BX, CX, DX destroyed
          ;         Other registers and interrupt state unchanged
          ;
          ; 
          entry     DefINI,noframe,,local  ;define entry point
          ;
          pushm     si,di               ;save registers
          ;
          Status    1,'DI'              ;(Search for default INI file)
          test      InitFlag,NOPREV     ;are we the first 4DOS shell?
           jz       DIDone              ;if not forget it
          test      InitFlg2,INIDONE    ;INI file already processed?
           jnz      DIDone              ;if so get out
          ;
          mov       cx,CSpecLen         ;get COMSPEC length
          or        cx,cx               ;check length
           jz       DITryBR             ;if nothing there go on
          ;
          Status    2,'CD'              ;(Search COMSPEC directory)
          mov       si,offset CSPath    ;get COMSPEC path
          push      cx                  ;save count
          call      TryINI              ;try the INI file
          pop       cx                  ;restore path length
           jnc      DIDone              ;if succesful go on
          cmp       cx,3                ;did we just try root?
           jg       DITryBR             ;if not go on
          mov       al,BootDrv          ;get boot drive
          cmp       al,bptr TempPath    ;did we just try that drive?
           je       DIDone              ;if so there's nothing else to try
          ;
DITryBR:
          Status    2,'BR'              ;(Search root of boot)
          mov       si,offset BootDrv   ;point to boot drive and ":\"
          mov       cx,3                ;get path length
          call      TryINI              ;try that directory
          ;
DIDone:   popm      di,si               ;restore registers
          exit                          ;that's all
          ;
          ; =================================================================
          ;
          ; TryINI - Try to process a default INI file
          ;
          ; On entry:
          ;         SI = address of buffer containing path to try,
          ;           terminated with a backslash
          ;         CX = length of path
          ;
          ; On exit:
          ;         Carry set if ProcINI found an error, clear if not
          ;         AX = error code from ProcINI if carry set,
          ;           otherwise destroyed
          ;         BX, CX, DX, SI, DI, ES destroyed
          ;         Other registers and interrupt state unchanged
          ;
          ; 
          entry     TryINI,noframe,,local  ;define entry point
          ;
          cld                           ;go forward
          mov       di,offset TempPath  ;point to temp buffer
          push      di                  ;save for ProcINI
          loadseg   es,ds               ;set ES for move
          rep       movsb               ;copy path
          mov       si,offset ININame   ;point to standard INI file name
          mov       cx,ININamL          ;get length
          rep       movsb               ;move into place, with null
          pop       ax                  ;get back buffer address
          call      ProcINI             ;try to find INI file in COMSPEC
                                        ;  directory
          exit                          ;that's all
          ;
          ; =================================================================
          ;
          ; ParsTail - Parse the command tail, process the default INI file
          ;            in the primary shell
          ;
          ; On entry:
          ;         DS = Local data segment
          ;         Text to be processed in PSP command tail
          ;
          ; On exit:
          ;         AX, BX, CX, DX, SI, DI, ES destroyed
          ;         All other registers and interrupt state unchanged
          ;
          ; Internal tables
          ;
Switches  db        "ACDEFKLPRYZ/"      ;switch list for branching
SwitchCnt equ       ($ - Switches)      ;length of Switches
          ;
SwJump:   dw        SwAEFile            ;'A', AUTOEXEC file name
          dw        SwTrans             ;'C', transient load
          dw        SwNoAuto            ;'D', disable AUTOEXEC
          dw        SwEnv               ;'E', environment size
          dw        SwAFail             ;'F', auto fail on critical error
          dw        SwKCmd              ;'K', OS/2 "command follows" switch
          dw        SwLocal             ;'L', local aliases / cmd history /
                                        ;  dir history
          dw        SwPerm              ;'P', permanent load
          dw        SwRoot              ;'R', force root shell
          dw        SwStep              ;'Y', single-step
          dw        SwDebug             ;'Z', debugging messages on
          dw        SwINI               ;'/', INI file directive
          ;
          ;
          entry     ParsTail,varframe,,local  ;set up entry point
          assume    ds:@curseg          ;fix assumes
          ;
          varW      SvStart             ;saved start point when looking for
                                        ;  COMSPEC directory
          varW      TailLen		;PSP tail length
          varend
          ;
          loadseg   es,ds               ;point to local segment
          mov       si,PSP_TLEN         ;point to passed command tail
          cld                           ;go forward
          lodsb                         ;get tail length
          mov       bl,al               ;copy for offset
	cmp	bl,7Eh		;is it too long?
	 jbe	PTSavLen		;if not go save it
	mov	bl,7Eh		;limit to 126 bytes
	;
PTSavLen:	xor       bh,bh               ;clear high byte
	mov	wptr TailLen,bx	;save it
          mov       bptr [bx][si],0     ;null-terminate tail (note display
                                        ;  code at end of routine uses this
                                        ;  terminator!)
;          mov       cx,bx               ;copy length
;          or        cx,cx               ;check tail length
;          jnz       TailMove            ;if something there go on
;          mov       si,offset TailBuf   ;point to dummy tail buffer
;          mov       bptr [si],0         ;terminate it for empty tail
;          jmp       PTDisp              ;go copy any COMSPEC
;          ;
;TailMove: inc       cx                  ;include null byte in the copy
;          mov       di,offset TailBuf   ;point to tail buffer
;          rep       movsb               ;copy tail to buffer
;          ;
;          ; Look for switches
;          ;
;TailStrt: mov       si,offset TailBuf   ;point to our buffer
          ;
TailScan: lodsb                         ;get byte of tail
          cmp       al,'@'              ;@filename?
           je       GotIName            ;yes, go process it
          cmp       al,'/'              ;slash switch?
           je       GotSwChr            ;if so process it
          cmp       al,'-'              ;dash switch?
           je       GotSwChr            ;if so process it
          call      ChkWhite            ;see if it's whitespace or null
           jc       SwTerm              ;if null we're done
           je       TailScan            ;if found whitespace keep going
          ;
          ; Found something that isn't whitespace or a switch.  First, see
          ; if what we found is a directory name; if so assume it is the
          ; COMSPEC directory.  Otherwise, assume it is the start of a
          ; command, stop switch processing, and return the rest of the
          ; tail to the caller.
          ;
          mov       SvStart,si          ;save start point
          dec       si                  ;back up to first character
          mov       di,offset TempPath  ;point to temporary path buffer
          call      UserPath            ;check the path
           jc       NotPath             ;give up if any error
          test      al,ATR_DIR          ;is it a directory?
           jz       NotPath             ;if not try for command
          dec       cx                  ;don't include terminator in length
          call      CSCopy              ;copy path and length, set flag
          jmp       short TailScan      ;and continue
          ;
NotPath:  mov       si,SvStart          ;not a path -- must be a command
                                        ;  so get back start point
          ;
          ; Tail processing is done
          ;
SwTerm:   jmp       PTDone              ;that's all folks
          ;
          ;
          ; Found a switch
          ;
GotSwChr: lodsb                         ;get the switch
          call      ChkWhite            ;is it whitespace or null?
           jc       SwTerm              ;if null quit
           je       TailScan            ;if whitespace ignore it
          call      UpShift             ;neither, upshift it
          mov       cx,SwitchCnt        ;get scan length
          mov       di,offset Switches  ;point to local switch list
          repne     scasb               ;scan for switch
          jne       NotSw               ;if not a valid switch copy it
          sub       di,(offset Switches + 1)  ;adjust to switch number
          shl       di,1                ;double for word offset
          jmp       wptr cs:SwJump[di]  ;and jump to correct switch routine
          ;
          ; Bad switch -- complain, then skip looking for white space
          ;
NotSw:    mov       dx,offset BadSwMsg  ;get message address
          calldos   MESSAGE             ;display message
          mov       dl,-1[si]           ;get back bad character
          calldos   DISPCHR             ;display it
          mov       dx,offset BadSwIgn  ;get closing message address
          calldos   MESSAGE             ;display message
          ;
NSwLoop:  xor	cx,cx		;no character count
	call      NxtWhite            ;skip to next whitespace
	 jc	SwTerm		;if null found, quit
          jmp       TailScan            ;after whitespace, go back for next
          			;  item
          ;
          ;
          ; ------- @filename - load an INI file -------
          ;
GotIName: mov       dx,offset TempPath  ;point to option filename area
          mov       di,dx               ;copy to DI
          call      CopyTok             ;copy the token
          stocb     0                   ;make it ASCIIZ
          mov       ax,dx               ;copy file name address
          call      ProcINI             ;process the file
           jnc      TailScan            ;if OK, continue scan
	mov	dx,offset IniNFMsg	;get message address
          calldos   MESSAGE             ;display message
          jmp       TailScan            ;and continue scan
          ;
          ;
          ; ------- /A - AUTOEXEC file name -------
          ;
SwAEFile: call      AESwitch            ;go process file name / path
          jmp       TailScan            ;continue scan
          ;
          ;
          ; ------- /C - transient load -------
          ;
SwTrans:  bset      InitFlag,TRANLOAD   ;transient load, set flag
          cmp       DosMajor,20         ;OS/2 2.0 or above DOS box?
           jae      SwTrDone            ;if so skip clear of /P flag
          bclr      InitFlag,PERMLOAD   ;clear any previous /P
          ;
SwTrDone: dec       si                  ;back up to "C", code at PTDone will
                                        ;  back up to switch char
          jmp       PTDone              ;tail processing is done, return rest
                                        ;  of tail to caller
          ;
          ;
          ; ------- /D - Disable AUTOEXEC (MS-DOS 6+) -------
          ;
SwNoAuto: bset      InitFlg2,NOAUTOEX   ;disable AUTOEXEC.BAT
	cmp	DosMajor,7	;DOS 7 or above?
	 jb	TailScan		;if not go on
	call      DefINI              ;process default INI file so next
				;  setting is retained
	mov	IFData.I_NoGUI,1	;disable Win95 GUI load
          jmp       TailScan            ;continue scan
          ;
          ;
          ; ------- /E - environment size -------
          ;
SwEnv:    call      DefINI              ;process default INI file in primary
                                        ;  shell, so that /E can override
                                        ;  Environment in the default file
          mov       al,[si]             ;get separator
          call      NumSw               ;read environment size
          or        ax,ax               ;check result
           je       SwEDone             ;if zero or no value go on
          mov       IFData.I_Env,ax     ;save requested environment size
          ;
SwEDone:  jmp       TailScan            ;continue scan
          ;
          ;
          ; ------- /F - auto fail on critical error -------
          ;
SwAFail:  call      DefINI              ;process default INI file in primary
                                        ;  shell, so that /F can override
                                        ;  CritFail in the default file
          bset      LMFlag2,AUTOFAIL    ;set automatic fail flag
          jmp       TailScan            ;continue scan
          ;
          ;
          ; ------- /K - "command follows" -------
          ;
SwKCmd:   cmp       bptr [si],":"       ;is it /K:?
           jne      SwKVer              ;if not go on
          inc       si                  ;skip ":"
          ;
SwKVer:   cmp       DosMajor,6          ;check if DOS 6
           jne      SwKDone		;if not, tail processing is done
          bset      InitFlg2,NOAUTOEX   ;/K disables AUTOEXEC.BAT in DOS 6+
          ;
SwKDone:  jmp       PTDisp              ;tail processing done, return rest
          ;
          ;
          ; ------- /L[AFHD] - local aliases / cmd history / dir history-------
          ;
SwLocal:  call      DefINI              ;process default INI file in primary
                                        ;  shell, so that /L can override
                                        ;  LocalAliases / LocalHistory in the
                                        ;  default file
          lodsb                         ;get next byte
          call      UpShift             ;upshift it
          cmp       al,'A'              ;local aliases?
           je       SwLAlias            ;if so go set that
          cmp       al,'F'              ;local functions?
           je       SwLFunc             ;if so go set that
          cmp       al,'H'              ;local cmd history?
           je       SwLHist             ;if so go set that
          cmp       al,'D'              ;local dir history?
           je       SwLDHist            ;if so go set that
          dec       si                  ;nothing useful after /L, back up
          mov       IFData.I_LocalH,1   ;set local cmd history
          mov       IFData.I_LocalD,1   ;set local dir history
          mov       IFData.I_LocalF,1   ;set local function history
          ;
SwLAlias: mov       IFData.I_LocalA,1   ;set local aliases only
          jmp       TailScan            ;continue scan
          ;
SwLFunc:  mov       IFData.I_LocalF,1   ;set local functions
          jmp       TailScan            ;continue scan
          ;
SwLHist:  mov       IFData.I_LocalH,1   ;set local cmd history
          jmp       TailScan            ;continue scan
          ;
SwLDHist: mov       IFData.I_LocalD,1   ;set local dir history
          jmp       TailScan            ;continue scan
          ;
          ;
          ; ------- /P - permanent load -------
          ;
SwPerm:   bset      InitFlag,PERMLOAD   ;permanent load, set flag
          call      AESwitch            ;go process any file name / path
          jmp       TailScan            ;continue scan
          ;
          ;
          ; ------- /R - Force root shell (processed earlier) -------
          ;
SwRoot:   jmp       TailScan            ;continue scan
          ;
          ;
          ; ------- /Y - single-step 4START and AUTOEXEC -------
          ;
SwStep:   mov       IFData.I_Step,1     ;single step, set flag
          jmp       TailScan            ;continue scan
          ;
          ;
          ; ------- /Z - Turn on debugging messages -------
          ;
SwDebug:  call      SetDBLev            ;set debug message level
          jmp       TailScan            ;continue scan
          ;
          ;
          ; ------- // - INI file directive -------
          ;
SwINI:    call      DefINI              ;process default INI file in primary
                                        ;  shell, so that // can override
                                        ;  values from the default file
          mov       dx,offset TempPath  ;point to temporary storage
          mov       di,dx               ;copy to DI
          call      CopyTok             ;copy the token
          stocb     0                   ;make it ASCIIZ
          push      si                  ;save scan address
          mov       si,dx               ;copy directive address
          mov       di,offset IFData    ;point to INI data structure
          xor       al,al               ;no flags
          xor       dx,dx               ;no section flags
          call      INILine             ;parse the line
           jnc      SIDone              ;if all OK go on
          mov       si,offset INIDMsg1  ;point to first part of message
          call      PUT_STR             ;display it
          mov       si,offset TempPath  ;point to string
          call      PUT_STR             ;display it
          mov       si,offset INIDMsg2  ;point to end of message
          call      PUT_STR             ;display it
          mov       si,dx               ;point to error text
          call      PUT_STR             ;display it
          call      PUT_NEWLINE         ;output CRLF
          ;
SIDone:   pop       si                  ;restore scan address
          jmp       TailScan            ;continue scan
          ;
          ;
          ; Command tail processing is done -- display the tail if requested
          ; (display is here to allow //Debug= in tail itself to turn display
          ; on!)
          ;
PTDone:   dec       si                  ;back up to last character scanned
          ;
PTDisp:   test      IFData.I_Debug,ID_TAIL  ;display tail?
;           jz       PTPSP               ;if not go on
           jz       PTSavPtr            ;if not go on
          push      si                  ;save pointer
          mov       si,offset TailMsg   ;point to tail message
          call      PUT_STR             ;display it
          mov       si,PSP_TAIL         ;get original tail offset (code above
                                        ;  added null termination)
          call      PUT_STR             ;display tail
          mov       si,offset TailEnd   ;point to tail end message
          call      PUT_STR             ;display that
          call      GET_CHR             ;read a character and ignore it
          call      PUT_NEWLINE         ;do cr/lf
          pop       si                  ;restore pointer
	;
	; See if we have a long command line in CMDLINE -- for this
	; to occur we have to have a tail length of 126 bytes, a
	; CMDLINE variable longer than the tail, and a match between
	; the passed tail and the CMDLINE contents after the first
	; whitespace
	;
PTSavPtr:	push	si		;save pointer
	cmp	wptr TailLen,7Eh	;check for full length
	 jb	PTPopPSP		;if not, use PSP command line
	;
	; Look for CMDLINE
	;
	mov       ax,PassEnv          ;get passed environment
	or	ax,ax		;anything there?
	 jz	PTPopPSP		;if not, use PSP command line
	mov       es,ax		;copy passed environment to ES
          mov       si,offset CmdLine	;point to "CMDLINE" string
          mov       cx,7                ;length of "CMDLINE"
          call      SrchEnv             ;search the environment
	 jc	PTPopPSP		;if not found, use PSP command line
	;
	; Skip to the second argument in CMDLINE, and make sure the
	; remaining command tail is long enough
	;
	push	ds		;save DS
	loadseg	ds,es		;now DS:SI = address of CMDLINE value\
          call	SkpWhite		;skip any initial whitespace
	 jc	PTPop2		;if all whitespace, use PSP cmd line
	call	NxtWhite		;skip first argument
	 jc	PTPop2		;if nothing there, use PSP cmd line
	call	SkpWhite		;skip to start of second argument
	 jc	PTPop2		;if all whitespace, use PSP cmd line
	pop	ds		;restore DS
	mov	di,si		;DI = address of CMDLINE tail
	cmp	cx,7Eh		;is CMDLINE now too short?
	 jbe	PTPopPSP		;if so, use PSP command line
	;
	; Compare the PSP tail and the CMDLINE tail
	;
	mov	cx,7Eh		;get tail length
	mov	di,si		;now ES:DI = CMDLINE tail
	mov	si,PSP_TLEN + 1	;point back to start of tail
	call	SkpWhite		;skip white space in PSP tail
	 jc	PTPopPSP		;if all whitespace, forget it
	dec	si		;back up to first PSP character
				;  (SkpWhite returns address + 1)
	inc	cx		;adjust CX to compensate
	dec	di		;back up to first CMDLINE character
	pushm	di,es		;save CMDLINE pointer
	;
PTCLLoop:	mov	al,es:[di]	;get byte from CMDLINE tail
	inc	di		;skip to next byte
	call	UpShift		;shift to upper case
	mov	ah,al		;save it
	lodsb			;get byte from PSP tail
	call	UpShift		;shift to upper case
	cmp	ah,al		;same?
	 jne	PTCLDiff		;if not, give up
	loop	PTCLLoop		;otherwise keep looking
	;
	; PSP and CMDLINE match -- use CMDLINE as the command line
	;
	popm	es,di		;restore CMDLINE pointer
	dstore	TailPtr,di,es	;save pointer for later
	pop	si		;clean up stack
	jmp	PTINI		;and get out
	;
PTCLDiff:	popm	di,es		;clean up stack
	jmp	PTPopPSP		;go save PSP pointers
	;
	; CMDLINE not found or doesn't match -- save pointer to remainder
	; of tail in PSP
      	;
PTPop2:	pop	ds		;restore DS
PTPopPSP:	pop	si		;restore old tail pointer
	;
PTSavPSP:	dstore	TailPtr,si,ds	;save pointer for later
	jmp	PTINI		;and get out
	;
;          ;
;          ; Move the rest of the tail, if any, back to the PSP
;          ;
;PTPSP:    mov       di,PSP_TAIL         ;point to tail
;          loadseg   es,cs               ;get PSP segment
;          ;
;	cmp	WinMode,WIN95	;is this Win95?
;	 je	PTCSChk		;if so we want the COMSPEC path even
;	 			;  if not loaded with /P
;;          test      InitFlag,TRANLOAD   ;loading transient?
;;           jnz      TailCopy            ;if so ignore COMSPEC path
;          test      InitFlag,PERMLOAD   ;see if loading permanent (primary)
;           jz       TailCopy            ;if not ignore COMSPEC path
;	;
;PTCSChk:	test      InitFlag,CSPSET     ;was a COMSPEC path specified?
;           jz       TailCopy            ;if not ignore it
;          Status    2,'CB'              ;(Copy COMSPEC path back)
;          push      si                  ;save start point for copy
;          mov       si,offset CSPath    ;get new COMSPEC path address
;          mov       cx,CSpecLen         ;get its length
;          rep       movsb               ;copy it to the tail
;          stocb     ' '                 ;stick a space after it
;          pop       si                  ;restore start addr of rest of tail
;          ;
;TailCopy:
;          Status    1,'CT'              ;(Copy command tail)
;          lodsb                         ;get a byte from old tail
;          or        al,al               ;check it
;           jz       SetTLen             ;if it's NUL we're all done, go on
;          cmp       di,PSP_LAST         ;got enough room?
;           jge      TooLong             ;if not, holler
;          stosb                         ;save it
;          jmp       short TailCopy      ;and loop
;          ;
;TooLong:  mov       dx,offset TLongMsg  ;point to message
;          calldos   MESSAGE             ;holler
;          ;
;SetTLen:  push      di                  ;save last byte + 1
;          stocb     CR                  ;put <cr> terminator in tail
;          mov       cx,PSP_LAST + 1     ;point to end of PSP + 1
;          sub       cx,di               ;get remaining byte count
;          jle       PutTLen             ;if full go on
;          xor       al,al               ;get null
;          rep       stosb               ;null out rest of tail
;          ;
;PutTLen:  pop       di                  ;get back last byte + 1
;          mov       ax,PSP_TLEN         ;get start - 1
;          xchg      ax,di               ;swap with last byte + 1
;          sub       ax,di               ;get length + 1
;          dec       ax                  ;get length
;          stosb                         ;store in first byte
          ;
PTINI:	call      DefINI              ;process default INI file in primary
                                        ;  shell if it wasn't processed
                                        ;  earlier
          ;
          exit                          ;all done
          ;
          ; =================================================================
          ;
          ; AESwitch - Process switch with AUTOEXEC name / path / params
          ;
          ; On entry:
          ;         SI points to character after switch
          ;
          ; On exit:
          ;         AX, BX, CX, DX, DI destroyed
          ;         Other registers and interrupt state unchanged
          ;         Sets up AutoExecPath in INI file data
          ;
          entry     AESwitch,varframe,,local  ;handle AutoExecPath
          assume    ds:@curseg          ;fix assumes
          ;
          varB      ASPFlag             ;non-zero if parms expected
          varend
          ;
          mov       bptr ASPFlag,0      ;clear parms flag
          lodsb                         ;get next character
          cmp       al,'/'              ;slash switch?
           je       AEBackup            ;if so we have no file name
          cmp       al,'-'              ;dash switch?
           je       AEBackup            ;if so we have no file name
          ;
AEChkCol: cmp       al,':'              ;is it ":"?
           je       AESkip              ;if so go get next character
          cmp       al,'='              ;also allow "="
           jne      ASChkQ              ;if not that go on
          ;
AESkip:   lodsb                         ;get next
          ;
ASChkQ:   mov       ah,'"'              ;assume terminate on a quote
          cmp       al,ah               ;is string quoted?
           jne      ASNQuote            ;if not go on
          inc       bptr ASPFlag        ;set parms flag
          jmp       short ASProc        ;and go on
          ;
ASNQuote: mov       ah,' '              ;no quote, terminate on a space
          dec       si                  ;back up to start of string
          ;
          ; Find end of file name part of string, get out if empty or
          ; all white space
          ;
ASProc:   mov       di,offset AEBreak   ;point to break table
          mov       2[di],ah            ;store terminator in table
          push      di                  ;save break table address
          call      DefINI              ;process default INI file in primary
                                        ;  shell, so that switch can override
                                        ;  values from the default file
          pop       di                  ;restore break table address
          call      STR_CSPN            ;skip to first break character
           jcxz     ASClean             ;if string empty get out
          ;
          ; Process and store filename
          ;
          mov       bx,offset AEXDir    ;point to AutoExecPath directive
          mov       dx,AEXDLen          ;get length
          call      ASStore             ;go store result in INI data
           jnc      ASParms             ;if OK go on
          ;
          ; Name was bad -- holler, then skip past the rest of the spec
          ; (if any)
          ;
          mov       dx,offset BadAEFil  ;point to message
          calldos   MESSAGE             ;display message
          cmp       bptr ASPFlag,0      ;looking for parms?
           je       ASDone              ;if not get out
          mov       di,offset AEBreak+2 ;point to quote only
          call      STR_CSPN            ;skip to quote or end
          add       si,cx               ;point to quote or end
          jmp       short ASClean       ;clean up and exit
          ;
          ; Find end of parameters, get out if none there
          ;
ASParms:  cmp       bptr ASPFlag,0      ;looking for parms?
           je       ASDone              ;if not must be just a name so
                                        ;  we're done
          mov       di,offset AEBreak+2 ;point to quote only
          call      STR_CSPN            ;skip to quote or end
           jcxz     ASClean             ;if string empty get out
          mov       bx,offset AEXParm   ;point to AutoExecParms directive
          mov       dx,AEXPLen          ;get length
          call      ASStore             ;go store result in INI data
          ;
ASClean:  lodsb                         ;get last byte
          or        al,al               ;was it end of line?
           jne      ASDone              ;if not it was a quote, exit
          ;
AEBackup: dec       si                  ;back up to end
          ;
ASDone:   exit                          ;that's all
          ;
          ; =================================================================
          ;
          ; ASStore - Store name or parameter data from AUTOEXEC switch
          ;
          ; On entry:
          ;         BX = address of directive name
          ;         DX = length of directive name
          ;         SI = address of text
          ;         CX = length of text
          ;
          ; On exit:
          ;         AX, BX, CX, DX, DI destroyed
          ;         SI = address of last text byte stored + 1
          ;         Other registers and interrupt state unchanged
          ;
          entry     ASStore,noframe,,local  ;store data from AUTOEXEC
                                            ;  switch parsing
          mov       di,offset TempPath  ;point to temporary storage
          push      di                  ;save buffer address
          pushm     cx,si               ;save name length and scan address
          mov       si,bx               ;point to AutoExecParms directive
          mov       cx,dx               ;copy to CX
          rep       movsb               ;copy directive
          popm      si,cx               ;get back scan address and text len
          rep       movsb               ;copy text
          stocb     0                   ;terminate it
          pop       di                  ;get back buffer address in DI
          push      si                  ;save scan address
          mov       si,di               ;buffer address to SI
          mov       di,offset IFData    ;point to INI data structure
          xor       al,al               ;no flags
          xor       dx,dx               ;no section flags
          call      INILine             ;parse the line
          pop       si                  ;restore scan address
          exit                          ;returns with carry from INILine
          ;
          ; =================================================================
          ;
          ; FixupINI - Copy INI file data to local data and vice versa, clean
          ; up INI data
          ;
          ; On entry:
          ;         No requirements
          ;
          ; On exit:
          ;         AX, BX, CX, DX, SI, DI, ES destroyed
          ;         Other registers and interrupt state unchanged
          ;
          entry     FixupINI,noframe,,local  ;copy INI data
          assume    ds:@curseg          ;fix assumes
          cld                           ;go forward
          ;
          Status    2,'CI'              ;(Copy INI data to local data)
          mov       bx,offset IBitTab   ;point to INI file bit table
          mov       cx,IBitCnt          ;get number of bits to copy
          ;
IBLoop:   mov       si,[bx].IBLoc       ;get location of data item
          mov       ah,bptr IFData[si]  ;get data item (0 or 1)
          test      [bx].IBFlag,IBREV   ;should sense be reversed?
           jz       IBChkClr            ;if not go on
          xor       ah,1                ;reverse sense of INI file bit
          ;
IBChkClr: or        ah,ah               ;check if being cleared
           jnz      IBMove              ;if not go move bit
          test      [bx].IBFlag,IBNOCLR ;clear disabled?
           jnz      IBNext              ;if so go on
          ;
IBMove:   mov       di,[bx].IBFlAddr    ;get flag byte address
          mov       al,[bx].IBMask      ;get bit mask (reversed)
          and       [di],al             ;clear the flag bit
          or        ah,ah               ;check data item
           jz       IBNext              ;if not set, go on
          not       al                  ;get non-reversed bit mask
          or        [di],al             ;set flag bit
          ;
IBNext:   add       bx,(size IBit)      ;move to next item
          loop      IBLoop              ;and loop for more
          ;
          ; Calculate the byte length of the keystroke substitution table
          ;
          mov       cx,IFData.I_KeyUse  ;get keys count
          shln      cx,2                ;convert to bytes (4 bytes/key)
          mov       INIKLen,cx          ;save byte count
          ;
          ; Set the DESQView and Windows flags from local data, ignoring
          ; any passed values
          ;
SetDVWin: mov       al,WinMode          ;get Windows flag
          mov       IFData.I_WMode,al   ;save it
	mov	IFData.I_W95Dsk,0	;clear Win95/98 desktop flag
	cmp	al,WIN95		;is it Win95/98 GUI?
	 jb	FIChkOS2		;if not go on
	cmp	OldShell.ShWMode,WIN95  ;is the previous shell from the
				;Win95/98 GUI?
	 jae	FIChkOS2		;if so go on
	mov	IFData.I_W95Dsk,1	;if not, set desktop flag
	;
FIChkOS2:	xor       ax,ax               ;get zero
          cmp       DosMajor,10         ;is it OS/2?
           jae      NoDV                ;if so don't allow DV calls
          cmp       DVVer,ax            ;is DV version set?
           je       NoDV                ;if not go save zero
          inc       al                  ;bump to make it 1
          jmp       short SetDV         ;go save it
          ;
NoDV:     bclr      LMFlag2,DVCLEAN     ;if DV isn't there kill cleanup but
                                        ;  for this shell only (leaves INI
                                        ;  data value alone for subsequent
                                        ;  shells)
          ;
SetDV:    mov       IFData.I_DVMode,al  ;save DV flag
          ;
          ; Set the boot drive
          ;
          mov       al,BootDrv          ;get boot drive
          mov       IFData.I_BootDr,al  ;save for transient portion and other
                                        ;  shells
	;
	; Set the MS-DOS 7+ flag
	;
	push	ds		;save DS
	mov	ax, 04A33h	;undoc'd call to see if it's MS-DOS7
	int	2Fh		;  versus PC-DOS7
	popm	ds		;restore DS
	cmp	ax, 0		;is it MS-DOS 7?
	 jne	FIAutoEx		;if not go on
	mov	IFData.I_MSDOS7,1	;if so set flag
          ;
          ; Set up shell level, and AUTOEXEC path if it isn't set already
          ;
FIAutoEx:	test      InitFlag,PERMLOAD   ;permanent load?
           jz       FIChkLog            ;if not forget AUTOEXEC
          mov       IFData.I_ShLev,0    ;permanent, set level to 0
          mov       bx,I_AEPath         ;get address for path offset
          mov       si,offset AutoExec  ;point to AUTOEXEC name
          mov       di,offset IFData    ;point to INI data structure
          test      InitFlg2,NOAUTOEX   ;is AUTOEXEC disabled?
           jz       FIAEChk             ;if not check the path
          cmp       wptr [di][bx],EMPTYSTR  ;path currently empty?
           je       FIChkLog            ;if so we're done
          xor       cx,cx               ;get zero for new string length
          jmp       short FIAEPut       ;and go kill user path
          ;
FIAEChk:  cmp       wptr [di][bx],EMPTYSTR  ;any path specified?
           jne      FIAEMod             ;if so go add "AUTOEXEC.BAT" to it
          mov       al,BootDrv          ;get boot drive
          mov       bptr [si],al        ;no path, just store boot drive
          mov       cx,AutoLen          ;get length
          jmp       short FIAEPut       ;and go store full path
          ;
FIAEMod:  mov       si,[di][bx]         ;get offset of current string
          add       si,[di].I_StrDat    ;get address of string
          call      STR_LEN             ;get length in CX
          mov       bx,cx               ;copy length
          cmp       bptr -1[bx][si],'\' ;is it just a path?
           jne      FIAELen             ;if not we're done, go check length
          push      di                  ;save address of INI data
          mov       di,offset TempPath  ;point to temp buffer
          push      di                  ;save that address
          loadseg   es,ds               ;set buffer segment
          rep       movsb               ;copy current path to temp buffer
          mov       si,offset AutoExec+3  ;point to "AUTOEXEC.BAT"
          mov       cx,AutoLen-3        ;get length with terminator
          rep       movsb               ;copy that in
          mov       cx,di               ;get end
          pop       si                  ;restore address of temp buffer
          sub       cx,si               ;get new count
          pop       di                  ;restore address of INI data
          mov       bx,I_AEPath         ;get address for path offset
          ;
FIAEPut:  push      cx                  ;save length of name
          call      IniStr              ;add, delete, or modify the AUTOEXEC
                                        ;  string
          pop       cx                  ;restore length
          ;
          ; Make sure AUTOEXEC name plus parameters is not too long
          ;
FIAELen:  mov       dx,cx               ;copy file name length
          mov       bx,I_AEParm         ;get address for parameters offset
          mov       si,[di][bx]         ;get offset of parameter string
          add       si,[di].I_StrDat    ;get address of string
          call      STR_LEN             ;get parameters length in CX
          add       dx,cx               ;add to file name length
          cmp       dx,AEMAX            ;is it too big?
           jbe      FIChkLog            ;if not go on
          xor       cx,cx               ;get zero
          call      IniStr              ;clear out parameter string
          mov       dx,offset AELngMsg  ;get error message address
          call      StatMsg             ;display message
          ;
          ; Check for a log file name and add a blank one if we don't have
          ; one
          ;
FIChkLog: mov       bx,I_LogNam         ;get address for log file name offset
          call      BlankLog            ;set it to blanks if necessary
          mov       bx,I_HLogNam        ;get address for hist log file name
                                        ;  offset
          call      BlankLog            ;set it to blanks if necessary
	;
	; Set InstallPath to our own directory (as best we can)
	;
          mov       bx,I_IPath          ;get address for path offset
          mov       di,offset IFData    ;point to INI data structure
	cmp	wptr [di][bx],EMPTYSTR  ;did user define InstallPath?
;	 jne	FIChkW95		;if so, leave it alone
	 jne	FIChkSwR		;if so, leave it alone
          test      InitFlag,CSPSET     ;was a COMSPEC path specified?
	 jz	FIChkHP		;if not, look at HelpPath
          mov       cx,CSpecLen         ;get COMSPEC length
	mov	si,offset CSPath	;point to COMSPEC path
          call      IniStr              ;store as InstallPath
;	jmp	FIChkW95		;and go on
	jmp	FIChkSwR		;and go on
	;
	; No InstallPath and no COMSPEC -- use the HelpPath, if any,
	; and clear it so C code doesn't bother with it
	;
FIChkHP:	mov	ax,[di].I_HPath	;get HelpPath offset
	cmp	ax,EMPTYSTR  	;did user define a help path?
;	 je	FIChkW95		;if not go on
	 je	FIChkSwR		;if not go on
	mov	[di][bx],ax	;save HelpPath offset for 4DOSPath
	mov	wptr [di].I_HPath,EMPTYSTR  ;and clear HelpPath
;	;
;	; Disable Win95 LFN calls if in DOS 6 or below, or OS/2
;	;
;FIChkW95:	cmp	DosMajor,6	;DOS 6 or below?
;	 jbe	FINoLFN		;if so kill LFNs
;	cmp	DosMajor,10	;OS/2 or NT?
;	 jb	FIChkSwR		;if not, allow LFNs
;	;
;FINoLFN:	mov	IFData.I_W95LFN,0	;disable LFN support
          ;
          ; Disable reduced swapping if swap file reopen is set
          ;
FIChkSwR:	test      LMFlag2,DREOPEN     ;swap file reopen requested?
           jz       FIChkStk		;if not go on
          bset      Flags,NOREDUCE      ;disable reduced swapping
          ;
          ; Increase the stack size under Windows 95/98
          ;
FIChkStk: cmp	WinMode,WIN95	;is this Win95/98?
	 jb	FINNTest 	 	;if not go on
	cmp	IFData.I_Stack,STACKDEF  ;default stack size?
	 jne	FINNTest		;if not, user changed it, go on
	mov	ax,STACKW95	;get Win95 default size
	mov	IFData.I_Stack,ax	;save as new stack size
          ;
          ; Disable Netware Names unless we are the root shell
          ;
FINNTest: test      LMFlags,ROOTFLAG    ;are we the root shell?
           jnz      FISavSh             ;if so go on
          mov       IFData.I_NName,0    ;kill Netware Names
          ;
FISavSh:  mov       al,ShellNum         ;get shell number
          mov       bptr IFData.I_Shell,al  ;save it
	mov       al,PrvShNum         ;get previous shell number
          mov       bptr IFData.I_PShell,al  ;save it
          ;
	; Clear the OPTION bit flags
	;
	lea	di,IFdata.I_OBits	;point to option bit flags array
	mov	cx,OBSIZE		;get size of array in bytes
	xor	al,al		;get zero
	rep	stosb		;clear the array
	;
	exit                          ;all done
          ;
          ; =================================================================
          ;
          ; BlankLog - Set up a blank log file name in the INI data
          ;
          ; On entry:
          ;         BX = offset of pointer to this name in IFData
          ;
          ; On exit:
          ;         AX, BX, CX, DX, SI, DI destroyed
          ;         Other registers and interrupt state unchanged
          ;         Sets up COMSPEC in Netware Names loadable module
          ;
          entry     BlankLog,noframe,,local  ;set up log file names
          assume    ds:@curseg          ;fix assumes
          ;
          Status    2,'LN'              ;(Set up log file names)
          cmp       wptr IFData[bx],EMPTYSTR  ;is name set already?
           jne      BLDone              ;if so go on
          mov       di,offset TempPath  ;point to buffer
          mov       si,di               ;copy for later
          loadseg   es,ds               ;set buffer segment
          cld                           ;go forward
          xor       al,al               ;get zero
          mov       cx,PATHBLEN         ;get buffer length in words
          push      cx                  ;save it
          rep       stosb               ;fill with blanks
          pop       cx                  ;restore count
          mov       di,offset IFData    ;point to data structure
          call      IniStr              ;add the string
          ;
BLDone:   exit                          ;that's all
          ;
          ; =================================================================
          ;
          ; SetupNN - Set up COMSPEC in Netware Names
          ;
          ; On entry:
          ;         No requirements
          ;
          ; On exit:
          ;         AX, BX, CX, DX, SI, DI destroyed
          ;         Other registers and interrupt state unchanged
          ;         Sets up COMSPEC in Netware Names loadable module
          ;
          entry     SetupNN,noframe,,local  ;set up Netware Names
          assume    ds:@curseg          ;fix assumes
          ;
          Status    1,'NN'              ;(Set up Netware Names)
          cmp       IFData.I_NName,0    ;load Netware dummy names?
           je       NNDone              ;if not go on
          cld                           ;go forward
          loadseg   es,ds               ;set destination segment
          mov       di,offset NNData    ;and offset
          mov       cx,CSPecLen         ;get COMSPEC length
           jcxz     NNBoot              ;if not set then default it
          mov       si,offset CSPath    ;set source offset
          rep       movsb               ;move the path into place
          mov       al,'\'              ;get backslash for termination
          cmp       bptr [di-1],al      ;terminating backslash?
           je       NNComNam            ;if so go on
          stosb                         ;if not, add one
          ;
NNComNam: mov       si,offset ShName    ;point to "4DOS.COM"
          mov       cx,ShNameL          ;get length
          jmp       short NNPut         ;go store name
          ;
NNBoot:   mov       al,BootDrv          ;no COMSPEC so get boot drive
          mov       si,offset ShPath    ;point to "x:\4DOS.COM"
          mov       [si],al             ;set drive
          mov       cx,ShPathL          ;get length
          ;
NNPut:    rep       movsb               ;copy COMSPEC name or full path
          xor       al,al               ;get terminating null
          stosb                         ;store in buffer
          ;
NNDone:   exit                          ;that's all
          ;
          ; =================================================================
          ;
          ; AdjLists - Adjust environment, alias, functions, and history sizes
          ;
          ; On entry:
          ;         No requirements
          ;
          ; On exit:
          ;         AX, BX, CX, DX, SI, DI destroyed
          ;         Other registers and interrupt state unchanged
          ;
          entry     AdjLists,noframe,,local  ;adjust list sizes
          assume    ds:@curseg          ;fix assumes
          ;
          ; Adjust the requested environment, alias, func, cmd history and dir
          ; history sizes so that they meet the following conditions, in
          ; the order specified:
          ;         - big enough to hold the old data
          ;         - bigger than the absolute minimum
          ;         - big enough for the user's requested size
          ;         - smaller than the absolute maximum
          ;
          ; For aliases,funcs, and history, the old size is used if none was
          ; specified by the user; if no old size is available the default
          ; is used.
          ;
          ; This code also handles global alias/func/history list allocation.
          ; If the previous shell has a global list and this one does too,
          ; we don't have to bother with size adjustments.  Otherwise we
          ; adjust the sizes as above, then save the adjusted size, and set
          ; the flags to reserve local or global space, or use the previous
          ; shell's global space.
          ;
          ;
          ; First adjust the environment
          ;
AdjEnv:
          Status    2,'AE'              ;(Adjust environment)
	mov	bl,InitFlag	;get flags
	push	bx		;save them
	bclr	InitFlag,XINHERIT	;environment is always inherited
          mov       ax,IFData.I_Env     ;get requested env size
          mov       dx,OldEnvSz	;get old size
          add       dx,EnvFree          ;add free space to old size to get
                                        ;  minimum space required in DX
          mov       bx,ENVMAX           ;get maximum
          mov       cx,ENVMIN           ;get minimum
          call      AdjOne              ;adjust the values
	pop	bx		;get back flags
	mov	InitFlag,bl	;restore
          mov       IFData.I_Env,ax     ;store new size
          mov       IFData.I_EnvNew,ax  ;save for OPTION to modify
          shrn      ax,4,cl             ;make paragraph count
          mov       EnvSizeP,ax         ;save paragraph count
          ;
          ; Now adjust the aliases
          ;
          Status    2,'AA'              ;(Adjust aliases)
          mov       ax,IFData.I_Alias   ;get requested alias size
          mov       dx,OldShell.DSwpAls.AHSwLen ;get old alias size, will be
                                        ;  0 in primary shell
          mov       bx,ALSMAX           ;get maximum
          mov       cx,ALSMIN           ;get minimum
          call      AdjOne              ;adjust the values
          mov       bx,offset AlsData   ;point to alias data
          mov       dl,IFData.I_LocalA  ;get local / global flag
          mov       dh,OldShell.DSwpAls.AHSwType  ;get previous type
          mov       si,OldShell.DSwpAls.AHSwLen   ;get previous length
          mov       di,OldShell.DSwpAls.AHSwSeg   ;get previous segment
          call      SetupAH             ;set up alias data
          mov       IFData.I_Alias,ax   ;store new size
          mov       IFData.I_AlsNew,ax  ;save for OPTION to modify
          ;
          ; Now adjust the functions
          ;
          Status    2,'AF'              ;(Adjust functions)
          mov       ax,IFData.I_Func	;get requested function size
          mov       dx,OldShell.DSwpFunc.AHSwLen ;get old function size, will be
                                        ;  0 in primary shell
          mov       bx,ALSMAX           ;get maximum
          mov       cx,ALSMIN           ;get minimum
          call      AdjOne              ;adjust the values
          mov       bx,offset FuncData   ;point to function data
          mov       dl,IFData.I_LocalF  ;get local / global flag
          mov       dh,OldShell.DSwpFunc.AHSwType  ;get previous type
          mov       si,OldShell.DSwpFunc.AHSwLen   ;get previous length
          mov       di,OldShell.DSwpFunc.AHSwSeg   ;get previous segment
          call      SetupAH             ;set up alias data
          mov       IFData.I_Func,ax    ;store new size
          mov       IFData.I_FuncNew,ax ;save for OPTION to modify
          ;
          ; The command history
          ;
          Status    2,'AH'              ;(Adjust command history)
          mov       ax,IFData.I_Hist    ;get requested cmd history size
          mov       dx,OldShell.DSwpHist.AHSwLen ;get old cmd history size,
                                        ;  will be 0 in primary shell
          mov       bx,HISTMAX          ;get maximum
          mov       cx,HISTMIN          ;get minimum
          call      AdjOne              ;adjust the values
          mov       bx,offset HistData  ;point to cmd history data
          mov       dl,IFData.I_LocalH  ;get local / global flag
          mov       dh,OldShell.DSwpHist.AHSwType  ;get previous type
          mov       si,OldShell.DSwpHist.AHSwLen   ;get previous length
          mov       di,OldShell.DSwpHist.AHSwSeg   ;get previous segment
          call      SetupAH             ;set up cmd history data
          mov       IFData.I_Hist,ax    ;store new size
          mov       IFData.I_HstNew,ax  ;save for OPTION to modify
          ;
          ; And the directory history
          ;
          Status    2,'AD'              ;(Adjust directory history)
          mov       ax,IFData.I_DHist   ;get requested dir history size
          mov       dx,OldShell.DSwpDir.AHSwLen ;get old history size, will
                                        ;  be 0 in primary shell
          mov       bx,DHISTMAX         ;get maximum
          mov       cx,DHISTMIN         ;get minimum
          call      AdjOne              ;adjust the values
          mov       bx,offset DirData   ;point to dir history data
          mov       dl,IFData.I_LocalD  ;get local / global flag
          mov       dh,OldShell.DSwpDir.AHSwType  ;get previous type
          mov       si,OldShell.DSwpDir.AHSwLen   ;get previous length
          mov       di,OldShell.DSwpDir.AHSwSeg   ;get previous segment
          call      SetupAH             ;set up dir history data
          mov       IFData.I_DHist,ax   ;store new size
          mov       IFData.I_DHNew,ax   ;save for OPTION to modify
          ;
          exit                          ;all done
          ;
          ; =================================================================
          ;
          ; AdjOne - Adjust one list size parameter
          ;
          ; On entry:
          ;         AX = Requested size
          ;         BX = Absolute maximum size
          ;         CX = Absolute minimum size
          ;         DX = Inherited minimum size
          ;
          ; On exit:
          ;         AX = Adjusted size
          ;         DX destroyed
          ;         Other registers and interrupt state unchanged
          ;
          entry     AdjOne,noframe,,local  ;adjust one list size
          assume    ds:@curseg          ;fix assumes
          ;
	test	InitFlag,XINHERIT	;do we care about inheritance?
	 jnz	AOUseAbs		;if not, use absolute minimum
          cmp       dx,cx               ;is inherited minimum big enough?
           jae      AOChkMin            ;yes, go on
          ;
AOUseAbs: mov       dx,cx               ;use absolute minimum
          ;
AOChkMin: cmp       ax,dx               ;check minimum
           jae      AOChkMax            ;if OK go on
          mov       ax,dx               ;if not use minimum
          ;
AOChkMax: cmp       ax,bx               ;check maximum
           jbe      AODone              ;if OK go on
          mov       ax,bx               ;if not use maximum
          ;
AODone:   add       ax,0Fh              ;add for roundoff
          and       ax,0FFF0h           ;round to nearest 16
          exit                          ;all done
          ;
          ; =================================================================
          ;
          ; SetupAH - Set up alias, function, and history data
          ;
          ; On entry:
          ;         AX = Block length for this shell
          ;         BX = Address of AHData structure
          ;         DH = Block type in previous shell
          ;         DL = Local / global flag for this shell
          ;         SI = Block length in previous shell
          ;         DI = Block segment in previous shell
          ;
          ; On exit:
          ;         AX = Actual block length for this shell (same as AX on
          ;           entry, unless both shells are global in which case this
          ;           is the same as [SI].AHSwLen on entry)
          ;         CX, DX destroyed
          ;         AHType, AHLenB, AHLenP, AHPType, AHPLenB, and AHPSeg
          ;           fields filled in in data structure
          ;         Other registers and interrupt state unchanged
          ;
          entry     SetupAH,noframe,,local  ;set up aliases / history
          assume    ds:@curseg          ;fix assumes
          ;
          mov       [bx].AHPType,dh     ;save previous type
          mov       [bx].AHPSeg,di      ;save previous segment
          mov       [bx].AHPLenB,si     ;save previous length
          cmp       dl,0                ;local in current shell?
           je       SAHGlob             ;if not, handle global
          mov       [bx].AHType,AHLOCAL ;set up for local aliases
          mov       [bx].AHLocB,ax      ;store local byte size
          jmp       SAHExit             ;and go on
          ;
SAHGlob:  cmp       dh,AHLOCAL          ;we're global, how about prev?
           je       SAHSetGN            ;if it was local, handle that
          mov       [bx].AHType,AHGLOLD ;set up for old global aliases
          mov       ax,si               ;both shells global - get old size
          jmp       SAHExit             ;and go on
          ;
SAHSetGN: mov       [bx].AHType,AHGLNEW ;set up for new global aliases
          ;
SAHExit:  mov       [bx].AHLenB,ax      ;store byte size
          mov       dx,ax               ;copy it
          shrn      dx,4,cl             ;convert to paragraphs
          mov       [bx].AHLenP,dx      ;save that as well
          ;
          exit                          ;all done
          ;
          ; =================================================================
          ;
          ; LenLoc - Set up all lengths and (where possible) locations for
          ;          the transient portion
          ;
          ; On entry:
          ;         No requirements
          ;
          ; On exit:
          ;         AX, BX, CX, DX, SI, DI destroyed
          ;         Other registers and interrupt state unchanged
          ;
          entry     LenLoc,noframe,,local  ;transient setup
          assume    ds:@curseg          ;fix assumes
          ;
          Status    2,'TA'              ;(Calculate transient portion addresses)
          ;
          ; Add up the alias, cmd history, dir history, and INI file data
          ; sizes and convert to an extra data paragraph count for loading
          ; the transient portion.
          ;
          mov       ax,(size INIData)   ;start with INI file data structure
          add       ax,IFData.I_StrMax  ;add INI file string length
          add       ax,INIKLen          ;add INI file keys length
          add       ax,0Fh              ;add for roundoff
          and       ax,0FFF0h           ;round to nearest 16
          mov       INILenB,ax          ;save it
          add       ax,AlsData.AHLocB   ;add local alias size
          add       ax,FuncData.AHLocB  ;add local function size
          add       ax,HistData.AHLocB  ;add local cmd history size
          add       ax,DirData.AHLocB   ;add local dir history size
          shrn      ax,4,cl             ;make paragraph count
          mov       ExtraPar,ax         ;save it
          ;
          ; Find the start of the transient portion and the necessary
          ; start and length values for its segments
          ;
          ; Calculate all transient portion segment addresses and lengths.
          ; This code is based on the following model of memory layout, and
          ; assumes that all segments are paragraph-aligned.  Note that all
          ; segments from ServSeg up may be moved and have the corresponding
          ; values readjusted when the transient portion is moved.
          ;
          ;                                    Load Seg     Final Seg
          ;         ÚÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄ¿
          ;         ³   Heap, including     ³
          ;         ³   aliases / history   ³
          ;         ÃÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄ´
          ;         ³       Transient       ³
          ;         ³       Stack           ³
          ;         ³       (Relocated)     ³
          ;         ÃÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄ´
          ;         ³       Transient       ³
          ;         ³       INI Data        ³  StackSeg
          ;         ÃÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄ´
          ;         ³       Transient       ³  _DATA or
          ;         ³       Data            ³  DGROUP       TDataSeg
          ;         ÃÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄ´
          ;         ³       Transient       ³
          ;         ³       Other Code      ³  _name_TEXT
          ;         ÃÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄ´
          ;         ³       Transient       ³
          ;         ³       Base Code       ³  _TEXT        TCodeSeg
          ;         ÃÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄ´
          ;         ³       Server          ³  SERV_TEXT    TranSeg, ServSeg
          ;         ÃÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄ´
          ;         ³       Loader          ³  LOAD_TEXT    TRelBase
          ;         ÃÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄ´
          ;         ³       PSP             ³               PSPSeg
          ;         ÀÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÙ
          ;
          ; First figure out and save the length of each area of the
          ; transient portion:  the server, the code, and the data (including
          ; data segment, stack, and heap)
          ;
          mov       si,SERV_TEXT        ;get server addr = start of transient
                                        ;  portion
          mov       TranAddr,si         ;save it
          mov       di,_TEXT            ;get code segment address
          mov       ax,di               ;copy code address
          sub       ax,si               ;server length = _TEXT - SERV_TEXT
          mov       ServLenP,ax         ;save it
          mov       CodeOffP,ax         ;save as code offset also
          mov       bx,ax               ;copy it
          shln      bx,3                ;convert to word count
          mov       ServLenW,bx         ;save that
          mov       dx,DGROUP           ;get data address
          mov       ax,dx               ;copy it
          sub       ax,di               ;code length = DGROUP - _TEXT
;** add length of language strings here??
          mov       CodeLenP,ax         ;save it
          mov       ax,dx               ;get back DGROUP
          sub       ax,si               ;DGROUP offset = DGROUP - SERV_TEXT
          mov       DataOffP,ax         ;save it
          mov       ax,StackSeg         ;get stack segment
          sub       ax,dx               ;data seg length = STACK - DGROUP
          add       ax,ExtraPar         ;add in alias / func / history / INI space,
                                        ;  this moves stack up to leave room
                                        ;  for these items
          mov       bx,StackOff         ;get original stack size in bytes
          mov       cx,StkSize          ;get INI file stack size
          cmp       bx,cx               ;is original bigger?
           ja       CalcStk             ;if so go on
          mov       bx,cx               ;if new size bigger use that
          ;
CalcStk:  mov       NewStack,bx         ;save as new stack size in bytes
          add       bx,0Fh              ;add for roundoff
          shrn      bx,4,cl             ;convert to paragraph length
          add       ax,bx               ;add to data length
          push      ax                  ;save offset of end of stack
          add       ax,HEAPADD          ;add any extra space for heap
	cmp	ax,0FFFh		;is it over 64K-1?
	 jb	LLDataOK		;if not, go on
	cmp	LLRetry,0		;if so, check if this is a retry
	 je	LLAdjust		;if not, go try again
          bset      LMFlags,FATALERR    ;if already retried, set fatal error
          error     DSIZE		;and give up
	;
	; Data segment too large, return to retry with reduced lists
	;
LLAdjust:	inc	LLRetry		;bump retry flag
	mov	dx,offset DLMsg	;complain about data segment size
	call	StatMsg		;display message
          call      GET_CHR             ;read a character and ignore it
          call      PUT_NEWLINE         ;do cr/lf
	pop	ax		;clear extra item on stack
	stc			;show problem
	jmp	LLDone		;and return
	;
	; Data segment fits!
	;
LLDataOK: mov       DataLenP,ax         ;save real DGROUP length
	mov       bx,ax               ;copy DGROUP length
          shln      bx,4,cl             ;make it a byte count
          mov       DataLenB,bx         ;save that
          add       ax,ServLenP         ;add server length
          mov       bx,ax               ;save length of DGROUP + server 
          add       ax,CodeLenP         ;add code length
          mov       TranLenP,ax         ;save as actual length of transient
          mov       TPLenP,ax           ;save for loader as well
          inc       TPLenP              ;bump for extra space at end
          push      ax                  ;save total
          ;
          ; Calculate the amount of data to swap, adjusted for reduced
          ; swapping (if any)
          ;
          test      Flags,NOREDUCE      ;reduced swap inhibited?
           jnz      SaveTot             ;if so go on
          mov       ax,bx               ;if reduced get length w/o code
          ;
SaveTot:  mov       SwapLenP,ax         ;save as total swap area size
          add       ax,63               ;add for 1K roundoff (64 paragraphs
                                        ;  per 1K)
          shrn      ax,6,cl             ;shift right 6 to get swap size in K
          mov       SwapLenK,ax         ;save it
          ;
          pop       ax                  ;get back total transient length
          pop       bx                  ;get back initial stack offset
          ;
          ; Save the new stack offset adjusted for the data segment length
          ; so that when we set SS = DS in the transient portion, SP will
          ; be in the right place.  This substitutes for a similar
          ; calculation normally done by the C startup code.
          ;
          shln      bx,4,cl             ;convert stack offset back to bytes
          sub       bx,2                ;reduce new stack offset by 2 as is
                                        ;  done in C startup
          and       bx,(not 1)          ;make it even per C startup
          mov       StackOff,bx         ;and save as new offset
          sub       bx,NewStack         ;get bottom of stack
          add       bx,STACKADJ         ;add in slop amount
          loadseg   es,DGROUP,dx        ;point to transient data segment
          mov       wptr es:[_STKHQQ],bx ;save adjusted stack overflow check
                                         ;  value
          ;
          ; Calculate the correct high-memory load address of the transient
          ; portion (will be adjusted later if we have to load resident)
          ;
          ;         Adjust for max load address, if any
          ;
          mov       bx,ds:[PSP_MEND]    ;get end of memory + 1 as target
          mov       dx,IFData.I_MaxLd   ;get max load address?
          or        dx,dx               ;test it
           je       AdjLoad             ;if no max address go on
          shln      dx,6,cl             ;convert max address to K
          cmp       bx,dx               ;are we below it?
           jbe      AdjLoad             ;if so go on
          mov       bx,dx               ;if not, limit to max load address
          ;
AdjLoad:  dec       bx                  ;leave one paragraph free at top
          ;
          ;         Adjust for temporary inheritance storage for global
          ;         aliases, functions, history, and directory history
          ;
          test      InitFlag,XINHERIT   ;inheritance disabled?
           jnz      SetHigh             ;if so go on
          mov       si,offset AlsData   ;get alias data pointer
          call      TempAH              ;make temp space if needed
          mov       si,offset FuncData  ;get function data pointer
          call      TempAH              ;make temp space if needed
          mov       si,offset DirData   ;get dir history data pointer
          call      TempAH              ;make temp space if needed
          mov       si,offset HistData  ;get history data pointer
          call      TempAH              ;make temp space if needed
          ;
SetHigh:  sub       bx,ax               ;subtract transient length to get
                                        ;  load address
          mov       THighAdr,bx         ;save high address for transient
          ;
          ; Calculate the size of the block to be moved when moving the
          ; transient portion up (or down).  This is smaller than the total
          ; length of the transient portion because all we need to move is
          ; what was loaded in the EXE file, not all the stack, heap, etc.
          ;
          ; This code also sets the INI file data address.
          ;
          mov       ax,offset DGROUP:_end  ;get end of data in loaded file
          add       ax,0Fh              ;add for roundoff
          shrn      ax,4,cl             ;convert to paragraph length
          mov       INIOffP,ax          ;save as INI data offset
          add       ax,CodeLenP         ;add code length
          add       ax,ServLenP         ;add server length
          mov       TranMovP,ax         ;save as transient move length
          ;
          ; Set the low-memory variables for break and INT 2E addresses
          ;
          mov       ax,offset SFBreak   ;get addr of server fake break code
          mov       ServFake,ax         ;save it locally
          mov       ax,offset I23Break  ;get addr of real break code
          mov       ServBrk,ax          ;save it locally
          mov       ax,offset I2EHdlr   ;get addr of server INT 2E handler
          mov       ServI2E,ax          ;save it locally
	clc			;show everything's OK
          ;
LLDone:	exit                          ;all done
          ;
          ; =================================================================
          ;
          ; TempAH - Adjusts the transient portion load address to leave
          ;          temporary space for alias/func/history inheritance.
          ;          Note space is reserved regardless of whether a UMB was
          ;          requested; this leaves room for the temp inheritance
          ;          blocks if no UMBs are available.
          ;
          ; On entry:
          ;         BX = current high end of load area (segment addr)
          ;         SI = address of alias / function / history data block
          ;
          ; On exit:
          ;         BX updated (reduced) if any space required
          ;         DX destroyed
          ;         Other registers and interrupt state unchanged
          ;
          entry     TempAH,noframe,,local  ;make temp inheritance space
          assume    ds:@curseg          ;fix assumes
          ;
          cmp       [si].AHType,AHGLNEW ;reserving global?
           jne      TADone              ;if not get out
          mov       dx,[si].AHPLenB     ;get amount to copy
          or        dx,dx               ;anything to copy?
           jz       TADone              ;if not forget it
          add       dx,0Fh              ;add for roundoff
          shrn      dx,4,cl             ;get paragraphs of temp space]
          sub       bx,dx               ;move down to accomodate it
          mov       [si].AHSeg,bx       ;save temp space address
          ;
TADone:   exit                          ;that's all
          ;
          ; =================================================================
          ;
          ; GetReloc - Get the relocation table appended to the end of the
          ;            4DOS file by APPREL
          ;
          ; On entry:
          ;         No requirements
          ;
          ; On exit:
          ;         AX, BX, CX, DX, SI, DI destroyed
          ;         Other registers and interrupt state unchanged
          ;
          entry     GetReloc,noframe,,local  ;get relocation table
          assume    ds:@curseg          ;fix assumes
          ;
          ; Find the relocation table.  The relocation table is appended
          ; to the executable file and is therefore found at the end of
          ; the data area, marked by the label _edata.
          ;
          Status    1,'PR'              ;(Process reloc table)
          mov       di,offset DGROUP:_edata  ;get end of data
          loadseg   es,DGROUP,ax        ;get DGROUP segment
          mov       si,offset RelocMrk  ;get mark offset
          mov       cx,RlMrkLen         ;get reloc table mark length
          cld                           ;go forward
          repe      cmpsb               ;see if the table is there
          errne     RELOC               ;if not give up big-time
          push      ds                  ;save DS
          loadseg   ds,es               ;get segment for table as source
          assume    ds:nothing          ;fix assumes
          pop       es                  ;our segment is destination
          mov       si,di               ;get current offset
          mov       di,offset RelocTab  ;point to local table
          xor       dx,dx               ;clear counter
          lodsw                         ;get total byte count
          mov       cx,ax               ;copy
           jcxz     RLDone              ;go on if nothing there
          cmp       cx,RELOCMAX         ;check against maximum
          erra      RLSIZE              ;holler if too big
          rep       movsb               ;move the table
          ;
RLDone:   loadseg   ds,cs               ;restore data segment
          assume    ds:@curseg          ;fix assumes
          ;
          exit                          ;all done
          ;
          ; =================================================================
          ;
          ; SwapSet - Set up swapping
          ;
          ; On entry:
          ;         No requirements
          ;
          ; On exit:
          ;         AX, BX, CX, DX, SI, DI destroyed
          ;         Other registers and interrupt state unchanged
          ;
          entry     SwapSet,varframe,,local  ;set up swapping
          ;
          varW      TokLen              ;saved token length
          varend
          ;
          assume    ds:@curseg          ;fix assumes
          ;
          cld                           ;everything goes forward here
          loadseg   es,ds               ;make ES local
          mov       si,IFData.I_Swap    ;get swapping pointer
          cmp       si,EMPTYSTR         ;was it set?
           je       SwDef               ;if not use default
          add       si,offset INIStDat  ;get offset of swapping string
          mov       di,offset SwapBrk   ;point to break table
          call      STR_PBRKN           ;skip delimiters at start
           jb       SwDef               ;if all delimiters use default
          jmp       short SwapPars      ;and go parse it
          ;
SwDef:    mov       al,CurDrive         ;get current drive as default
          cmp       CSpecLen,0          ;any COMSPEC?
           je       SwSetDrv            ;if not use current drive
          mov       al,bptr CSPath      ;get first byte of COMSPEC
          ;
SwSetDrv: call      UpShift             ;shift to upper case
          cmp       al,'B'              ;is it a floppy?
           ja       SWSavDrv            ;if not go on
          xor       al,al               ;it's a floppy, terminate the string
                                        ;  here so disk swapping is disabled
          ;
SwSavDrv: mov       SwDefD,al           ;put drive in buffer
          mov       si,offset SwDeflt   ;if not get default
          ;
SwapPars: mov       di,offset SwapBrk   ;point to break table
          call      STR_PBRKN           ;skip delimiters at start
           jb       SwapFJmp            ;if all delimiters use default
          call      STR_CSPN            ;find length of token
          or        cx,cx               ;check length
           jnz      SwapTry             ;if found a token see what it is
          ;
SwapFJmp: jmp       SwapFail            ;if no token there load resident
          ;
SwapTry:  mov       TokLen,cx           ;save token length
          mov       di,offset SwapTLst  ;get swap type list
          call      dword ptr TokListP  ;see if it's in the list
           ja       SwNUniq             ;holler if it's not unique
           jb       SwPath              ;if not there at all try a path
          dec       al                  ;check type
           jl       SwEMS               ;negative, was zero, try EMS
           je       SwXMS               ;zero, was 1, try XMS
          ;
          ; TokList returned 2 = no swapping
          ;
          mov       dx,offset NoneMsg   ;point to no swapping message
          jmp       SwNone              ;and go display it
          ;
          ; Try EMS
          ;
SwEMS:    call      SetEMS              ;call the EMS setup routine
           jc       SwapNext            ;if it fails try something else
          mov       al,SWAPEMS          ;show EMS swapping
          mov       dx,offset EMSMsg    ;get EMS message
          jmp       short SwapOK        ;and go display it
          ;
          ; Try XMS
          ;
SwXMS:    call      SetXMS              ;call the XMS setup routine
           jc       SwapNext            ;if it fails try something else
          mov       al,SWAPXMS          ;show XMS swapping
          mov       dx,offset XMSMsg    ;get XMS message
          jmp       short SwapOK        ;and go display it
          ;
          ; Non-unique swap type, complain
          ;
SWNuniq:  push      si                  ;save token address
          mov       di,offset TempPath  ;point to temporary buffer
          push      di                  ;save for display
          rep       movsb               ;copy string there
          stocb     0                   ;null-terminate
          jmp       short PathErr       ;go display error
          ;
          ; See if we have a path for disk swapping
          ;
SwPath:   push      si                  ;save swapping string pointer
          mov       di,offset TempPath  ;point to temporary path buffer
          push      di                  ;save that
          rep       movsb               ;move token to buffer
          stocb     0                   ;null-terminate
          pop       si                  ;get back buffer address
          push      si                  ;save it again
          call      FNCheck             ;check the path
           jc       PathErr             ;complain if error
          pop       si                  ;get buffer address again
          mov       bx,dx               ;copy path portion length
          mov       bptr [si][bx],0     ;null-terminate the path
          mov       cx,dx               ;copy length for SetDisk
          call      SetDisk             ;try to set up disk swapping
           jnc      SwDiskOK            ;if it's OK go on
          pop       si                  ;restore swapping string pointer
          jmp       short SwapNext      ;and keep parsing
          ;
PathErr:  mov       si,offset SwBad1    ;point to error message
          call      PUT_STR             ;display it
          pop       si                  ;get back buffer address
          call      PUT_STR             ;display bad option
          mov       si,offset SwBad2    ;point to end of error message
          call      PUT_STR             ;display it
          pop       si                  ;restore swapping string pointer
          jmp       short SwapNext      ;try next token
          ;
          ; Disk swapping OK, display message
          ;
SwDiskOK: pop       si                  ;make stack right
          mov       al,SWAPDISK         ;get disk swap type
          pushm     ax,bx               ;save swap type and handle
          mov       dx,offset DiskMsg   ;tell user we're using disk
          call      StatMsg             ;display message
          test      InitFlag,TRANLOAD   ;transient load?
           jnz      SwapDSz             ;if so skip file name output (StatMsg
                                        ;  skips everything else)
          mov       si,offset SwapPath  ;point to path
          call      PUT_STR             ;write it
          jmp       short SwapDSz       ;and go display size
          ;
          ; Move to next token
          ;
SwapNext: add       si,TokLen           ;skip token
          lodsb                         ;get delimiter and skip past it
          or        al,al               ;end of swapping string?
           je       SwapFail            ;if so we failed
          jmp       SwapPars            ;and keep parsing
          ;
          ; Swapping OK -- display message and size
          ;
SwapOK:   pushm     ax,bx               ;save swap type and handle
          call      StatMsg             ;display (end of) swap message
          ;
SwapDSz:  mov       dx,offset SizeMsg   ;point to " ("
          call      StatMsg             ;display it
          mov       ax,SwapLenK         ;get swap size in paragraphs
          mov       dx,offset SizeBuf   ;point to buffer
          mov       di,dx               ;copy for output
          loadseg   es,ds               ;use our segment
          call      DecOutWU            ;output size to buffer
          stocw     ')K'                ;output "K)"
          stocw     <CR + (LF shl 8)>   ;output CR / LF
          stocb     '$'                 ;output terminator
          call      StatMsg             ;display size and ")"
          bset      LMFlags,SWAPENAB    ;show swap enabled
          popm      bx,ax               ;restore swap type and handle
          mov       SwMethod,al         ;save swap method
          mov       LMHandle,bx         ;save handle
          mov       di,offset LocDev    ;point to device SCB
          call      DefDev              ;define device SCB
          jmp       short SwapExit      ;and exit
          ;
          ; Swapping failed
          ;
SwapFail: mov       dx,offset FailMsg   ;tell user we're not swapping
          ;
SwNone:   mov       SwMethod,SWAPNONE   ;set method
          call      StatMsg             ;display non-swapping message
          ;
SwapExit: exit                          ;all done
          ;
          ; =================================================================
          ;
          ; SetXMS - Set up XMS swapping if possible
          ;
          ; On entry:
          ;         No requirements
          ;
          ; On exit:
          ;         Carry clear if swapping set up, set if not
          ;         BX = XMS handle if successful
          ;         AX, CX, DX destroyed
          ;         All other registers and interrupt state unchanged
          ;
          entry     SetXMS,noframe,,local  ;set up XMS swap
          assume    ds:@curseg          ;fix assumes
          ;
          Status    1,'XS'              ;(Try XMS swap)
          test      InitFlag,XMSAVAIL   ;is XMS available?
           jz       NoXMS               ;if not there go on
          mov       dx,SwapLenK         ;get swap area size in K
          callxms   ALLOC,XMSDrvr       ;allocate pages
          or        ax,ax               ;any error?
           jz       NoXMS               ;if so don't use XMS
          Status    2,'XU'              ;(Using XMS swap)
          bset      ModXMS.ModFlags,LOADME  ;load XMS swap in code
          ;
          ; Set up control block for swapping server in from XMS
          ;
          mov       ax,ServLenP         ;get server length
          shln      ax,4                ;make it bytes
          mov       wptr XMSMove.MoveLen,ax   ;save in move data
          xor       ax,ax               ;get zero
          mov       wptr XMSMove.MoveLen[2],ax   ;clear high-order length
          mov       XMSMove.SrcHdl,dx   ;set source handle for XMS
          mov       XMSMove.SrcOff,ax   ;clear source offset
          mov       XMSMove.SrcSeg,ax   ;clear source segment
          mov       XMSMove.DestHdl,ax  ;clear destination handle
          mov       XMSMove.DestOff,ax  ;clear destination offset
          ;
          ; XMS setup done
          ;
          mov       bx,dx               ;copy handle
          clc                           ;show XMS is OK
          jmp       short SwXExit       ;and exit
          ;
          ; XMS not available
          ;
NoXMS:
          Status    2,'XF'              ;(XMS swap failed)
          stc                           ;show error
          ;
SwXExit:  exit                          ;all done
          ;
          ; =================================================================
          ;
          ; SetEMS - Set up EMS swapping if possible
          ;
          ; On entry:
          ;         No requirements
          ;
          ; On exit:
          ;         Carry clear if swapping set up, set if not
          ;         BX = EMS handle if successful
          ;         AX, CX, DX destroyed
          ;         All other registers and interrupt state unchanged
          ;
          entry     SetEMS,noframe,,local  ;set up EMS swap
          assume    ds:@curseg          ;fix assumes
          ;
          ; Reserve space for swapping in EMS memory if available
          ;
          Status    1,'ES'              ;(Try EMS swap)
          test      InitFlag,EMSAVAIL   ;is EMS available?
           jz       NoEMS               ;if not there go on
          mov       bx,SwapLenK         ;get swap area size in K
          add       bx,0Fh              ;add for 16K roundoff
          and       bx,0FFF0h           ;round it to get actual size in K
          push      bx                  ;save actual size
          mov       cl,4                ;get shift count
          shr       bx,cl               ;make 16K page count
          callems   ALLOC               ;allocate pages
          or        ah,ah               ;any error?
           jnz      EMSErr              ;if so don't use EMS
          Status    2,'EU'              ;(Using EMS swap)
          loadseg   es,ds               ;set ES for shell number store
          mov       si,offset EMSName   ;point to handle name buffer
          lea       di,7[si]            ;point to location for last digit
          call      ShellOut            ;put shell number in name
          mov       al,1                ;set handle name subfunction
          callems   HNAME               ;set handle name, ignore any error
          pop       SwapLenK            ;save actual size of EMS block
          bset      ModEMS.ModFlags,LOADME  ;load EMS swap in code
          ;
          ; EMS setup done
          ;
          mov       bx,dx               ;copy handle
          clc                           ;show EMS is OK
          jmp       short SwEExit       ;and exit
          ;
          ; EMS not available
          ;
EMSErr:   pop       ax                  ;throw away actual EMS size in K
          ;
NoEMS:
          Status    2,'EF'              ;(EMS swap failed)
          stc                           ;show error
          ;
SwEExit:  exit                          ;all done
          ;
          ; =================================================================
          ;
          ; SetDisk- Set up disk swapping if possible
          ;
          ; On entry:
          ;         CX = length of swap path
          ;         DS:SI = pointer to swap path (backslash-terminated)
          ;
          ; On exit:
          ;         Carry clear if swapping set up, set if not
          ;         BX = file handle if successful
          ;         AX, CX, DX destroyed
          ;         All other registers and interrupt state unchanged
          ;
          entry     SetDisk,varframe,,local  ;set up disk swap
          assume    ds:@curseg          ;fix assumes
          ;
          varW      USwCount            ;unique name retry count
          varW      SavPEnd             ;saved end of path location
          varend
          ;
          ; Disk swap -- set up swap file
          ;
          Status    1,'DS'              ;(Try disk swap)
          Status    2,'DN'              ;(Create swap file name)
          cld                           ;go forward here
          loadseg   es,ds               ;es = data seg
          mov       di,offset SwapPath  ;point to swap file name buffer
          push      di                  ;save it
          rep       movsb               ;copy path to buffer
          cmp       IFData.I_USwap,0    ;unique swap file names?
           je       StdName             ;if not go use standard name
          cmp       DosMajor,3          ;unique names possible?
           jb       StdName             ;if not go use standard name
          ;
          ; Open uniquely named swap file
          ;
          Status    2,'DQ'              ;(Open unique swap file)
          pop       dx                  ;point to start of name
          mov       bptr USwCount,URETRIES  ;set retry count
          ;
USLoop:   mov       bptr [di],0         ;terminate path after backslash
          mov       cx,(ATR_HIDE or ATR_SYS)  ;open new swap file as a hidden
                                             ;  system file
;;LFN
          calldos   UNIQUE              ;try to open it w/ unique name
           jnc      UniqOpen            ;if OK go on
          dec       bptr USwCount       ;if not count retries
           jnz      USLoop              ;loop and try again
          ;
FailJmp:  jmp       DiskFail            ;if error we can't use disk
          ;
UniqOpen: mov       SavPend,di          ;save end of path if we have to retry
          call      SetDSLen            ;set file length and close it
           jc       FailJmp             ;if error we can't use disk
          mov       si,offset SwapPath  ;get unique name address
          call      STR_LEN             ;get name length
          pushm     cx,si               ;save length and address
          inc       cx                  ;be sure to move null too
          mov       ax,cx               ;copy count with null
          add       ax,USwExtL          ;add extension length
          mov       SwapPLen,ax         ;save as swap path length
          mov       dx,offset TempPath  ;get temp buffer address (in DX for
                                        ;  rename below)
          mov       di,dx               ;copy for string copy
          rep       movsb               ;copy old name to temp buffer
          popm      di,cx               ;get back swap path addr and length
          push      di                  ;save address again
          add       di,cx               ;skip to end
          mov       si,offset USwExt    ;point to extension
          mov       cx,USwExtL          ;get its length
          rep       movsb               ;move extension into place
          pop       di                  ;get back original address
;;LFN
          calldos   RENAME              ;try to rename to .4SW extension
           jnc      DSReOpen            ;if it worked go on
          mov       dx,offset TempPath  ;get original name address
;;LFN
          calldos   DELETE              ;rename failed, delete original
          dec       bptr USwCount       ;count retries
           jz       FailJmp             ;if out of retries, give up
          mov       dx,offset SwapPath  ;point to swap path again
          mov       di,SavPEnd          ;get saved end of path for retry
          jmp       short USLoop        ;loop and try again
          ;
          ; Disk swapping using standard name (4DOSSWAP.nnn)
          ;
StdName:  moveb     ,<offset SwapName>,,,SwapNL  ;copy name to buffer
          mov       cx,di               ;get end address
          pop       ax                  ;get back start address
          sub       cx,ax               ;calculate path length
          mov       SwapPLen,cx         ;save swap path length
          neg       cx                  ;get -length
          add       cx,PATHBLEN         ;get bytes remaining in SwapPath
           jcxz     PutExt              ;if none just go dump in extension
          xor       al,al               ;get null
          push      di                  ;save address of first null
          rep       stosb               ;clear rest of path to nulls
          pop       di                  ;restore address
          ;
PutExt:   dec       di                  ;point to last byte of extension
          call      ShellOut            ;put shell number as file extension
          ;
          ; Open standard name swap file, make it big enough
          ;
          Status    2,'DO'              ;(Open swap file)
          mov       dx,offset SwapPath  ;point to name
          mov       cx,(ATR_HIDE or ATR_SYS)  ;open new swap file as a hidden
                                             ;  system file
;;LFN
          calldos   CREATE              ;try to create or truncate it
           jc       DiskFail            ;if error we can't use disk
          ;
          Status    2,'DL'              ;(Set swap file length)
          call      SetDSLen            ;set the file to the right length
           jc       DiskFail            ;if error we can't use disk
          ;
          ; Re-open the swap file with proper sharing and inheritance bits
          ;
          Status    2,'DX'              ;(Reopen swap file)
DSReOpen: mov       dl,ACC_RDWR         ;set for read/write access
          cmp       DosMajor,3          ;DOS 3 or later?
           jl       DoReOpen            ;if not go on
          or        dl,(SHR_INH + SHR_DRW)  ;in DOS 3 open it in deny read/
                                            ;  write mode, no inheritance
          ;
DoReOpen: mov       al,dl               ;copy access bits
          mov       ROAccess,al         ;save for use if reopen enabled
          mov       dx,offset SwapPath  ;point to name
;;LFN
          calldos   OPEN                ;re-open the file
           jc       DiskFail            ;if error we can't use disk
          mov       bx,ax               ;copy handle
          calldos   GETTIME             ;get current time in CX / DX
          mov       wptr DSigTime,dx    ;store seconds / hundredths
          mov       wptr DSigTime+2,cx  ;store hour / minute
          Status    2,'DG'              ;(Write swap file signature)
          mov       dx,offset DiskSig   ;point to signature
          mov       cx,SIGBYTES         ;get signature length
          calldos   WRITE,SWWRIT        ;write the block to disk
          ;
          ; Disk swapping is all set up, tell the user
          ;
          Status    2,'DU'              ;(Use disk swap)
          bset      ModDisk.ModFlags,LOADME  ;load disk swap in code
          clc                           ;show all OK
          jmp       short DiskExit      ;and return (handle is in BX)
          ;
          ; Disk file operation failed -- try to close and delete file
          ;
DiskFail: calldos   CLOSE               ;try to close it, ignore errors
          mov       dx,offset SwapPath  ;point to name
;;LFN
          calldos   DELETE              ;try to delete it, ignore errors
          mov       SwapPLen,0          ;no swap path
          stc                           ;show error
          ;
DiskExit: exit                          ;all done
          ;
          ; =================================================================
          ;
          ; SetDSLen - Set disk swap file length
          ;
          ; On entry:
          ;         AX = file handle
          ;
          ; On exit:
          ;         Carry flag clear if all OK, set if error
          ;         AX, BX, CX, DX, SI, DI destroyed
          ;         Other registers and interrupt state unchanged
          ;         File set to proper length and closed
          ;         Swap file open flag set
          ;
          entry     SetDSLen,noframe,,local  ;set swap file length
          assume    ds:@curseg          ;fix assumes
          ;
          bset      Flags,DISKOPEN      ;show swap file is open
          mov       bx,ax               ;copy handle
          mov       dx,SwapLenP         ;get swap area length in paragraphs
          xor       cx,cx               ;clear high word
          dshln     dx,cx,4             ;shift to get byte count
          add       dx,(SIGBYTES - 1)   ;point to (end - 1) + length of
                                        ;  signature to get total file length
          adc       cx,0                ;handle any carry
          mov       al,SEEK_BEG         ;start at beginning
          calldos   SEEK                ;move pointer to end
           jc       SDLError            ;if error we can't use disk
          mov       cx,1                ;get byte count (1-byte write just
                                        ;  sets EOF)
          xor       dx,dx               ;dummy output area
          calldos   WRITE               ;write 1 byte to force file length
           jc       SDLError            ;if error we can't use disk
          cmp       ax,1                ;check bytes written
           jne      SDLError            ;if not right disk is full
          Status    2,'DC'              ;(Close swap file)
          calldos   CLOSE               ;close (sets file length)
           jnc      SDLDone             ;if no error get out
          ;
SDLError: stc                           ;show error
          ;
SDLDone:  exit                          ;that's all
          ;
          ; =================================================================
          ;
          ; LoadRes - Load 4DOS in resident (non-swapping) mode
          ;
          ; On entry:
          ;         No requirements
          ;
          ; On exit:
          ;         AX, BX, CX, DX, SI, DI destroyed
          ;         Other registers and interrupt state unchanged
          ;
          entry     LoadRes,noframe,,local  ;load resident
          assume    ds:@curseg          ;fix assumes
          ;
          ; No swapping -- slide 4DOS down to just above the loader.  This
          ; takes place in the following sequence:
          ;
          ;         1) Copy the local server data to the transient portion.
          ;         2) Reduce memory allocation so the current block ends
          ;            at the end of the transient portion
          ;         3) Allocate space for the slider (this allocation will
          ;            be above the loaded copy of the transient portion).
          ;         4) Set up a new stack in the slider's memory block and
          ;            switch to it.
          ;         5) Push the 4DOS initialization data onto the new stack.
          ;         6) Push the slider data onto the new stack.
          ;         7) Transfer control to the slider, which will:
          ;         8) Slide the transient portion down to just above the
          ;            "resident" portion.
          ;         9) Free its own memory block (the block reserved in step
          ;            (3) above).
          ;        10) Initialize the registers for 4DOS.
          ;        11) Transfer control to 4DOS.
          ;
          ; Clean up flags, set up all chaining and interrupt hooks, move
          ; data to EXE area, allocate or set up for any data blocks
          ;
          and       Flags,NOREDUCE      ;clear flags, preserve NOREDUCE
          call      LoadMod             ;load all necessary loadable modules,
                                        ;  set resident size
          mov       ax,cs               ;get local segment
          add       ax,ResSize          ;add resident length to get new
                                        ;  segment for transient portion
          push      ax                  ;save for slider destination
          mov       TranSeg,ax          ;save as actual transient segment
          mov       IFData.I_HiSeg,ax   ;save as high segment for C code
          call      DataBlks            ;set up master env, global aliases,
                                        ;  functions, & history
          ;
          ; Do relocation (assumes AX is set to transient load segment
          ; from above)
          ;
          Status    2,'LR'              ;(Relocate for resident mode)
          call      DoReloc             ;do the relocation
          call      SetupICB            ;set up inheritance blocks
          ;
          call      SetChain            ;set up chains and interrupts
          mov       ax,SERV_TEXT        ;get segment for server data
          call      InitTran            ;initialize transient portion
          mov       ax,INISeg           ;get INI file data segment
          mov       ShellInf.DSwpMLoc,ax  ;set for secondary shells
          ;
          ;
          ; Adjust memory allocation so the current block ends at the end
          ; of the transient portion
          ;
          Status    2,'AM'              ;(Adjust memory allocation)
          mov       bx,TranAddr         ;get load segment
          add       bx,TranLenP         ;get end of transient portion + 1
          mov       ax,cs               ;get local segment
          sub       bx,ax               ;get new length for block
          mov       es,ax               ;get segment to adjust
          calldos   REALLOC,NOMEM       ;reduce size
          ;
          ; Allocate space for the slider code and move it up there
          ;
          Status    1,'MS'              ;(Move slider)
          mov       bx,SlidePar         ;get length of slider code
          calldos   ALLOC,NOMEM         ;allocate space for it
          mov       dx,ax               ;save slider segment
          movew     ,<offset Slider>,ax,<SlideLoc>,SlideLen
                                        ;move the slider code
          pop       cx                  ;get back destination segment
          ;
          ; Set up a new stack in the slider segment
          ;
          Status    2,'SS'              ;(Set up slider stack)
          cli                           ;hold interrupts
          mov       ss,dx               ;copy slider stack segment
          mov       sp,SlStkTop         ;set slider stack location
                                        ;  (now stack is in slider segment)
          sti                           ;and allow interrupts
          ;
          ; Save info needed to copy and start 4DOS, then branch to the
          ; slider
          ;
          Status    2,'SP'              ;(Save slider parameters)
          push      TCodeSeg            ;initial CS
          mov       ax,offset _TEXT:__astart  ;get C entry point address
          push      ax                  ;save as initial IP
          push      DataLenB            ;save DGROUP length
          push      StackOff            ;initial SP
          push      TDataSeg            ;save DGROUP segment as initial SS/DS
          mov       ax,ResSize          ;get resident length
          add       ax,TranLenP         ;add transient length to get new
                                        ;  total memory block length
          push      ax                  ;save that too
          mov       bx,cs               ;copy PSP segment
          add       ax,bx               ;get new end of memory block
          push      ax                  ;save for slider to stuff in PSP
          push      cs                  ;save psp segment for slider to use
          push      cx                  ;save destination segment for move
          push      TranAddr            ;source segment for move
          push      TranLenP            ;paragraph count for slider
          push      dx                  ;slider code segment
          mov       ax,SlideLoc         ;get slider offset within segment
          push      ax                  ;store it
          Status    1,'SL'              ;(Slide transient down & start it)
          retf                          ;and branch to slider code
          ;
          exit                          ;all done (never gets here)
          ;
          ; =================================================================
          ;
          ; DoReloc - Relocate transient code
          ;
          ; On entry:
          ;         AX = segment address where transient portion will go
          ;
          ; On exit:
          ;         AX, BX, CX, DX, SI, DI, ES destroyed
          ;         All other registers and interrupt state unchanged
          ;
          ;
          entry     DoReloc,noframe,,local  ;set up entry point
          assume    ds:@curseg          ;fix assumes
          ;
          mov       bx,LOAD_TEXT        ;get segment base to use to find
                                        ;  relocatable items
          mov       dx,ax               ;get new load segment
          sub       dx,SERV_TEXT        ;adjustment amount is difference of
                                        ;  new base and original base
;	add	dx,bx		;adjust for relocation of SERV_TEXT
;				;  reference above
          mov       si,offset RelocTab  ;point to table
          call      Reloc               ;fix up all relocations
          ;
          exit                          ;all done
          ;
          ; =================================================================
          ;
          ; LOADMOD - Load all necessary loadable modules, calculate
          ;           size of resident portion
          ;
          ; On entry:
          ;         DS = CS
          ;
          ; On exit:
          ;         DS preserved
          ;         All other registers destroyed, interrupt state unchanged
          ;         Loads modules and sets up ResSize
          ;
          entry     LoadMod,noframe,,local  ;set up loadable modules
          assume    ds:@curseg          ;fix assumes
          ;
          ; Figure out about loading root-shell related stuff:  INT 2E,
          ; COMMAND.COM message handler
          ;
          Status    1,'LM'              ;(Load resident modules)
          ;
          ; Decide whether to load the INT 2E handler and COMMAND.COM message
          ; handler
          ;
          cld                           ;go forward
          ;
          test      LMFlags,ROOTFLAG    ;are we the root shell?
           jz       NoMsgHdl            ;if not go disable msg handler, skip
                                        ;  INT 2E
          mov       bx,offset ModI2ES   ;assume standard INT 2E
          cmp       IFData.I_Int2E,0    ;is full INT 2E support set?
           je       Set2E               ;if not go on
          mov       bx,offset ModI2EF   ;get full INT 2E address
          ;
Set2E:    bset      [bx].ModFlags,LOADME  ;load INT 2E handler
          cmp       DosMajor,4          ;DOS 4 or above?
           jb       NoMsgHdl            ;if not kill message handler
          cmp       DosMajor,10         ;OS/2 DOS box?
           jae      NoMsgHdl            ;if so kill message handler
          test      LMFlag2,NOCCMSG     ;message handler disabled?
           jnz      ChkZero             ;if so go on
          ;
          bset      ModCMIni.ModFlags,LOADME  ;load CC msg handler init code
          bset      ModCMHdl.ModFlags,LOADME  ;load CC msg handler code
          bset      ModPErr.ModFlags,LOADME   ;load parse error text
          jmp       short ChkZero       ;and go check for shell 0
          ;
NoMsgHdl: bset      LMFlag2,NOCCMSG     ;disable message handler
          ;
          ; Decide whether to load the shell table
          ;
ChkZero:  cmp       ShellNum,0          ;are we shell 0 (1st copy of 4DOS)?
           jne      ChkSec              ;if not no shell table
          bset      ModShDat.ModFlags,LOADME  ;load shell table data
          bset      ModShMan.ModFlags,LOADME  ;load new shell code
          ;
          ; Decide whether to load the critical error text
          ;
ChkSec:   test      Flags,NOREDUCE      ;using reduced size?
           jz       ChkRO               ;if so skip critical error load
          bset      ModCrit.ModFlags,LOADME  ;load critical error text
          ;
          ; Decide whether to load the swap file reopen code
          ;
ChkRO:    cmp       SwMethod,SWAPDISK   ;swap to disk?
           jne      NoReOpen            ;if not go on
          test      LMFlag2,DREOPEN     ;reopen requested?
;           jz       CheckNN             ;if not go on
           jz       ModGo               ;if not go on
          mov       si,offset SwapPath  ;point to swap path
          mov       di,offset ROPath    ;point to reopen path
          loadseg   es,ds               ;set destination segment
          mov       cx,SwapPLen         ;get byte count
          cld                           ;go forward
          rep       movsb               ;copy path for reopen
          mov       ReopenP.fseg,cs     ;fill in segment in dword ptr
          bset      ModRODat.ModFlags,LOADME  ;load reopen data
          bset      ModReOpn.ModFlags,LOADME  ;load reopen code
;          jmp       short CheckNN       ;go on
          jmp       short ModGo         ;go on
          ;
NoReOpen: bclr      LMFlag2,DREOPEN     ;if not disk swapping kill reopen
          ;
          ; Load the modules
          ;
ModGo:    mov       di,offset ModArea   ;get starting location for first one
          loadseg   es,ds               ;all in one segment here
          cld                           ;moves go forward
          mov       dl,ModCount         ;get module count
          mov       bx,offset ModTable  ;point to table
          ;
ModLoop:  test      [bx].ModFlags,LOADME  ;loading this one?
          jz        NextMod             ;if not go on
          mov       cx,[bx].ModLen      ;get module length in bytes
          mov       ax,cx               ;copy it
          add       ax,di               ;get end + 1
          cmp       ax,offset ModGuard  ;past guard point?
          erra      MODFIT              ;if so we're in big trouble!
          mov       si,[bx].ModPtr      ;get pointer address
          mov       [si],di             ;save module's starting location
          mov       si,[bx].ModStart    ;get starting offset
          rep       movsb               ;load the module
          ;
          if        DEBUG ge 2          ;only if debugging
          push      [bx].ModText        ;put message address on stack
          call      DebugMsg            ;display message if display on
          endif                         ;debugging code
          ;
NextMod:  add       bx,size ModEntry    ;move to next table entry
          dec       dl                  ;count modules
          ja        ModLoop             ;if more go load the next one
          ;
          ; Copy Netware Names if loading resident
          ;
          cmp       IFData.I_NName,0    ;load Netware dummy names?
           je       LMSetSiz            ;if not go on
          cmp       SwMethod,SWAPNONE   ;are we swapping?
           jne      LMSetSiz            ;if so go on
          Status    2,'NR'              ;(Copy resident Netware Names)
          mov       cx,NNLen            ;get Netware Names module length
          mov       si,offset NNData    ;get start address
          rep       movsb               ;copy names to end of low memory area
          ;
LMSetSiz: add       di,0Fh              ;increment last transfer address for
                                        ;  paragraph roundoff
          shrn      di,4,cl             ;make it a paragraph count
          mov       ResSize,di          ;save it as resident portion size
          ;
          ; Adjust pointer inside critical and parse error tables for move
          ;
          mov       si,offset ModCrit   ;point to critical error info
          mov       di,BadMsg           ;get offset of word to fix
          call      ModAdj              ;go adjust it
          mov       si,offset ModPErr   ;point to critical error info
          call      ModAdj              ;go adjust it (DI remains from above)
          ;
          exit                          ;all done
          ;
          ; =================================================================
          ;
          ; ModAdj - Adjust a word inside a loadable module for the
          ; distance the module was moved
          ;
          ; On entry:
          ;         SI = address of the loadable module table entry
          ;         DI = address of the word within the module to be
          ;              adjusted
          ;
          ; On exit:
          ;         All registers and interrupt state unchanged
          ;
          ;
          entry     ModAdj,noframe,,local  ;set up entry point
          assume    ds:@curseg          ;fix assumes
          ;
          pushm     ax,bx               ;save regs used
          test      [si].ModFlags,LOADME  ;was module loaded?
          jz        MAExit              ;if not skip it
          mov       bx,[si].ModPtr      ;get pointer address
          mov       bx,[bx]             ;get pointer value = new address
          mov       ax,[si].ModStart    ;get original address
          sub       ax,bx               ;get move distance
          sub       [bx][di],ax         ;adjust internal pointer in module
          ;
MAExit:   popm      bx,ax               ;restore regs used
          exit                          ;all done
          ;
          ; =================================================================
          ;
          ; GetDREnv - Get any DR-DOS environment variables set in CONFIG.SYS
          ;
          ; On entry:
          ;         No requirements.
          ;
          ; On exit:
          ;         DS preserved
          ;         All other registers destroyed, interrupt state unchanged
          ;         DR-DOS env variables (if any) copied to TempStor area
          ;
          ;
          entry     GetDREnv,noframe,,local  ;set up entry point
          assume    ds:@curseg          ;fix assumes
          ;
          test      LMFlags,ROOTFLAG    ;are we the true root shell?
           jz       GDRExit             ;if not, forget about DR variables
          cmp       IFData.I_DRSets,0   ;are DR-DOS SETs enabled?
           je       GDRExit             ;if not get out
          mov       ax,DRCHECK          ;get DR-DOS check code
          calldos                       ;check for it
           jc       GDRExit             ;if not DR-DOS go on
          mov       ax,DRSTART          ;code to get startup data address
          calldos                       ;get the address
           jc       GDRExit             ;if can't get it go on
          mov       ax,es:12h[bx]       ;get DR env segment
          mov       dx,ds:[PSP_MEND]    ;get end of memory
          sub       dx,800h             ;address of last 32K
          cmp       ax,dx               ;is addr in last 32K (sanity check)?
           jb       GDRExit             ;if not forget it
	test	InitFlag,PERMLOAD	;check if /P
	 jz	GDRCopy		;if not go on
          mov       wptr es:12h[bx],0	;clear pointer to show env used
	;
GDRCopy:	cld                           ;go forward
          loadseg   es,ds               ;destination is local segment
          push      ds                  ;save local segment
          mov       ds,ax               ;source is DR segment
          xor       si,si               ;clear source offset
          mov       di,offset TempStor  ;get temporary storage location
          ;
GDRLoop:  lodsb                         ;get first byte of variable
          stosb                         ;store it
          or        al,al               ;end of env?
           jz       GDRDone             ;if so stop
          ;
GDRLoop2: cmp       si,DRENVMAX         ;do we have room for more?
           ja       GDRError            ;if not give up
          lodsb                         ;get byte of variable name / data
          stosb                         ;store it
          or        al,al               ;end of variable?
           jnz      GDRLoop2            ;if not continue
          jmp       short GDRLoop       ;if so loop for next variable
          ;
GDRError: mov       dx,offset GDREMsg   ;point to error message
          call      StatMsg             ;display it
          xor       si,si               ;show no DR env variables
          ;
GDRDone:  pop       ds                  ;restore local segment
          mov       DREnvLen,si         ;save length
          ;
GDRExit:  exit                          ;all done
          ;
          ; =================================================================
          ;
          ; MvDREnv - Move any DR-DOS environment variables to transient
          ;           portion
          ;
          ; On entry:
          ;         No requirements.
          ;
          ; On exit:
          ;         DS preserved
          ;         All other registers destroyed, interrupt state unchanged
          ;         DR-DOS env variables (if any) copied to TempStor area
          ;
          ;
          entry     MvDREnv,noframe,,local  ;set up entry point
          assume    ds:@curseg          ;fix assumes
          ;
          mov       cx,DREnvLen         ;get length of data to move
           jcxz     MDRExit             ;if nothing there get out
          mov       si,offset TempStor  ;get offset of data to move
          mov       ax,TranSeg          ;get segment for server
          add       ax,DataOffP         ;get data / stack segment
          mov       di,StackOff         ;point to top of stack
          sub       di,NewStack         ;point to bottom
          add       di,1Fh              ;liberal pad for C startup code
                                        ;  adjustments (StackOff is adjusted
                                        ;  downward by startup code)
          push      cx                  ;save env length
          shrn      di,4,cl             ;make it a paragraph count
          pop       cx                  ;restore env length
          add       ax,di               ;make absolute segment location
          mov       DREnvLoc,ax         ;store location for later
          mov       es,ax               ;point ES to transient stack seg
          xor       di,di               ;clear offset
          cld                           ;go forward
          rep       movsb               ;copy env data to unused stack space
          ;
MDRExit:  exit                          ;all done
          ;
          ; =================================================================
          ;
          ; MoveTran - Move the transient portion to its final address
          ;
          ; On entry:
          ;         No requirements
          ;
          ; On exit:
          ;         AX, BX, CX, DX, SI, DI destroyed
          ;         Other registers and interrupt state unchanged
          ;
          entry     MoveTran,noframe,,local  ;move transient portion
          assume    ds:@curseg          ;fix assumes
          ;
          ; Reduce our memory allocation to the end of the initialization
          ; code, leaving the transient portion of 4DOS in unallocated
          ; memory.
          ;
          Status    2,'RM'              ;(Reduce local memory)
          mov       ax,THighAdr         ;get transient high address
          mov       bx,offset InitEnd   ;point to end of what we need to
                                        ;  continue + 1 paragraph
          add       bx,0Fh              ;increment for paragraph roundoff
          shrn      bx,4,cl             ;make it a paragraph count for alloc
          mov       dx,cs               ;get local base
          add       dx,bx               ;get end of our area + 1 paragraph
          cmp       dx,ax               ;is our end below where transient
                                        ;  portion goes?
          errae     NOMEM               ;if not blow up
          loadseg   es,cs               ;get local segment
          calldos   REALLOC,MEMDA       ;reduce memory usage
          ;
          ; Note that this code leaves one extra paragraph free.  This is
          ; because some devices which add DOS memory above 640KB use the
          ; top 16 bytes of the 640K region to store an MCB for the
          ; additional memory above that.  This MCB can wipe out the top
          ; 16 bytes of 4DOS (or vice versa) if it is allowed to overlap
          ; with 4DOS, hence the extra paragraph added to keep 4DOS one
          ; paragraph lower in memory when it is loaded high.
          ;
          Status    1,'TH'              ;(Move transient portion to high memory)
          ;
          if        DEBUG ge 2          ;set up move area message
          jmp       short SkipMove      ;jump around message
MoveBuf   db        1                   ;level 1
          db        "  moving from "
SrcBuf    db        "    "
          db        " to "
DstBuf    db        "    "
          db        " paragraph count "
PCntBuf   db        "    $"
          ;
SkipMove: loadseg   es,cs               ;set ES for HexOut
          mov       cx,TranMovP         ;get length
          mov       ax,TranAddr         ;get source
          mov       di,offset SrcBuf    ;point to source buffer
          call      HexOutW             ;convert source
          mov       ax,THighAdr         ;get destination
          mov       di,offset DstBuf    ;point to destination buffer
          call      HexOutW             ;convert destination
          mov       ax,cx               ;get length
          mov       di,offset PCntBuf   ;point to count buffer
          call      HexOutW             ;convert count
          mov       ax,offset MoveBuf   ;get buffer address
          push      ax                  ;stack it
          call      DebugMsg            ;display the message
          ;
          endif                         ;debugging code only
          ;
          ;
          ; Set up the move loop
          ;
          ; These move loops start with BX pointing to the source segment
          ; and DX pointing to the destination segment.  There are two loops,
          ; one for moving down (rare) and one for moving up (the usual 
          ; case).  Under tight memory constraints the destination transient
          ; portion can overlap in either direction with the source and hence
          ; may need to be moved *down*!  This is because the final size of 
          ; the transient portion can be larger than the current size due to
          ; extra paragraphs added for aliases, functions and history.
          ;
          mov       dx,THighAdr         ;get transient destination address
          mov       TranSeg,dx          ;save as actual transient seg
          mov       TranCur,dx          ;and as current transient segment
          mov       IFData.I_HiSeg,dx   ;save as high segment for C code
          mov       bx,TranAddr         ;get source address
          mov       cx,TranMovP         ;get move length in paragraphs
          push      ds                  ;save DS during loop
          assume    ds:nothing          ;fix assumes
          cmp       bx,dx               ;test if moving transient up or down
           je       TMvDone             ;if it's just right don't move it!
           jb       TMoveUp             ;if moving up go do that
          cld                           ;moving down, moves go forward
          ;
          ; The move down loop starts with BX pointing to the paragraph at 
          ; the beginning of the loaded transient portion and DX pointing
          ; to the paragraph at the beginning of the new location for the
          ; transient portion.  It works by calculating the largest block
          ; it can move, and beginning the move at offset 0 in each block.
          ; The loop moves a maximum of 64K words at a time, repeating as
          ; necessary.
          ;
TMDLoop:  push      cx                  ;save remaining paragraphs
          maxu      cx,SLIDEMAX         ;reduce to max transfer size
          mov       ds,bx               ;copy source to DS
          mov       es,dx               ;copy destination to ES
          mov       ax,cx               ;copy count
          shln      cx,3                ;make it word count
          xor       si,si               ;source offset is 0
          xor       di,di               ;so is destination
          rep       movsw               ;copy the block (bottom up)
          add       bx,ax               ;increment source paragraph
          add       dx,ax               ;and destination
          pop       cx                  ;get back remaining paragraphs
          sub       cx,ax               ;reduce by actual transfer amount
           ja       TMDLoop             ;loop if more to transfer
          jmp       short TMvDone       ;and go save it
          ;
          ; The "end first" loop starts with BX pointing to the paragraph
          ; after the end of the loaded transient portion and DX pointing
          ; to the paragraph at the end of allocated DOS memory.  It works
          ; by calculating the largest block it can move, reducing BX and
          ; DX to point to the starting paragraph addresses of that block
          ; for source and destination respectively, and then finding the
          ; offset of the last word of the block and beginning the move
          ; from there.  The loop moves a maximum of 64K words at a time,
          ; repeating as necessary.
          ;
TMoveUp:  add       bx,cx               ;adjust source
          add       dx,cx               ;adjust destination
          std                           ;move top down
          ;
TMULoop:  push      cx                  ;save remaining paragraphs
          mov       ax,cx               ;copy it
          maxu      ax,SLIDEMAX         ;reduce to max transfer size
          sub       bx,ax               ;get source segment base
          mov       ds,bx               ;copy to DS
          sub       dx,ax               ;get destination segment base
          mov       es,dx               ;copy to ES
          mov       cx,ax               ;copy for count
          shln      cx,3                ;make it word count
          mov       si,cx               ;copy for source offset
          dec       si                  ;... offset is count - 1
          shl       si,1                ;convert word offset to byte offset
          mov       di,si               ;destination offset is the same
          rep       movsw               ;copy the block (top down)
          pop       cx                  ;get back remaining paragraphs
          sub       cx,ax               ;reduce by actual transfer amount
           ja       TMULoop             ;loop if there is more to transfer
          ;
          cld                           ;restore direction flag
          ;
          ; Move complete
          ;
TMvDone:  pop       ds                  ;loop done, restore DS
          assume    ds:@curseg          ;fix assumes
          ;
          bset      Flags,TRANSHI       ;show transient loaded high
          exit                          ;all done
          ;
          ; =================================================================
          ;
          ; SetupSCB - Set up swap control blocks
          ;
          ; On entry:
          ;         No requirements
          ;
          ; On exit:
          ;         AX, BX, CX, DX, SI, DI destroyed
          ;         Other registers and interrupt state unchanged
          ;
          entry     SetupSCB,noframe,,local  ;setup swap control blocks
          assume    ds:@curseg          ;fix assumes
          ;
          ; Set up the swap control blocks.  These blocks, defined in
          ; SERVDATA.ASM, control how different regions of 4DOS's transient
          ; portion are copied to and from the swapping space.  The regions
          ; in question, listed in ascending order by memory location, are:
          ;
          ;         Server code and data
          ;         Transient code area
          ;         Transient data/stack area
          ;
          ; The control block approach is designed to swap out all
          ; necessary sections, but to swap in only the code and data,
          ; since the PSP and server are swapped in by 4DOS.ASM.  The
          ; blocks are set up as follows:
          ;
          ;    1) If reduced swapping is not enabled and swapping is to XMS
          ;       or EMS, then all of the transient portion is swapped
          ;       together.  In this case region 1 is used to define the
          ;       entire transient portion for swap out, and region 2 is
          ;       used to define the combined code and DGROUP areas for
          ;       swap in.
          ;
          ;    2) If reduced swapping is not enabled and swapping is to disk,
          ;       then the code segment is swapped during initialization, and
          ;       the server and DGROUP areas are swapped separately.  In
          ;       this case region 1 is used to define the server area for
          ;       swap out, region 2 is used to define the DGROUP area for
          ;       swap out, and region 3 is used to define the combined code
          ;       and DGROUP areas for swap in.  In addition a local block
          ;       is defined and used to calculate the code checksum and swap
          ;       out the code.
          ;
          ;    3) If reduced swapping is enabled, then the code segment is
          ;       swapped by an earlier shell, and the server and data/stack
          ;       areas are swapped separately.  In this case region 1 is
          ;       used to define the server area for swap out, region 2 is
          ;       used to define the DGROUP area for swap out or in, and
          ;       region 3 is used to define the code area for swap in using
          ;       the previous shell's swap space.
          ;
          ;
          ; Figure out if we are doing reduced swapping
          ;
          mov       ax,offset SERV_TEXT:SLocDev  ;get address of server local
                                                 ;  device SCB
          xor       dl,dl               ;clear flags
          test      Flags,NOREDUCE      ;using previous shell's code?
          jnz       NotType3            ;if not go on
          jmp       SwapTyp3            ;if so go set up for it
          ;
NotType3: cmp       SwMethod,SWAPDISK   ;swap to disk?
          je        SwapTyp2            ;if so go set up for that
          ;
          ; No reduced swapping, swap to EMS / XMS
          ;
          Status    2,'S1'              ;(Swap type 1)
          mov       di,offset Region1   ;set up region 1 SCB
          xor       bx,bx               ;start at relative memory segment 0
          xor       si,si               ;swap file segment 0 also
          mov       cx,TranLenP         ;length of the whole thing
          bset      dl,NEWRBAS          ;show we get a new relocation base
                                        ;  when this region is swapped out
          call      DefRegn             ;define region 1 for everything
          xor       dl,dl               ;clear flags
          mov       di,offset Region2   ;set up region 2 SCB
          mov       bx,CodeOffP         ;code seg paragraph offset
          mov       si,bx               ;swap file offset is the same
          mov       cx,CodeLenP         ;get code length
          add       cx,DataLenP         ;add data length
          call      DefRegn             ;define regn 2 for code and data/stk
          mov       di,offset CodeReg   ;point to inherited code region SCB
          mov       cx,CodeLenP         ;save as region 2 but code only
          call      DefRegn             ;define code regn SCB for next shell
          mov       ax,0101h            ;swap out via region 1 only
          mov       bx,0201h            ;swap in via region 2 only
          jmp       SaveSwap            ;done, go on
          ;
          ; No reduced swapping, swap to disk
          ;
SwapTyp2:
          Status    2,'S2'              ;(Swap type 2)
          mov       di,offset Region1   ;set up region 1 SCB
          xor       bx,bx               ;start at relative memory segment 0
          xor       si,si               ;swap file segment 0 also
          mov       cx,ServLenP         ;get server length
          call      DefRegn             ;define region 1 for server
          mov       di,offset Region2   ;set up region 2 SCB
          mov       bx,DataOffP         ;DGROUP paragraph offset
          mov       si,bx               ;swap file offset is the same
          mov       cx,DataLenP         ;get data length
          call      DefRegn             ;define region 2 for DGROUP
          mov       di,offset Region3   ;set up region 3 SCB
          mov       bx,CodeOffP         ;code seg paragraph offset
          mov       si,bx               ;swap file offset is the same
          mov       cx,CodeLenP         ;get code length
          add       cx,DataLenP         ;add DGROUP length
          bset      dl,CHKCALC+RLDISABL ;set checksum and disable relocation
                                        ;  flags for this region
          call      DefRegn             ;define region 3 for code and DGROUP
          ;
          Status    2,'CO'              ;(Swap out code segment)
          mov       di,offset CodeTemp  ;set up code region SCB
          mov       cx,CodeLenP         ;get code length only (BX, SI, and
                                        ;  DL are already set)
          mov       ax,offset LocDev    ;point to local device SCB
          call      DefRegn             ;define temporary code region SCB
          mov       di,offset CodeReg   ;point to inherited code region SCB
          call      DefRegn             ;define code regn SCB for next shell
          mov       si,di               ;block address to SI
          mov       dl,2                ;set DL for checksum only
          loadseg   es,cs               ;set ES to PSP segment
          mov       bx,SERV_TEXT        ;get segment for this call
          mov       ax,offset SERV_TEXT:SwapOne  ;get call offset
          push      bx                  ;stack segment for second call
          push      ax                  ;stack offset for second call
          push      bx                  ;stack segment for this call
          push      ax                  ;stack offset for this call
          call      TranCall            ;calculate the checksum
           jc       ErrHdlAX            ;if any error get out
          xor       dl,dl               ;clear to swap out
          call      TranCall            ;swap the code out (never done again)
           jc       ErrHdlAX            ;if any error get out
          ;
          mov       ax,0102h            ;swap out via blocks 1 and 2
          mov       bx,0301h            ;swap in via region 3 only
          jmp       short SaveSwap      ;done, go on
          ;
          ; Reduced swapping is set
          ;
SwapTyp3:
          Status    2,'S3'              ;(Swap type 3)
          mov       bx,OldShell.CSwpRBas  ;get prev shell's relocation base
          mov       OldRBase,bx         ;save as our old relocation base
          mov       di,offset Region1   ;set up region 1 SCB
          xor       bx,bx               ;start at relative memory segment 0
          xor       si,si               ;swap file segment 0 also
          mov       cx,ServLenP         ;get server length
          call      DefRegn             ;define region 1 for server
          mov       di,offset Region2   ;set up region 2 SCB
          mov       bx,DataOffP         ;DGROUP paragraph offset
          mov       si,cx               ;swap file offset of data segment
                                        ;  (just after server)
          mov       cx,DataLenP         ;get data length
          cmp       SwMethod,SWAPDISK   ;using the disk?
           jne      Typ3Reg2            ;if not go on
          bset      dl,CHKCALC          ;disk, set checksum flag
          ;
Typ3Reg2: call      DefRegn             ;define region 2 for data/stack
          ;
          mov       di,offset Region3   ;set up region 3 SCB
          loadseg   es,ds               ;make ES local
          push      di                  ;save address
          mov       si,offset OldCReg   ;point to old code region SCB
          mov       cx,(size RegSCB)    ;get bytes to move
          rep       movsb               ;copy old region SCB to new
          pop       di                  ;restore SCB address
          mov       [di].DevPtr,offset SERV_TEXT:SLocCDev  
                                        ;set new device SCB offset in server
          mov       bx,CodeOffP         ;memory offset of code segment
          mov       [di].SwapMSeg,bx    ;store in SCB
          mov       si,di               ;copy region SCB address
          mov       di,offset CodeReg   ;get inherited SCB address
          mov       cx,(size RegSCB)    ;get bytes to move
          rep       movsb               ;copy code region SCB for next shell
          ;
          mov       di,offset LocCDev   ;set up local code device SCB
          mov       si,offset OldCDev   ;point to old code device SCB
          mov       cx,(size DevSCB)    ;get bytes to move
          rep       movsb               ;copy old device SCB to new
          mov       ax,0102h            ;swap out via blocks 1 and 2
          mov       bx,0202h            ;swap in via blocks 2 and 3
          ;
          ; Save swapping control information
          ;
SaveSwap: mov       OutStart,ah         ;save swap out start block
          mov       OutCnt,al           ;save swap out block count
          mov       InStart,bh          ;save swap in start block
          mov       InCnt,bl            ;save swap in block count
          ;
          exit                          ;all done
          ;
          ; =================================================================
          ;
          ; SetupICB - Set up inherited control blocks for passing info to
          ;            next shell
          ;
          ; On entry:
          ;         No requirements
          ;
          ; On exit:
          ;         AX, BX, CX, DX, SI, DI destroyed
          ;         Other registers and interrupt state unchanged
          ;
          entry     SetupICB,noframe,,local  ;set up inheritance control
          assume    ds:@curseg          ;fix assumes
          ;
          ;
          ; CAUTION:  The code below uses AL for the swap type and BX for
          ; the swap handle.  These must be preserved as they are also used
          ; to set up the data segment (alias / function / history) SCB.
          ;
          ; Get handle and modify it to system file handle if using disk
          ; swapping.
          ;
          Status    1,'CN'              ;(Set up control blocks for next shell)
          mov       al,SwMethod         ;get swap method
          mov       bx,LMHandle         ;get local handle
          loadseg   es,ds               ;make ES local
          cmp       al,SWAPDISK         ;swapping to disk?
          jne       SetSInf             ;no, go on
          mov       bl,PSP_FTAB[bx]     ;get corresp. system file handle
          or        bh,80h              ;set high bit for swapping code
                                        ;  (shows it's a sys file handle)
          ;
          ; Set up the ShellInf structure in 4DOS.ASM, which, along with the
          ; code SCBs defined below, allows a later shell to obtain all
          ; information to be inherited from this shell.
          ;
SetSInf:  mov       ShellInf.DSwpType,al  ;save data swap method
          mov       ShellInf.DSwpHdl,bx   ;save data swap handle
	mov	dl,WinMode	;get Windows mode
	mov	ShellInf.ShWMode,dl	;save it for next shell
          mov       dx,DataOffP         ;get data segment offset
          test      Flags,NOREDUCE      ;reduced swapping?
           jnz      SaveDRel            ;if not save as swap file address
          mov       dx,ServLenP         ;reduced swapping so data seg is
                                        ;  in swap file just after server
          ;
SaveDRel: add       dx,INIOffP            ;add to get INI data offset
          mov       ShellInf.DSwpLoc,dx   ;save for next shell
          cmp       al,SWAPNONE           ;swapping disabled?
           jne      GetSDLen              ;if not go on
          add       dx,TranSeg            ;make absolute segment
          mov       ShellInf.DSwpMLoc,dx  ;save for next shell
          ;
GetSDLen: mov       dx,ExtraPar           ;get INI / history / aliases length
          mov       ShellInf.DSwpLenP,dx  ;save for next shell
          mov       dx,INILenB            ;get INI file data length
          mov       ShellInf.DSwpILen,dx  ;save for next shell
          ;
          mov       si,offset AlsData   ;point to alias data
          mov       di,offset ShellInf.DSwpAls  ;point to alias swap data
          call      SwDatAH             ;set up alias swap data
          mov       si,offset FuncData   ;point to alias data
          mov       di,offset ShellInf.DSwpFunc  ;point to function swap data
          call      SwDatAH             ;set up function swap data
          mov       si,offset HistData  ;point to cmd history data
          mov       di,offset ShellInf.DSwpHist  ;point to cmd hist swap data
          call      SwDatAH             ;set up cmd history swap data
          mov       si,offset DirData   ;point to dir history data
          mov       di,offset ShellInf.DSwpDir  ;point to dir hist swap data
          call      SwDatAH             ;set up dir history swap data
          ;
          ; Set up the code SCBs in 4DOS.ASM, which allow a later shell to
          ; use our code segment swap area.
          ;
          cmp       al,SWAPNONE         ;swapping disabled?
           je       ICBExit             ;if so no need to set up the SCBs
          test      Flags,NOREDUCE      ;using old code?
           jz       CSOld               ;if so go handle that
          ;
          ; Set up the inherited code device SCB when there is no reduced
          ; swapping.  The inherited code region SCB has already been set
          ; up by SetupSCB.
          ;
          Status    2,'NB'              ;(Use new code SCBs)
          mov       di,offset CodeDev   ;point to inherited code device SCB
          call      DefDev              ;define the code swap device SCB
                                        ;  to be passed on to later shells
          mov       dx,THighAdr         ;get high load address
          mov       ShellInf.CSwpRBas,dx  ;save as relocation base
          jmp       short ICBExit       ;all done
          ;
          ; Set up the code swap info block when reduced swapping is in use.
          ; In this case all we have to do is copy the previous shell's
          ; SCBs.
          ;
CSOld:
          Status    2,'OB'              ;(Use old code SCBs)
          mov       si,offset OldCReg   ;point to old code region SCB
          mov       di,offset CodeReg   ;point to inherited code region SCB
          mov       cx,(size RegSCB)    ;get bytes to move
          loadseg   es,ds               ;make ES local
          rep       movsb               ;copy old region SCB to new
          mov       si,offset OldCDev   ;point to old code device SCB
          mov       di,offset CodeDev   ;point to inherited code device SCB
          mov       cx,(size DevSCB)    ;get bytes to move
          rep       movsb               ;copy old device SCB to new
          mov       dx,OldShell.CSwpRBas  ;get prev shell's relocation base
          mov       ShellInf.CSwpRBas,dx  ;save as relocation base
          ;
ICBExit:  exit                          ;all done
          ;
          ; =================================================================
          ;
          ; SwDatAH - Set up swap data for aliases, functions and history
          ;
          ; On entry:
          ;         SI = alias / history local data block address
          ;         DI = alias / history swap data block address
          ;
          ; On exit:
          ;         DX destroyed
          ;         Other registers and interrupt state unchanged
          ;
          entry     SwDatAH,noframe,,local  ;set up alias / history swap data
          assume    ds:@curseg          ;fix assumes
          ;
          mov       dl,[si].AHType      ;get list type
          mov       [di],dl             ;save for next shell (AHSwType)
          mov       dx,[si].AHLenB      ;get list length
          mov       [di+1],dx           ;save for next shell (AHSwLen)
          ;
          exit                          ;all done
          ;
          ; =================================================================
          ;
          ; MoveUMB - Move resident portion to a UMB
          ;
          ; On entry:
          ;         No requirements
          ;
          ; On exit:
          ;         AX, BX, CX, DX, SI, DI destroyed
          ;         Other registers and interrupt state unchanged
          ;
          entry     MoveUMB,noframe,,local  ;move resident portion to a UMB
          assume    ds:@curseg          ;fix assumes
          ;
          ; Reserve a UMB for the resident code
          ;
          Status    1,'UR'              ;(Reserve UMB)
          ;
          ; Set things up to reserve the UMB in a specific region
          ; if requested
          ;
          push      wptr IFData.I_UMBLd ;stack desired region
          push      ResSize             ;stack size to reserve
          call      ResUMB              ;reserve a UMB if possible
           jc       NoUMB               ;if error get out
          mov       LMSeg,bx            ;save the new segment
          mov       ah,1                ;assume DOS UMB
          test      al,RUDOSUMB         ;is it a DOS UMB?
           jnz      MUFree              ;if so go on
          inc       ah                  ;set type to XMS UMB
          ;
MUFree:   call      AddFree             ;add block to the free list
          test      al,RUREGERR         ;was there a region error?
           jz       MUMove              ;if not go on
          mov       dx,offset NoDatReg  ;get region error message address
          call      StatMsg             ;display it
          mov       dx,offset RPUText   ;get resident portion msg address
          call      StatMsg             ;display it
          ;
          ; Move the resident portion to a UMB
          ;
          ; NOTE:  Any changes to local data made after this point will
          ; NOT affect subsequent operation because they will operate on
          ; the low-memory copy of the data, and that copy will not be
          ; used once the resident portion is moved to a UMB.
          ;
MUMove:
          Status    1,'UM'              ;(Move resident portion to UMB)
          xor       si,si               ;start at beginning of PSP
          mov       es,bx               ;get UMB code segment
          xor       di,di               ;clear destination offset
          mov       cx,ResSize          ;get resident size in paragraphs
          shln      cx,3                ;convert to words
          rep       movsw               ;move loader to UMB
          mov       ResSize,PSP_LENP    ;reduce resident size
          mov       es:[PSP_CHN],ds     ;chain UMB PSP back to low PSP
          mov       es:ReopenP.fseg,es  ;set swap file reopen code segment in
                                        ;  case it is used
          jmp       short UMBDone       ;and exit
;          mov       dx,offset UMBMsg    ;point to message
;          jmp       short UMBDisp       ;and go display it
          ;
NoUMB:    bclr      Flags,LOADUMB       ;clear UMB load flag
          mov       bptr IFData.I_UMBLd,0  ;clear it in INI file
          mov       dx,offset NoDatUMB  ;get region error message address
          call      StatMsg             ;display it
          mov       dx,offset RPUText   ;get resident portion msg address
          call      StatMsg             ;display it
          ;
UMBDone:  exit                          ;all done
          ;
          ; =================================================================
          ;
          ; DataBlks - Create data blocks for the master environment, global
          ;            aliases, global functions, global history, and global
          ;            directory history.
          ;            Each block that is not in a UMB is added to the
          ;            low-memory reservation list.
          ;
          ; On entry:
          ;         No requirements
          ;
          ; On exit:
          ;         BX, CX, DX, SI, DI, ES destroyed
          ;         Other registers and interrupt state unchanged
          ;
          entry     DataBlks,noframe,,local  ;reserve data blocks
          assume    ds:@curseg          ;fix assumes
          ;
          Status    1,'DB'              ;(Reserve data blocks)
          ;
          push      ax                  ;preserve AX
          mov       al,IFData.I_UMBEnv  ;get desired environment region
          or        al,al               ;check it
           jz       DBLowEnv            ;if not in UMBs go on
          xor       ah,ah               ;clear high byte
          mov       bx,EnvSizeP         ;get size to reserve
          mov       dx,offset MEUText   ;addr of problem message suffix
          call      DataUMB             ;reserve the UMB if possible
           jc       DBLowEnv            ;if error go put it low
          mov       IFData.I_ESeg,bx    ;save master environment location
          mov       ax,0FFFFh           ;flag for environment copy
          jmp       DBCpyEnv            ;go set up copy information
          ;
DBLowEnv: mov       IFData.I_UMBEnv,0   ;clear UMB environment flag
          mov       ax,EnvSizeP         ;get size to reserve low
          ;
DBCpyEnv: mov       bx,PassEnv          ;get passed environment
          mov       cx,OldEnvSz         ;get old size
          or        bx,bx               ;check passed environment
           jnz      DBLESet             ;if something there go on
          mov       cx,DREnvLen         ;get length of DR-DOS passed env 
                                        ;  (passed from CONFIG.SYS SETs,
                                        ;  will be zero if none)
          mov       bx,DREnvLoc         ;get segment
          ;
DBLESet:  mov       dx,I_ESeg           ;point to addr storage word
          xor       si,si               ;no addr in low memory
          call      DataLow             ;add to low memory reservation list
          ;
DBAlias:  mov       ax,(I_ALoc+2)       ;place to put block segment
          mov       bx,offset ShellInf.DSwpAls.AHSwSeg  ;low loc for segment
          mov       dx,offset GAUText   ;addr of problem message suffix
          mov       si,offset IFData.I_UMBAls  ;point to UMB flag
          mov       di,offset AlsData   ;point to alias data
	  call      DataAH              ;go set up alias block
          
	  mov       ax,(I_FLoc+2)       ;place to put block segment
          mov       bx,offset ShellInf.DSwpFunc.AHSwSeg  ;low loc for segment
          mov       dx,offset GFUText   ;addr of problem message suffix
          mov       si,offset IFData.I_UMBFunc  ;point to UMB flag
          mov       di,offset FuncData  ;point to function data
          call      DataAH              ;go set up function block
	  
          mov       ax,(I_HLoc+2)       ;place to put block segment
          mov       bx,offset ShellInf.DSwpHist.AHSwSeg  ;low loc for segment
          mov       dx,offset GHUText   ;addr of problem message suffix
          mov       si,offset IFData.I_UMBHst  ;point to UMB flag
          mov       di,offset HistData  ;point to cmd history data
          call      DataAH              ;go set up cmd history block
	  
          mov       ax,(I_DLoc+2)       ;place to put block segment
          mov       bx,offset ShellInf.DSwpDir.AHSwSeg  ;low loc for segment
          mov       dx,offset GDUText   ;addr of problem message suffix
          mov       si,offset IFData.I_UMBDir  ;point to UMB flag
          mov       di,offset DirData   ;point to dir history data
          call      DataAH              ;go set up dir history block
          ;
          pop       ax                  ;restore AX
          exit                          ;all done
          ;
          ; =================================================================
          ;
          ; DataAH - Reserve a UMB or set up to reserve a low memory block
          ;          for global aliases, command history, or directory history
          ;
          ; On entry:
          ;         AX = offset of block address in INI file data area
          ;         BX = offset of block address in low memory inheritance
          ;              info
          ;         DX = block name text address
          ;         SI = offset of UMB flag
          ;         DI = alias / history data block address
          ;
          ; On exit:
          ;         AX, BX, CX, DX, SI, DI, ES destroyed
          ;         Other registers and interrupt state unchanged
          ;
          entry     DataAH,varframe,,local  ;set up alias / history block
          assume    ds:@curseg          ;fix assumes
          ;
          varW      DAINIOff            ;INI block address offset
          varW      DALowOff            ;low-memory block address offset
          varend
          ;
          Status    2,'SA'              ;(Set up alias / history data block)
          ;
          mov       DAINIOff,ax         ;save INI offset
          mov       DALowOff,bx         ;save low-memory offset
          mov       al,[di].AHType      ;get block type
          cmp       al,AHGLNEW          ;reserving global?
           je       DAReserv            ;if so go on
          cmp       al,AHGLOLD          ;using previous global?
           jne      DADone              ;if not, exit
          mov       ax,[di].AHPSeg      ;get old segment
          mov       es,LMSeg            ;put it low or in UMB as needed
          mov       es:[bx],ax          ;store in inheritance block
          jmp       DADone              ;and exit
          ;
DAReserv: mov       al,[si]             ;get desired region
          or        al,al               ;check it
           jz       DALow               ;if not in UMBs go on
          xor       ah,ah               ;clear high byte
          mov       bx,[di].AHLenP      ;get size to reserve
          call      DataUMB             ;reserve the UMB if possible
           jc       DALow               ;if error set up for low memory
          mov       [di].AHSeg,bx       ;save location
          mov       di,DALowOff         ;get low-memory offset
          mov       es,LMSeg            ;put it low or in UMB as needed
          mov       es:[di],bx          ;save for inheritance
          jmp       DADone              ;done, get out
          ;
DALow:    mov       bptr [si],0         ;clear UMB flag
          mov       ax,[di].AHLenP      ;get size to reserve
          mov       bx,[di].AHSeg       ;get temp seg if inheriting
          xor       cx,cx               ;assume no inheritance
          test      InitFlag,XINHERIT   ;inheritance disabled?
           jnz      DASetLow            ;if so go on
          mov       cx,[di].AHPLenB     ;get old size
          ;
DASetLow: mov       dx,DAINIOff         ;INI offset to DX
          mov       si,DALowOff         ;low-memory offset to SI
          call      DataLow             ;add block to low memory list
          ;
DADone:   exit                          ;and get out
          ;
          ; =================================================================
          ;
          ; DataUMB - Reserve a UMB for the master environment, global
          ;           aliases, global cmd history, or global dir history
          ;
          ; On entry:
          ;         AX = desired region
          ;         BX = space to reserve in paragraphs
          ;         DX = block name text address
          ;
          ; On exit:
          ;         Carry clear if no error, set if error
          ;         If carry clear:
          ;           BX = segment address of reserved UMB
          ;           Word at BX:0 set to 0
          ;         AX, CX, DX, ES destroyed
          ;         Other registers and interrupt state unchanged
          ;
          entry     DataUMB,noframe,,local  ;reserve a data UMB
          assume    ds:@curseg          ;fix assumes
          ;
          Status    2,'OU'              ;(Reserve one data UMB)
          ;
          ; Set things up to reserve the UMB in a specific region
          ; if requested
          ;
          pushm     si,di               ;save SI, DI
          push      dx                  ;save error text address
          push      ax                  ;stack desired region
          push      bx                  ;stack size to reserve
          call      ResUMB              ;reserve a UMB if possible
          pop       cx                  ;restore error text address
           jc       DUErr               ;if error get out
          mov       es,bx               ;point to the UMB
          mov       wptr es:[0],0       ;make it look empty with double null
          mov       ah,1                ;assume DOS UMB
          test      al,RUDOSUMB         ;is it a DOS UMB?
           jnz      DUSFree             ;if so go on
          inc       ah                  ;set type to XMS UMB
          ;
DUSFree:  call      AddFree             ;add block to the free list
          test      al,RUREGERR         ;was there a region error?
           jz       DUDone              ;if not go exit (test clears carry)
          mov       dx,offset NoDatReg  ;get region error message address
          jmp       DUDisp              ;and go display it (carry clear)
          ;
DUErr:    mov       dx,offset NoDatUMB  ;point to error message (carry set)
          ;
DUDisp:   pushf                         ;save result flag
          call      StatMsg             ;display UMB status
          mov       dx,cx               ;copy block type text address
          call      StatMsg             ;display that
          popf                          ;get back result
          ;
DUDone:   popm      di,si               ;restore SI, DI
          exit                          ;all done
          ;
          ; =================================================================
          ;
          ; DataLow - Set up to reserve a low-memory block for the master
          ;           environment, global aliases, global cmd history, or
          ;           global data history, or to copy the environment to a UMB
          ;
          ; On entry:
          ;         AX = block size, paragraphs, or FFFF to copy environment
          ;         BX = source segment for any data copy
          ;         CX = bytes to copy (gets rounded up to word count),
          ;              0 if no copy
          ;         DX = offset of segment address word within INI data
          ;         SI = offset of segment address word in low-memory
          ;              inheritance data
          ;
          ; On exit:
          ;         DI destroyed
          ;         All other registers and interrupt state unchanged
          ;
          entry     DataLow,noframe,,local  ;set up to reserve low memory
          assume    ds:@curseg          ;fix assumes
          ;
          Status    2,'OL'              ;(Set up for one low-memory block)
          ;
          mov       di,LRPtr            ;point to low-memory reserve data
          mov       [di].LRRSize,ax     ;set size to reserve (bytes)
          inc       cx                  ;bump for roundoff
          shr       cx,1                ;make it a word count
          mov       [di].LRCSize,cx     ;set size to copy (words)
          mov       [di].LRSource,bx    ;set source segment
          mov       [di].LRSavLoc,dx    ;set addr offset in INI data area
          mov       [di].LRLowLoc,si    ;set addr offset in low memory
                                        ;  inheritance data
          add       LRPtr,(size LRItem) ;move to next item
          ;
          exit                          ;all done
          ;
          ; =================================================================
          ;
          ; SetChain - Set up PSP chaining, critical error handler, all
          ;            interrupt hooks, and PSP old INT 22-24 values; 
          ;
          ; On entry:
          ;         DS = CS
          ;
          ; On exit:
          ;         DS preserved
          ;         All other registers destroyed, interrupt state unchanged
          ;         Sets a variety of crucial flags etc.
          ;
          entry     SetChain,noframe,,local  ;set up chaining and INT hooks
          assume    ds:@curseg          ;fix assumes
          ;
          ; Get segment for actual loader code --  this may be the same as
          ; CS or it may point to a UMB.  This is used to store the prompt,
          ; set saved interrupt vectors in the PSP, and set actual interrupt
          ; vectors.  NOTE:  this code assumes that if a UMB load is active
          ; then the appropriate code has already been moved to the UMB.
          ;
          Status    1,'CH'              ;(Set up chains)
          mov       dx,LMSeg            ;get loader segment
          ;
          ; Fix PSP chain pointer to point to us
          ;
          mov       ax,ds:[PSP_CHN]     ;get old chain pointer
          mov       SavChain,ax         ;save it
          mov       ds:[PSP_CHN],ds     ;make PSP chain pointer point to us
          ;
          ; Set up critical error handler data (DOS version, prompt text)
          ;
          mov       es,dx               ;destination in loader segment
          mov       si,offset Prompt2   ;assume DOS 2 prompt
          calldos   VERSION             ;get dos version
          cmp       al,3                ;DOS 3 or greater?
           jb       SetPromp            ;no, go copy prompt
          bset      es:LMFlags,DOS3     ;set DOS 3 flag in loader segment
          mov       si,offset Prompt3   ;point to DOS 3 prompt
          ;
SetPromp: mov       di,offset Prompt    ;point to prompt storage
          lodsb                         ;get length byte
          mov       cl,al               ;copy to CL
          xor       ch,ch               ;clear high byte of count
          rep       movsb               ;copy prompt
          mov       es:EMSeg,es         ;set segment for ErrMsg call
          ;
          ; Save the critical error text address for 4DOSUTIL.ASM and for
          ; the local resident code
          ;
          mov       bx,dx               ;copy our loader segment
          mov       ax,CritPtr          ;get offset where critical error
                                        ;  text module was loaded
          test      Flags,NOREDUCE      ;anyone before us to get data from?
           jnz      CritSave            ;if not go store these values
          dload     ax,bx,OldShell.ShCETAdr  ;get old values
          ;
CritSave: dstore    CritTAdr,ax,bx      ;save segment/offset for transient
          dstore    es:ShellInf.ShCETAdr,ax,bx  ;save it in loader segment
                                                ;  for resident code
          ;
          ; Get the initial SFT entries for STDIN and STDERR and save them
          ; for use by the INT 24 handler
          ;
          mov       al,ds:[PSP_FTAB+STDIN]  ;get SFT handle for STDIN
          mov       ah,ds:[PSP_FTAB+STDERR]  ;get SFT handle for STDERR
          mov       es:Err24Hdl,ax      ;save new INT 24 in/out handles
          ;
          ; Now fix up the old terminate address in the PSP to point to
          ; ourselves, and if we're the root shell fix the INT 23/24
          ; addresses as well.  COMMAND.COM does it this way and some
          ; programs depend on it.  Also this allows us to abort ourselves
          ; via an INT 24 (A)bort response and still come back to a point
          ; within 4DOS.
          ;
          dload     ax,bx,ds:[PSP_TERM] ;get old PSP INT 22 vector
          dstore    OldSav22,ax,bx      ;save it locally
          mov       ds:[PSP_TERM],offset ExecExit  ;save term addr in PSP
          mov       ds:[PSP_TERM]+2,dx  ;save segment too
          test      LMFlags,ROOTFLAG    ;are we the root shell?
           jz       HookAll             ;if not don't change INT 23/24 values
          mov       ds:[PSP_CRIT],offset Int24  ;save crit error addr in PSP
          mov       ds:[PSP_CRIT+2],dx    ;save segment too
          mov       ds:[PSP_BRK],offset Int23   ;save ^Break address in PSP
          mov       ds:[PSP_BRK+2],dx     ;save segment too
          ;
          ; Hook all the interrupts:  resident server, ^Break, critical
          ; error, and COMMAND call
          ;
HookAll:  push      ds                  ;save ds
          mov       ds,dx               ;get loader segment
          ;
          mov       al,SERVINT          ;get server interrupt number
          mov       cl,INTHOOK          ;get flag bit
          mov       dx,offset IntServ   ;get handler address
          mov       di,offset OldInt2F  ;get old vector storage address
          call      HookInt             ;go hook the server interrupt
          dstore    OldInt,bx,ax        ;save old vector for chain (saves in
                                        ;  loader segment)
          or        ax,bx               ;old segment and offset both 0?
           jnz      Hook23              ;if not go on
          bset      LMFlags,NOCHAIN     ;set interrupt chain not required
          ;
Hook23:   mov       al,23h              ;get interrupt number
          mov       cl,I23HOOK          ;get flag bit
          mov       dx,offset Int23     ;get handler address
          mov       cs:LocInt23,dx      ;save for restore by server
          mov       di,offset OldInt23  ;get old vector storage address
          call      HookInt             ;go hook the ^C interrupt
          ;
          mov       al,24h              ;get interrupt number
          mov       cl,I24HOOK          ;get flag bit
          mov       dx,offset Int24     ;get handler address
          dstore    cs:LocInt24,dx,ds   ;save full addr for restore by server
          mov       di,offset OldInt24  ;get old vector storage address
          call      HookInt             ;go hook the critical error interrupt
          ;
          test      LMFlags,ROOTFLAG    ;are we the root shell?
           jz       HookDone            ;if not don't hook int 2E
          mov       al,2Eh              ;get interrupt number
          mov       cl,I2EHOOK          ;get flag bit
          mov       dx,cs:Int2EPtr      ;get handler address
          mov       di,offset OldInt2E  ;get old vector storage address
          call      HookInt             ;go hook the ^C interrupt
          ;
          ;
HookDone: pop       ds                  ;restore DS
          ;
          exit                          ;all done
          ;
          ; =================================================================
          ;
          ; InitTran - Initialize transient portion
          ;
          ; On entry:
          ;         AX = segment where server data goes
          ;
          ; On exit:
          ;         AX, BX, CX, DX, SI, DI destroyed
          ;         Other registers and interrupt state unchanged
          ;         Moves local data to transient portion
          ;
          entry     InitTran,noframe,,local  ;initialize transient
          assume    ds:@curseg          ;fix assumes
          ;
          ; Fill in the rest of the server data and move it to the start of
          ; the server segment
          ;
          push      ax                  ;save server seg
          mov       LMExePtr,offset LMExec  ;set EXEC address
          mov       al,LMFlags          ;get our flags
          mov       SrvFlags,al         ;copy to server
          mov       al,LMFlag2          ;get second byte
          mov       SrvFlag2,al         ;copy to server
          mov       al,SwMethod         ;get swap method
          mov       SrvSwap,al          ;copy to server
          mov       ax,TranSeg          ;get actual transient segment
          mov       es,LMSeg            ;get resident segment
          mov       es:ServSeg,ax       ;save transient seg for resident code
          mov       bx,ax               ;copy segment
          sub       bx,SERV_TEXT        ;adjust off original server seg
          add       bx,LOAD_TEXT        ;add back load seg to get reloc base
          mov       TRelBase,bx         ;and save it
          mov       bx,ax               ;copy segment
          add       bx,CodeOffP         ;add code offset for actual _TEXT seg
          mov       TCodeSeg,bx         ;save as code segment for server
          mov       bx,ax               ;copy segment again
          add       bx,DataOffP         ;add data offset for actual DROUP seg
          mov       TDataSeg,bx         ;save as data segment for server
          add       bx,INIOffP          ;get INI data segment
          mov       INISeg,bx           ;save it for inheritance when
                                        ;  memory-resident
          pop       es                  ;now ES = server seg
          mov       di,offset SDataLoc  ;get destination offset
          mov       si,offset SDStart   ;start at beginning of server data
          mov       cx,SDataLen         ;set byte count
          cld                           ;go forward
          rep       movsb               ;move the server data into place
          ;
          ; Set up transient portion DGROUP address
          ;
          mov       ax,es               ;get server segment
          add       ax,DataOffP         ;point to current location of DGROUP
                                        ;  (will be different from TDataSeg
                                        ;  in non-swapping mode as transient
                                        ;  portion has not yet been moved)
          mov       es,ax               ;now ES = DGROUP (used through rest
                                        ;  of this routine)
          mov       TranDCur,ax         ;save as current data seg
          ;
          ; Figure out the length of the data segment including the INI file
          ; data, and any local alias / function / history storage
          ;
          mov       ax,offset DGROUP:_end  ;get end of data segment
          push      ax                  ;save it for copying INI data
          mov       bx,ExtraPar         ;get length of INI, alias, history
          shln      bx,4,cl             ;make it bytes
          add       ax,bx               ;add to get total length
          ;
          ; Zero the transient portion's uninitialized data segment.  This
          ; also clears any local alias and history buffers, as they are
          ; located just after the data segment.  This code assumes AX
          ; contains the total space in the data segment, aliases, and
          ; history calculated above.
          ;
ITZero:   mov       di,offset DGROUP:_edata ;point to uninitialized data area
          mov       cx,ax               ;copy total data seg space
          sub       cx,di               ;get length of data segment
          shr       cx,1                ;make it a word count
          xor       ax,ax               ;get zero
          rep       stosw               ;clear it out
	;
	; Copy the COMSPEC path to gszFindDesc, and the passed command
	; line to gszCmdline
	;
	mov	di,offset DGROUP:_gszFindDesc  ;get buffer address
	test      InitFlag,CSPSET     ;was a COMSPEC path specified?
           jz       ITCSPEnd		;if not just terminate it
          mov       si,offset CSPath    ;get new COMSPEC path address
          mov       cx,CSpecLen         ;get its length
          rep       movsb               ;copy it for DOSINIT.C

ITCSPEnd:	xor	al,al		;get 0
	stosb			;terminate the COMSPEC path
	;
	push	ds		;save DS
	lds	si,TailPtr	;point to command tail
          call      STR_LEN             ;get length in CX
	mov	di,offset DGROUP:_gszCmdline  ;get buffer address
	rep	movsb		;copy command tail
	xor	al,al		;get zero
	stosb			;terminate the tail
	pop	ds		;restore DS
          ;
          ; Copy INI file data, strings, and keys to the area between the 
          ; transient portion's data segment and stack.
          ;
          mov       si,offset IFData    ;point to source
          pop       di                  ;restore INI data address
          push      di                  ;save again alias / history pointer
                                        ;  storage below
          mov       ax,di               ;copy to AX
          mov       cx,(size INIData)   ;get data size in bytes
          add       ax,cx               ;skip address past fixed data
          mov       [si].I_StrDat,ax    ;save new DGROUP-relative offset of
                                        ;  INI file strings
;          mov       bx,[si].I_StrUse    ;get strings length
          mov       bx,[si].I_StrMax    ;get strings length
          add       ax,bx               ;skip past strings
          mov       [si].I_Keys,ax      ;save new DGROUP-relative offset of
                                        ;  INI file keys
          ;
          mov       ShellInf.ShINILoc,di  ;save offset for INI dump routine
          mov       ShellInf.ShINILoc+2,es  ;save segment too
          rep       movsb               ;move INI file data to transient
          ;
          mov       si,offset INIStDat  ;point to strings data
          mov       cx,bx               ;copy strings length
          rep       movsb               ;copy the strings
          ;
          mov       si,offset INIKeys   ;point to keys data
          mov       cx,INIKLen          ;get keys length
          rep       movsb               ;copy the keys
          ;
          ; Inherit any local alias / history data
          ;
          add       di,0Fh              ;add for roundoff
          shrn      di,4,cl             ;get seg offset for alias / history
          mov       LAHSeg,di           ;save it for InitAH
          ;
          pop       di                  ;get back INI file data address
          mov       bx,offset AlsData   ;get alias data block address
          mov       si,offset ShellInf.DSwpAls.AHSwSeg  ;low loc for segment
          call      InitAH              ;set up and inherit aliases
          mov       wptr es:[di].I_ALoc+2,ax  ;set alias segment

          mov       bx,offset FuncData  ;get function data block address
          mov       si,offset ShellInf.DSwpFunc.AHSwSeg  ;low loc for segment
          call      InitAH              ;set up and inherit functions
          mov       wptr es:[di].I_FLoc+2,ax  ;set history segment

          mov       bx,offset HistData  ;get cmd history data block address
          mov       si,offset ShellInf.DSwpHist.AHSwSeg  ;low loc for segment
          call      InitAH              ;set up and inherit cmd history
          mov       wptr es:[di].I_HLoc+2,ax  ;set history segment

          mov       bx,offset DirData   ;get dir history data block address
          mov       si,offset ShellInf.DSwpDir.AHSwSeg  ;low loc for segment
          call      InitAH              ;set up and inherit dir history
          mov       wptr es:[di].I_DLoc+2,ax  ;set dir history segment
          ;
ITExit:   exit                          ;all done
          ;
          ; =================================================================
          ;
          ; InitAH - Initialize and inherit alias, function, or history list
          ;
          ; On entry:
          ;         BX = address of AHData structure for list
          ;         SI = address of low-memory block addr word for
          ;              inheritance
          ;
          ; On exit:
          ;         AX = address of assigned segment
          ;         ES, DI preserved
          ;         BX, CX, DX, SI destroyed
          ;         Other registers and interrupt state unchanged
          ;         Local area reserved if necessary; old
          ;           data inherited or copied as required;
          ;           AHSeg field in structure updated for correct segment;
          ;           low-memory inheritance data updated for next shell
          ;
          entry     InitAH,noframe,,local  ;inherit data from old shell
          assume    ds:@curseg          ;fix assumes
          ;
          pushm     di,es               ;save registers
          ;
          mov       al,[bx].AHType      ;get block type
          cmp       al,AHGLOLD          ;using previous global space?
           jne      AHChkGN             ;if not go on
          mov       dx,[bx].AHPSeg      ;get previous shell's segment
          jmp       AHDone              ;and get out
          ;
AHChkGN:  mov       dx,[bx].AHSeg       ;assume global UMB or temp segment
          mov       es,dx               ;make that the destination
          cmp       al,AHGLNEW          ;using new global space?
           je       AHInh               ;if so go inherit the data
          ;
          mov       ax,LAHSeg           ;get current local segment
          mov       dx,ax               ;save it
          add       ax,[bx].AHLenP      ;add length of reserved area
          mov       LAHSeg,ax           ;store new local segment
          mov       ax,dx               ;get back segment
          sub       ax,INIOffP          ;make it relative to INI file data
          mov       es,LMSeg            ;point to resident segment
          mov       es:[si],ax          ;store block addr for inheritance in
                                        ;  next shell
          mov       ax,dx               ;get back segment again
          add       ax,TranDCur         ;make it current absolute segment
          mov       es,ax               ;copy as destination
          mov       wptr es:[0],0       ;make it look empty with double null
          add       dx,TDataSeg         ;get final segment address
          ;
AHInh:    test      InitFlag,XINHERIT   ;any inheritance?
           jnz      AHDone              ;if not get out
          cmp       [bx].AHPType,AHLOCAL  ;local in previous shell?
           je       AHInhGo             ;if so go inherit prev shell's data
          xor       di,di               ;clear destination offset
          mov       cx,[bx].AHPLenB     ;get previous byte length
          shr       cx,1                ;make it word length
          push      ds                  ;save DS
          mov       ds,[bx].AHPSeg      ;get previous segment
          assume    ds:nothing          ;fix assumes
          xor       si,si               ;clear source offset
          cld                           ;go forward
          rep       movsw               ;copy data from prev shell's global
                                        ;  area
          pop       ds                  ;restore DS
          assume    ds:@curseg          ;fix assumes
          jmp       AHDone              ;all done, get out
          ;
AHInhGo:  mov       ax,[bx].AHPSeg      ;get previous relative segment
          shln      ax,4,cl             ;convert to byte offset in DGROUP
          pushm     bx,dx               ;save data pointer, final seg
          mov       dx,[bx].AHPLenB     ;get length
          mov       bx,es               ;set BX to dest seg
          call      Inherit             ;inherit the data
          popm      dx,bx               ;get back data pointer, final seg
          ;
AHDone:   mov       [bx].AHSeg,dx       ;save final segment
          mov       ax,dx               ;copy to AX
          popm      es,di               ;restore registers
          exit                          ;all done
          ;
          ; =================================================================
          ;
          ; Inherit - Inherit data from previous shell
          ;
          ; On entry:
          ;         AX = byte offset of data to inherit within inherited
          ;              INI file data area on swap device
          ;         BX = absolute segment address where data gets stored
          ;         DX = number of bytes to transfer (will be rounded to
          ;              paragraph)
          ;
          ; On exit:
          ;         AX, BX, CX, DX, SI, DI destroyed
          ;         Other registers and interrupt state unchanged
          ;         Moves local data to transient portion
          ;
          entry     Inherit,noframe,,local  ;inherit data from old shell
          assume    ds:@curseg          ;fix assumes
          ;
          push      es                  ;save ES
          mov       si,OldShell.DSwpMLoc  ;get memory-resident location
          or        si,si               ;is previous shell memory-resident?
           jnz      InhMem              ;if so handle that
          cmp       OldShell.DSwpType,0FFh  ;is data swap type set?
           jne      InhGo               ;if so go on
          stc                           ;oops, didn't work
          jmp       short InhExit       ;so exit
          ;
InhGo:    add       ax,0Fh              ;add for roundoff
          shrn      ax,4,cl             ;convert to paragraphs
          mov       si,OldShell.DSwpLoc ;get data location on swap device
          add       si,ax               ;get desired location in swap area
          mov       ax,offset InhDev    ;copy device SCB address
          add       dx,0Fh              ;add for roundoff
          shrn      dx,4,cl             ;convert to paragraphs
          mov       cx,dx               ;copy
          mov       dl,SKIPTEST+SEGABS  ;set flags to skip tests, load to
                                        ;  absolute segment address
          mov       di,offset InhReg    ;point to region SCB
          push      di                  ;save address
          call      DefRegn             ;set up region SCB
          mov       di,ax               ;point to device SCB
          mov       al,OldShell.DSwpType  ;get data swap type
          mov       bx,OldShell.DSwpHdl ;get handle
          call      DefDev              ;set up device SCB
          pop       si                  ;get back address of region SCB
          mov       dl,1                ;swap in
          loadseg   es,cs               ;set ES to PSP segment
          mov       ax,SERV_TEXT        ;get segment for this call
          push      ax                  ;stack it
          mov       ax,offset SERV_TEXT:SwapOne  ;get call offset
          push      ax                  ;stack for call
          call      TranCall            ;swap block in
          jmp       short InhOK         ;and exit
          ;
InhMem:   mov       es,bx               ;copy destination segment
          xor       di,di               ;destination offset is 0
          push      ds                  ;save DS
          mov       ds,si               ;copy source segment
          mov       si,ax               ;get offset
          mov       cx,dx               ;copy byte count
          call      MEM_CPY             ;copy the data into place
          pop       ds                  ;restore DS
          ;
InhOK:    clc                           ;show all OK
          ;
InhExit:  pop       es                  ;restore ES
          exit                          ;all done
          ;
          ;
          ; *************************************************************
          ; *                                                           *
          ; *                     Support Subroutines                   *
          ; *                                                           *
          ; *************************************************************
          ;
          ; =================================================================
          ;
          ; ERRHANDL - Loader error handler
          ;
          ; On entry:
          ;         DX = primary error code
          ;
ErrHdlAX: mov       dx,ax               ;enter here if error code is in AX
          ;
ErrHandl: mov       bp,sp               ;save frame pointer
          sub       sp,ERRMAX           ;make frame for output buffer
          loadseg   ds,cs               ;set ds
          assume    ds:@curseg          ;fix assumes
          push      dx                  ;save error code
          mov       dx,offset InitEMsg  ;get error message offset
          calldos   MESSAGE             ;display message
          pop       ax                  ;get back major error code
          ;
          mov       si,offset InitErrs  ;get error table address
          loadseg   es,ss               ;destination is on stack
          lea       di,[bp-ERRMAX]      ;buffer on stack
          push      di                  ;save address
          call      dword ptr EMOffset  ;get error message
          ;
          stocb     '$'                 ;dump a terminator into the buffer
          pop       dx                  ;get back message address on stack
          push      ds                  ;save local segment
          loadseg   ds,ss               ;point to error segment
          calldos   MESSAGE             ;write the error message
          pop       ds                  ;restore local segment
          mov       dx,offset CrLf      ;point to cr/lf
          calldos   MESSAGE             ;output that
          ;
          ; Release shell number
          ;
          mov       bl,ShellNum         ;get shell number
          or        bl,bl               ;are we the root loader?
          je        ErrClean            ;if so go on
          mov       ax,SERVSIG          ;if not, get server signature
          mov       bh,SERVSEC          ;get secondary shell function
          int       SERVINT             ;call server to close down shell num
          ;
          ; Clean up interrupts and chaining
          ;
ErrClean: call      IntClean            ;unhook interrupts
          ;
          ; Restore the old PSP chain pointer and saved INT 22 vector
          ;
          mov       ax,SavChain         ;get old chain pointer
          or        ax,ax               ;anything there?
          jz        Rest22              ;if not go on
          mov       ds:PSP_CHN,ax       ;restore old value
          ;
Rest22:   dload     ax,bx,OldSav22      ;get old INT 22 vector
          mov       cx,ax               ;copy offset
          or        cx,bx               ;anything there?
          jz        ChkRoot             ;if not don't restore
          dstore    ds:PSP_TERM,ax,bx   ;restore it in PSP
          ;
ChkRoot:  test      LMFlags,(ROOTFLAG or FATALERR)  ;root shell / fatal error?
          jnz       fatal               ;if so give up
          mov       al,0FFh             ;get error code
          calldos   TERM                ;and quit
          ;
fatal:    mov       dx,offset FatalMsg  ;point to message
          calldos   MESSAGE             ;tell user
          ;
forever:  sti                           ;be sure interrupts are enabled
          jmp       short forever       ;give up but allow keyboard
                                        ;  interrupts so Ctrl-Alt-Del works
          ;
          ; =================================================================
          ;
          ; UserPath - Parse a drive / path specification from the user
          ;
          ;
          ; On entry:
          ;         DS:SI = pointer to start of path specification, must be
          ;                 terminated by whitespace or null
          ;         DS:DI = pointer to where to put parsed path
          ;
          ; On exit:
          ;         AL = file attributes if no carry and CX > 0
          ;         CX = full name length including trailing '\' (if any)
          ;              and null
          ;         DS:SI = pointer to end of processed path spec + 1
          ;         ES:DI = pointer to null terminator at end of output path
          ;                 (not changed if DS:SI points to null or white
          ;                 space at input)
          ;         AH, BX, DX destroyed
          ;         Carry flag clear if path is OK, set if not
          ;         All other registers and interrupt state unchanged
          ;
          ;
          entry     UserPath,noframe,,local  ;set up entry point
          assume    ds:nothing          ;DS could be anywhere
          ;
	Status    2,'UP'
          loadseg   es,ds               ;point ES to local data
          push      di                  ;save output address
          mov       dx,si               ;save input address
          call      CopyTok             ;copy the path token
          cmp       si,dx               ;is the path empty?
          je        UPEmpty             ;if so get out
          mov       al,'\'              ;get backslash for termination
          cmp       bptr [di-1],al      ;terminating backslash?
          je        UPFind              ;if not go on
          stosb                         ;if not, add one
          ;
UPFind:   xor       al,al               ;get a null
          stosb                         ;terminate with it
          pop       di                  ;restore original output address
          push      si                  ;save current input address
          mov       si,di               ;point to output path
          call      FNCheck             ;normalize and find out what it is
          pop       si                  ;restore input address
          jc        UPDone              ;if something's wrong, give up with
                                        ;  carry set
          add       di,dx               ;add length to point to end
          jmp       short UPOK          ;and go on
          ;
UPEmpty:  xor       cx,cx               ;empty, show no length
          pop       di                  ;restore original output address
          ;
UPOK:     clc                           ;all OK, clear carry
          ;
UPDone:   exit                          ;all done
          ;
          ; =================================================================
          ;
          ; ChkWhite - Check if character is whitespace
          ;
          ;
          ; On entry:
          ;         AL = Character to test
          ;
          ; On exit:
          ;         Carry set if character is NUL, otherwise clear
          ;         If not NUL, zero flag set if whitespace, clear if not
          ;         All other registers and interrupt state unchanged
          ;
          ;
          entry     ChkWhite,noframe,,local  ;set up entry point
          assume    ds:nothing, es:nothing  ;fix assumes
          ;
	pushm	cx,es		;save registers
	loadseg	es,cs		;ES = current segment
	assume	es:@curseg	;fix assumes
          or        al,al               ;is it null?
          je        CWNull              ;if so flag that
          mov       cx,WhiteCnt         ;get scan length
          push      di                  ;save DI
          mov       di,offset WhiteLst  ;point to white space list
          repne     scasb               ;scan for whitespace
          pop       di                  ;restore DI
          clc                           ;show not null
          jmp       short CWDone        ;and exit
          ;
CWNull:   stc                           ;show null
          ;
CWDone:   popm	es,cx		;restore registers
	assume	es:nothing	;fix assumes
	exit                          ;all done
          ;
          ; =================================================================
          ;
          ; SkpWhite - Skip past whitespace
          ;
          ;
          ; On entry:
          ;         DS:SI = Buffer to test
          ;         CX = number of characters remaining in buffer (set to
	;	  0 for null-terminated buffer)
          ;
          ; On exit:
	;	DS:SI = address of first non-whitespace character + 1
	;	AL = first non-whitespace character
          ;         Carry set if found a NUL at DS:[SI-1], or if all
          ;	  scanned characters are whitespace, otherwise clear
          ;         CX decremented by number of characters scanned
          ;         All other registers and interrupt state unchanged
          ;
          ;
          entry     SkpWhite,noframe,,local ;set up entry point
          assume    ds:nothing, es:nothing  ;fix assumes
          ;
SWLoop:	lodsb			;get a byte
	call	ChkWhite		;whitespace or null?
	 jc	SWDone		;if null, get out
	 jnz	SWStop		;if not whitespace, get out
	loop	SWLoop		;if whitespace, loop for more
	stc			;show all whitespace
	jmp	SWDone		;and exit
	;
SWStop:	dec	cx		;adjust CX for last character
	clc			;show not all whitespace
          ;
SWDone:   exit                          ;all done
          ;
          ;
          ; =================================================================
          ;
          ; NxtWhite - Find next whitespace
          ;
          ;
          ; On entry:
          ;         DS:SI = Buffer to test
          ;         CX = number of characters remaining in buffer (set to
	;	  0 for null-terminated buffer)
          ;
          ; On exit:
	;	DS:SI = address of first whitespace character + 1
          ;         Carry set if found a NUL at DS:[SI-1], or if all
          ;	  scanned characters are not whitespace, otherwise clear
          ;         CX decremented by number of characters scanned
          ;         All other registers and interrupt state unchanged
          ;
          ;
          entry     NxtWhite,noframe,,local ;set up entry point
          assume    ds:nothing, es:nothing  ;fix assumes
          ;
NWLoop:	lodsb			;get a byte
	call	ChkWhite		;whitespace or null?
	 jc	NWDone		;if null, get out
	 jz	NWStop		;if whitespace, get out
	loop	NWLoop		;if not whitespace, loop for more
	stc			;show no whitespace
	jmp	NWDone		;and exit
	;
NWStop:	dec	cx		;adjust CX for last character
	clc			;show whitespace found
          ;
NWDone:   exit                          ;all done
          ;
          ;
          ; =================================================================
          ;
          ; UpShift - Make the character in AL upper case
          ;
          ;
          ; On entry:
          ;         AL = Character to upshift
          ;
          ; On exit:
          ;         AL = Upshifted character
          ;         All other registers and interrupt state unchanged
          ;
          ;
          entry     UpShift,noframe,,local  ;set up entry point
          assume    ds:nothing          ;fix assumes
          ;
          cmp       al,'a'              ;is it lower case?
          jb        USDone              ;if not go on
          cmp       al,'z'              ;check high end
          ja        USDone              ;if too big go on
          sub       al,32               ;make it upper case
          ;
USDone:   exit                          ;all done
          ;
          ; =================================================================
          ;
          ; CopyTok - Copy a token
          ;
          ;
          ; On entry:
          ;         DS:SI = pointer to start of token, must be terminated by
          ;                 slash, whitespace, or null, or quoted
          ;         ES:DI = pointer to where to put token
          ;
          ; On exit:
          ;         AX, CX destroyed
          ;         DS:SI = pointer to end of input token + 1
          ;         ES:DI = pointer to end of output token + 1
          ;         All other registers and interrupt state unchanged
          ;
          ;
          entry     CopyTok,noframe,,local  ;set up entry point
          assume    ds:@curseg          ;fix assumes
          ;
	lodsb                         ;get first byte of token
	cmp	al,'"'		;quoted string?
	jne	CTTest		;if not, go on
	;
CTQLoop:	lodsb			;get next byte
	or	al,al		;null?
	 je	CTDone		;if so quit
	cmp	al,'"'		;ending quote?
	 je	CTDone		;if so quit
	stosb			;copy character
	jmp	short CTQLoop	;and go back for more
	;
CTLoop:   lodsb                         ;get a byte of token
	;
CTTest:	cmp       al,'/'              ;is it a slash?
          je        CTDone              ;if so we're done
          call      ChkWhite            ;white space or null?
          jbe       CTDone              ;if either we're done
          stosb                         ;copy character
          jmp       short CTLoop        ;and go back for more
          ;
CTDone:   dec       si                  ;back up to terminator
          exit                          ;all done
          ;
          ; =================================================================
          ;
          ; CSCopy - Copy the new COMSPEC path from TempPath and save
          ;          associated data
          ;
          ;
          ; On entry:
          ;         New COMSPEC path in TempPath
          ;         CX = number of bytes to copy
          ;
          ; On exit:
          ;         AX, CX, ES destroyed
          ;         Path moved to CSPath and variables set accordingly
          ;         All other registers and interrupt state unchanged
          ;
          ;
          entry     CSCopy,noframe,,local  ;set up entry point
          assume    ds:@curseg          ;fix assumes
          ;
          pushm     si,di               ;save local registers
          mov       CSpecLen,cx         ;save length
          bset      InitFlag,CSPSET     ;show COMSPEC path found
          loadseg   es,ds               ;set ES for move
          mov       si,offset TempPath  ;get source
          mov       di,offset CSPath    ;get destination
          cld                           ;go forward
          rep       movsb               ;copy the path
          popm      di,si               ;restore local registers
          ;
          exit                          ;all done
          ;
          ; =================================================================
          ;
          ; NumSw - Convert a numeric switch from the command tail.
          ;
          ;
          ; On entry:
          ;         AL = switch character
          ;         DS:SI = address of separator before switch value
          ;
          ; On exit:
          ;         AX = switch value
          ;         DS:SI = address of first character beyond switch value
          ;         All other registers and interrupt state unchanged
          ;
          ;
          entry     NumSw,noframe,,local  ;set up entry point
          assume    ds:nothing          ;fix assumes
          ;
          push      bx                  ;save bx
          xor       bx,bx               ;clear result
          lodsb                         ;get separator
          or        al,al               ;is it the end?
          je        EndNum              ;if no more in tail go on
          ;
InLoop:   lodsb                         ;get a byte from string
          sub       al,'0'              ;convert to binary
          jb        EndNum              ;if out of digit range stop scan
          cmp       al,9                ;check high end
          ja        EndNum              ;if out of digit range stop scan
          xor       ah,ah               ;clear high byte
          shl       bx,1                ;old value * 2
          add       ax,bx               ;(old * 2) + new
          shln      bx,2                ;old * 8
          add       bx,ax               ;(old * 10) + new
          jmp       short InLoop        ;go back for more digits
          ;
EndNum:   mov       ax,bx               ;copy result
          dec       si                  ;back up in input
          pop       bx                  ;restore bx
          exit                          ;all done
          ;
          ; =================================================================
          ;
          ; ShellOut - Output the shell number
          ;
          ; On entry:
          ;         ES:DI = address of last output byte
          ;         Output buffer must contain "000" at ES:[DI-2]
          ;
          ; On exit:
          ;         AX destroyed
          ;         ES:DI = address of last output byte + 1
          ;         All other registers and interrupt state unchanged
          ;
          ;
          entry     ShellOut,noframe,,local  ;set up entry point
          assume    ds:@curseg          ;fix assumes
          ;
          mov       al,ShellNum         ;get shell number
          cmp       al,9                ;one digit?
          jle       PutSNum             ;if so go on
          dec       di                  ;back up a digit
          cmp       al,99               ;two digits?
          jle       PutSNum             ;if so go on
          dec       di                  ;back up a digit
          ;
PutSNum:  call      DecOutBU            ;put shell number as file extension
          ;
          exit                          ;all done
          ;
          ; =================================================================
          ;
          ; ResUMB - Reserve a UMB
          ;
          ; On entry:
          ;         Arguments on stack, pascal calling convention:
          ;           desired region (word, only low byte used)
          ;           paragraphs to reserve (word)
          ;
          ; On exit:
          ;         Carry clear if UMB reserved, set if error
          ;         AL = Status flags if carry clear
          ;         BX = Paragraph address of reserved block if carry clear
          ;         CX, DX, SI, DI, ES destroyed
          ;         Other registers and interrupt state unchanged
          ;
          ;
          entry     ResUMB,varframe,,local  ;set up entry point
          ;
          argW      RegSize             ;size required in paragraphs
          argW      ReqRegn             ;requested region (0 = none, 1 = any,
                                        ;  2 = region 1, etc.)
          ;
          varW      RUStatus            ;status byte
          varend
          ;
          mov       bptr RUStatus,0     ;clear status
          mov       bl,bptr ReqRegn     ;get requested region
          sub       bl,2                ;convert value to region number - 1
           jb       RUGo                ;if no region number go on
          mov       cx,URCount          ;get region count
           jcxz     RUErr               ;if no regions we have a problem
          push      cx                  ;save count
          mov       si,offset URInfo    ;point to region info table
          push      si                  ;save it
          mov       ax,(size RegInfo)   ;get region info size
          ;
RClear:   mov       [si].MinFree,0FFFFh ;close down the region
          add       si,ax               ;next region
          loop      RClear              ;do all the regions
          ;
          pop       si                  ;get back table address
          push      si                  ;and save it again
          mul       bl                  ;get offset to desired region's entry
          add       si,ax               ;point to desired region's entry
          mov       bx,RegSize          ;get required UMB size
          mov       [si].MinFree,bx     ;save as min free size for region
          pop       si                  ;get back table address again
          pop       cx                  ;get back region count
          ;
          pushc     0                   ;use region table for min free
          pushc     0                   ;no shrink
          pushc     1                   ;leave UMBs linked
          push      cx                  ;set maximum region
          push      ds                  ;stack table segment
          push      si                  ;stack table address
          mov       ax,ASM_TEXT         ;get SetUReg segment
          push      ax                  ;stack for TranCall
          mov       ax,(offset ASM_TEXT:SetUReg)  ;get SetUReg address
          push      ax                  ;stack for TranCall
          call      TranCall            ;call it
          or        ax,ax               ;check result
           jnz      RUGo                ;if OK go on
          ;
          ; Specified region was unavailable, complain and reserve without
          ; a specific region
          ;
          pushc     0                   ;set to free the region
          push      URCount             ;stack region count
          push      ds                  ;stack region table segment
          mov       si,offset URInfo    ;get region info offset
          push      si                  ;stack it
          mov       ax,ASM_TEXT         ;get FreeUReg segment
          push      ax                  ;stack for TranCall
          mov       ax,(offset ASM_TEXT:FreeUReg)  ;get FreeUReg address
          push      ax                  ;stack for TranCall
          call      TranCall            ;call it
          ;
RUErr:    bset      <bptr RUStatus>,RURegErr  ;set region error
          ;
RUGo:     xor       ah,ah               ;get UMB reserve function
          mov       bx,RegSize          ;get required UMB size
          call      DosUMB              ;try to get UMBs from DOS
           jc       TryXMS              ;if it didn't work go try XMS
          Status    2,'U5'              ;(Use DOS 5 UMB)
          mov       bx,ax               ;copy block address to BX
          bset      <bptr RUStatus>,RUDOSUMB  ;show DOS UMB in use
          jmp       short RUExit        ;go save it
          ;
TryXMS:   test      InitFlag,XMSAVAIL   ;any XMS there?
           jz       RUFail              ;no, forget it
          mov       dx,RegSize          ;get required UMB size
          callxms   REQUM,XMSDrvr       ;allocate pages
          or        ax,ax               ;any error?
           jz       RUFail              ;if so can't use UMB
          Status    2,'UX'              ;(Use XMS UMB)
          ;
          ; Clean up any UMB region settings used for reserving UMB
          ;
RUExit:   push      bx                  ;save block address
          ;
          pushc     0                   ;set to free the region
          push      URCount             ;stack region count
          push      ds                  ;stack region table segment
          mov       si,offset URInfo    ;get region info offset
          push      si                  ;stack it
          mov       ax,ASM_TEXT         ;get FreeUReg segment
          push      ax                  ;stack for TranCall
          mov       ax,(offset ASM_TEXT:FreeUReg)  ;get FreeUReg address
          push      ax                  ;stack for TranCall
          call      TranCall            ;call it
          ;
          pop       bx                  ;restore block address
          mov       al,RUStatus         ;retrieve status flags
          clc                           ;show all OK
          jmp       short RUDone        ;and get out
          ;
RUFail:   stc                           ;show error
          ;
RUDone:   exit                          ;all done
          ;
          ; =================================================================
          ;
          ; TranCall - Call a routine in the transient portion
          ;
          ; On entry:
          ;         Top word on stack = address of routine to call
          ;         Second word on stack = segment of routine to call
          ;         Registers / stack set up for called routine
          ;
          ; On exit:
          ;         Registers as returned from called routine
          ;
          ;
TranCall  proc      near                ;set up entry point
          assume    ds:@curseg          ;fix assumes
          ;
          pop       TCRet               ;save return address
          pop       TCLoc.foff          ;save offset for call
          mov       TCAX,ax             ;save AX
          pop       ax                  ;get segment for call
          sub       ax,SERV_TEXT        ;make it relative
          add       ax,TranCur          ;make it correct for current location
                                        ;  of transient portion
          mov       TCLoc.fseg,ax       ;save it for CALL
          mov       ax,TCAX             ;restore AX
          push      cs                  ;return segment
          push      TCRet               ;return offset
          jmp       dword ptr TCLoc     ;"call" the routine
          ;
TranCall  endp
          ;
          ; =================================================================
          ;
          ; StatMsg - Display status message
          ;
          ; On entry:
          ;         DS:DX = status message address
          ;
          ; On exit:
          ;         AX, DX destroyed
          ;         All other registers and interrupt state unchanged
          ;
          ;
          entry     StatMsg,noframe,,local  ;set up entry point
          assume    ds:@curseg          ;fix assumes
          ;
	ife	_RT
          test      InitFlag,TRANLOAD   ;transient load?
          jnz       StatDone            ;if so no status messages
          calldos   MESSAGE             ;display message
	endif
          ;
StatDone: exit                          ;all done
          ;
          ; =================================================================
          ;
          ; DebugMsg - Display debugging message
          ;
          ; On entry:
          ;         CS-relative message address on stack, first byte of
          ;         message is debug message level
          ;
          ; On exit:
          ;         All registers and interrupt state unchanged
          ;
          ;
          if        DEBUG ge 2          ;only assemble if debugging
          entry     DebugMsg,argframe,,local  ;set up entry point
          assume    ds:nothing, es:nothing    ;registers could be anywhere
          ;
          argW      MsgAddr             ;message address
          ;
          test      cs:InitFlag,DBMSG   ;displaying debug messages?
          jz        DMDone              ;if not get out
          pushm     ax,bx,dx,ds         ;save all registers used
          loadseg   ds,cs               ;set message segment
          mov       bx,MsgAddr          ;get message offset
          mov       al,[bx]             ;get message level
          cmp       al,DBLevel          ;check against user specified level
          jg        DMPop               ;if message level is higher, skip it
          mov       dx,offset DBPrefix  ;get prefix offset
          calldos   MESSAGE             ;display it
          mov       dx,bx               ;get message address - 1
          inc       dx                  ;get message address
          calldos   MESSAGE             ;display it
          cmp       DBLevel,9           ;level 9?
          jl        DMPop               ;if not go on
          ;
DMKeyRd:  mov       ah,1                ;check keyboard buffer
          int       16h                 ;any character there?
          jz        DMRdChr             ;if not go read the real character
          xor       ah,ah               ;read keyboard service
          int       16h                 ;read key from buffer
          jmp       short DMKeyRd       ;loop until buffer is empty
          ;
DMRdChr:  xor       ah,ah               ;read keyboard service
          int       16h                 ;read key from buffer
          ;
DMPop:    popm      ds,dx,bx,ax         ;restore registers
          ;
DMDone:   exit                          ;all done
          ;
          else                          ;different version for released code
          entry     DebugMsg,argframe,,local  ;set up entry point
          assume    ds:nothing, es:nothing    ;registers could be anywhere
          ;
          argW      OrigAX              ;saved AX value (saved by caller)
          ;
          test      cs:InitFlag,DBMSG   ;displaying debug messages?
           jz       DMDone              ;if not get out
          pushm     bx,cx,dx,ds         ;save all registers used
          loadseg   ds,cs               ;set message segment
          mov       bl,ah               ;get high byte of code
          shrn      bl,4,cl             ;slide over
          cmp       bl,DBLevel          ;check against user specified level
           jg       DMPop               ;if message level is higher, skip it
          mov       cl,2                ;get shift count
          shl       ax,cl               ;split characters at byte boundary
          and       ah,3Fh              ;clean up left half
          shr       al,cl               ;clean up right half
          add       ax,2020h            ;convert to ASCII
          mov       DBMCode,ax          ;store for output
          mov       dx,offset DBPrefix  ;get message address
          calldos   MESSAGE             ;display it
          cmp       DBLevel,9           ;level 9?
           jl       DMPop               ;if not go on
          xor       ah,ah               ;read keyboard service
          int       16h                 ;read key from buffer
          ;
DMPop:    popm      ds,dx,cx,bx         ;restore registers
          ;
DMDone:   mov       ax,OrigAX           ;restore AX
          exit                          ;all done
          endif                         ;debugging code
          ;
          ; =================================================================
          ;
          ; SetDBLev - Set debugging message level
          ;
          ; On entry:
          ;         DS:SI = address of byte after "/Z" in command tail
          ;
          ; On exit:
          ;         AX destroyed
          ;         SI incremented if level byte found, otherwise unchanged
          ;         All other registers and interrupt state unchanged
          ;
          ;
          entry     SetDBLev,noframe,,local  ;set up entry point
          assume    ds:@curseg          ;fix assumes
          ;
          bset      InitFlag,DBMSG      ;set message flag
          mov       al,[si]             ;get next byte
          sub       al,'0'              ;check low limit, adjust for ASCII
          jb        DBLDone             ;if too low ignore it
          cmp       al,9                ;check high limit
          ja        DBLDone             ;if too high ignore it
          mov       DBLevel,al          ;set level
          inc       si                  ;and skip the byte
          ;
DBLDone:  exit                          ;all done
          ;
          ; =================================================================
          ;
          ; HOOKINT - Hook an interrupt
          ;
          ; On entry:
          ;         AL = interrupt number
          ;         CL = hook flag bit mask for IntFlags
          ;         DX = address of new handler
          ;         DI = old vector storage address
          ;         DS = segment of new handler
          ;
          ; On exit:
          ;         AX:BX = old interrupt vector
          ;         Other registers and interrupt state unchanged
          ;
          entry     HookInt,noframe,,local    ;hook an interrupt
          assume    ds:nothing          ;fix assumes
          ;
          push      es                  ;save es
          pushm     ax,ds               ;save interrupt number and handler
                                        ;  segment
          loadseg   ds,cs               ;set DS to local segment
          calldos   GETINT              ;get old vector
          mov       [di].fseg,es        ;save segment
          mov       [di].foff,bx        ;save offset
          popm      ds,ax               ;get back segment and int number
          calldos   SETINT              ;set interrupt handler address
          bset      cs:IntFlags,cl      ;show it's hooked
          mov       ax,es               ;save old segment in ax
          ;
          pop       es                  ;restore es
          exit                          ;all done
          ;
          ; =================================================================
          ;
          ; DefRegn - Define a region swap control block (SCB)
          ;
          ; On entry:
          ;         AX = offset of corresponding device SCB
          ;         BX = relative paragraph address of region
          ;         CX = region length in paragraphs
          ;         DL = flags
          ;         SI = offset of start of region on swap device, in
          ;                paragraphs
          ;         DI = address of region SCB
          ;
          ; On exit:
          ;         Region SCB initialized
          ;         All registers and interrupt state unchanged
          ;
          entry     DefRegn,noframe,,local  ;define entry
          assume    ds:@curseg          ;fix assumes
          ;
          mov       [di].DevPtr,ax      ;set offset of device SCB
          mov       bptr [di].SwapFlag,dl  ;set flags
          mov       [di].SwapOff,si     ;set offset of this block
          mov       [di].SwapMSeg,bx    ;set segment of this block
          mov       [di].SwapLen,cx     ;set length of block in paragraphs
          ;
          exit                          ;that's all
          ;
          ; =================================================================
          ;
          ; DefDev - Define a device swap control block (SCB)
          ;
          ; On entry:
          ;         AL = swap type
          ;         BX = swap handle
          ;         DI = address of device SCB
          ;
          ; On exit:
          ;         Device SCB initialized
          ;         All registers and interrupt state unchanged
          ;
DevTabl   label     word                ;branch table for varying swap types
          dw        DevEMS              ;EMS swapping
          dw        DevXMS              ;XMS swapping
          dw        DevDisk             ;disk swapping
          ;
          entry     DefDev,noframe,,local  ;define entry
          assume    ds:@curseg          ;fix assumes
          ;
          pushm     bx,cx,si,di,es      ;save registers changed
          mov       [di].SwapType,al    ;set swapping type
          mov       [di].SwapHdl,bx     ;set handle
          mov       bx,ShiftAdd         ;get checksum values
          mov       [di].SwapSum,bx     ;save in SCB
          mov       bl,al               ;copy swap type
          xor       bh,bh               ;clear high byte
          shl       bx,1                ;shift for word offset
          jmp       wptr DevTabl[bx]    ;and jump to appropriate handler
          ;
DevEMS:   mov       bx,FrameSeg         ;get frame segment
          mov       wptr [di].SwapSeg,bx  ;save it in SCB
          jmp       short DDevExit      ;and exit
          ;
DevXMS:   dload     bx,cx,XMSDrvr       ;get driver address
          mov       wptr [di].SwapDrvr,bx    ;save offset in SCB
          mov       wptr [di].SwapDrvr+2,cx  ;save segment in SCB
          jmp       short DDevExit      ;and exit
          ;
DevDisk:  lea       di,[di].SwapSig     ;point to signature buffer in block
          loadseg   es,ds               ;set destination segment
          mov       si,offset DiskSig   ;point to local buffer
          mov       cx,SIGBYTES         ;get signature length
          cld                           ;go forward
          rep       movsb               ;move signature into place
          ;
DDevExit: popm      es,di,si,cx,bx      ;restore registers
          exit                          ;that's all
          ;
          ; =================================================================
          ;
          ; AddFree - Add to the list of blocks to be freed on termination
          ;
          ; On entry:
          ;         AH = block type (0 = DOS, 1 = DOS UMB, 2 = XMS UMB)
          ;         BX = block address
          ;
          ; On exit:
          ;         SI destroyed
          ;         All other registers and interrupt state unchanged
          ;
          entry     AddFree,noframe,,local  ;define entry
          assume    ds:@curseg          ;fix assumes
          ;
          Status    2,'AF'              ;(Add to free list)
          mov       si,FreePtr          ;get free list pointer
          mov       MFreeLst[si].FreeType,ah  ;set type
          mov       MFreeLst[si].FreeSeg,bx   ;set block address
          add       FreePtr,(size FreeItem)   ;bump to next entry
          ;
          exit                          ;that's all
          ;
; =========================================================================
          ;
          ; Include interrupt cleanup routines
          ;
          include   INTCLEAN.ASM
          ;
; =========================================================================
          ;
          ; "Plug-in" modules to be added to 4DOS.COM as needed.   These
          ; include:
          ;
          ;         - Swap-in routines to swap the server code in from
          ;           XMS, EMS, or disk
          ;         - Shell table maintenance
          ;         - Interrupt 2E handling
          ;
          ; Note that the error handler for these routines is set to the
          ; resident error handler, not the initialization error handler
          ; (ErrHandl).  Calls are made indirectly via the pointer ResErrP.
          ;
          errset    <wptr ResErrP>      ;set error handler name
          ;
          ;
          ; ------- Server swap in modules -------
          ;
          ;
          ; XMSIN -- Swap server in from XMS
          ;
          StartMod  XMS                 ;define loadable module start
          ;
          entry     XMSIn,noframe       ;XMS swap in
          assume    ds:@curseg          ;fix assumes
          ;
          mov       si,offset XMSMove   ;point to move control block
          mov       ax,ServSeg          ;get new destination segment
          mov       [si].DestSeg,ax     ;set server seg
          callXMS   MOVE,XMSDrvr,XMSIN  ;move the data
          ;
          exit                          ;and return
          ;
          EndMod    XMS                 ;define loadable module end
          ;
          ;
          ; EMSIN -- Swap server in from EMS
          ;
          StartMod  EMS                 ;define loadable module start
          ;
          entry     EMSIn,noframe       ;EMS swap in
          assume    ds:@curseg          ;fix assumes
          ;
          mov       dx,LMHandle         ;get EMS handle
          callems   SMAP                ;save the current mapping state
          xor       al,al               ;clear physical page number
          xor       bx,bx               ;clear logical page number
          callems   MAP,EMSMPI          ;map the first page
          mov       cx,ServLenW         ;get word count for transfer
          mov       es,ServSeg          ;set destination segment
          mov       ds,FrameSeg         ;set source
          assume    ds:nothing          ;fix assumes
          xor       si,si               ;source starts at zero
          xor       di,di               ;same for destination
          cld                           ;moves go forward
          rep       movsw               ;copy the block
          callems   RMAP                ;restore the mapping state
          ;
          exit                          ;and return
          ;
          EndMod    EMS                 ;define loadable module end
          ;
          ;
          ; DISKIN -- Swap server in from disk
          ;
          StartMod  Disk                ;define loadable module start
          ;
          entry     DiskIn,noframe      ;disk swap in
          assume    ds:@curseg          ;fix assumes
          ;
          ; Test swap area checksum
          ;
          mov       cx,ServLenW         ;get server length in words
          add       cx,7                ;add for paragraph roundoff
          and       cx,0FFF8h           ;make it even paragraphs
          mov       ds,ServSeg          ;get starting segment
          assume    ds:nothing          ;fix assumes
          xor       si,si               ;start at byte 0
          xor       bx,bx               ;clear low-order checksum
          xor       dx,dx               ;clear high-order checksum
          ;
ChkLoop:  lodsw                         ;load a word
          add       bx,ax               ;add to sum
          adc       dx,0                ;add any carry
          loop      ChkLoop             ;loop until checksum is done
          ;
          loadseg   ds,cs               ;make DS local again
          assume    ds:@curseg          ;fix assumes
          cmp       dx,wptr ServChk[2]  ;high-order match?
          jne       DiskGo              ;if not do the swap
          cmp       bx,wptr ServChk     ;low-order match?
          je        DiskRet             ;if so we're done
          ;
          ; Test signature
          ;
DiskGo:   test      LMFlag2,DREOPEN     ;is reopen enabled?
           jz       DoSeek              ;if not just go seek
          call      dword ptr ReopenP   ;check if reopen needed
          ;
DoSeek:   mov       bx,LMHandle         ;get swap file handle
          xor       dx,dx               ;clear low word of seek offset
          xor       cx,cx               ;clear high word
          mov       al,SEEK_BEG         ;seek is relative to start
          calldos   SEEK,DSKSK          ;seek to beginning of file
          ;
SeekOK:   mov       dx,offset SigBuf    ;point to buffer
          mov       si,dx               ;save address for compare
          mov       cx,SIGBYTES         ;get signature length
          calldos   READ,DSKRD          ;read the test string from disk
          loadseg   es,ds               ;set ES for comparison
          mov       di,offset DiskSig   ;point to signature for compare
          mov       cx,SIGBYTES         ;get signature length
          repe      cmpsb               ;is file OK?
          je        SwFileOK            ;continue if so
          bset      LMFlags,FATALERR    ;set fatal error flag
          error     DSKINT              ;and give up
          ;
SwFileOK: mov       ds,ServSeg          ;get starting segment
          assume    ds:nothing          ;no assume any more
          xor       dx,dx               ;offset is always zero
          mov       cx,cs:ServLenW      ;get word count for transfer
          shl       cx,1                ;make it a byte count
          calldos   READ,DSKRD          ;read the block from disk
          ;
DiskRet:  exit                          ;and return
          ;
          EndMod    Disk                ;define loadable module end
          ;
          ;
          ; ------- Swap file reopen modules -------
          ;
          ; Data for swap file reopen
          ;
          StartMod  RODat               ;define loadable module start
          ;
ROPath    path      <>                  ;swap file path
          ;
          EndMod    RODat               ;define loadable module end
          ;
          ;
          ; SWREOPEN -- Reopen swap file on seek error (for use with
          ;             Netware), seeks to location 0 as well.  Called by
          ;             both resident and transient code.  Leaves new file
          ;             handle in AX.
          ;
          StartMod  ReOpn               ;define loadable module start
          ;
          entry     SwReOpen,noframe,far  ;swap file reopen
          ;
          push      ds                  ;save DS
          loadseg   ds,cs               ;make it local
          assume    ds:@curseg          ;fix assumes
          ;
          mov       bx,LMHandle         ;get swap file handle
          xor       dx,dx               ;clear low word of seek offset
          xor       cx,cx               ;clear high word
          mov       al,SEEK_BEG         ;seek is relative to start
          calldos   SEEK                ;seek to beginning of file
           jnc      RODone              ;if it succeeded go on
          ;
          mov       dx,RODP             ;point to name
          mov       al,ROAccess         ;get access bits
;;LFN
          calldos   OPEN,DSKRO          ;open the file, give up if we can't
          mov       LMHandle,ax         ;save the handle
          mov       bx,ax               ;leave handle in BX
          ;
RODone:   pop       ds                  ;restore DS
          exit                          ;and return
          ;
          EndMod    ReOpn               ;define loadable module end
          ;
          ;
          ; ------- Shell table modules -------
          ;
          ; Shell table data
          ;
          StartMod  ShDat               ;define loadable module start
          ;
ShellTab  db        SHTBYTES dup (0)    ;define table, fill with zeros
          ;
          EndMod    ShDat               ;define loadable module end
          ;
          ;
          ; New shell code
          ;
          StartMod  ShMan               ;define loadable module start
          ;
          ; ShellMan - Shell number manager
          ;
          ; On entry:
          ;         BL = 0 to start a new shell, or the terminating shell
          ;              number to terminate a shell
          ;         No requirements
          ;
          ; On exit:
          ;         BL = shell number if starting a new shell, otherwise
          ;              undefined
          ;         Carry set if error (no number available), clear if all
          ;           OK
          ;         AX, BH, CX destroyed
          ;         All other registers unchanged, interrupts on
          ;
          ; NOTE:  This routine disables interrupts temporarily while
          ; checking for an available shell number.  This is because in
          ; a multitasking system this code must be re-entrant.
          ;
          ;
          entry     ShellMan,noframe    ;set up entry
          assume    ds:@curseg          ;fix assumes
          ;
          or        bl,bl               ;check what we're doing
           jnz      TrmShell            ;go terminate if that's the request
          mov       bx,ShellDP          ;get pointer to shell data
          mov       cx,SHTBYTES         ;get table length
          mov       al,1                ;start shell counter at 1
          ;
ByteLoop: mov       ah,80h              ;start with bit 7
          push      cx                  ;save cx
          mov       cx,8                ;get bit count
          ;
BitLoop:  cli                           ;hold interrupts
          test      [bx],ah             ;test bit
           jz       GotBit              ;if not set we found it
          sti                           ;allow interrupts
          shr       ah,1                ;move to next bit
          inc       al                  ;bump shell counter
          loop      BitLoop             ;and continue
          ;
          pop       cx                  ;restore cx
          inc       bx                  ;move to next byte
          loop      ByteLoop            ;try again
          ;
          stc                           ;no room, error
          jmp       short SMDone        ;all done
          ;
GotBit:   bset      [bx],ah             ;set bit
          sti                           ;allow interrupts
          pop       cx                  ;restore cx
          mov       bl,al               ;copy shell number to bl
          jmp       short SMOK          ;and exit
          ;
          ; Come here to terminate a shell
          ;
TrmShell: dec       bl                  ;make shell number 0-based
          mov       cl,bl               ;copy it
          shrn      bl,3                ;make byte number
          xor       bh,bh               ;clear high byte
          and       cl,7                ;make shift count
          mov       ah,7Fh              ;get zero bit to shift
          ror       ah,cl               ;shift bit into place
          add       bx,ShellDP          ;get pointer to shell data byte
          and       [bx],ah             ;clear it
          ;
SMOK:     clc                           ;show all OK
          ;
SMDone:   exit                          ;that's all folks
          ;
          EndMod    ShMan               ;define loadable module end
          ;
          ;
          ; ------- Interrupt 2E handler modules -------
          ;
          ; Standard do-nothing INT 2E handler
          ;
          StartMod  I2ES                ;define loadable module start
          ;
Int2ES    proc      far
          assume    ds:@curseg          ;fix assumes
          ;
          iret                          ;just return
          ;
Int2ES    endp
          ;
          EndMod    I2ES                ;define loadable module end
          ;
          ;
          ; Full support INT 2E handler
          ;
          StartMod  I2EF                ;define loadable module start
          ;
Int2EF    proc      far                 ;full INT 2E handler module
          assume    ds:@curseg          ;fix assumes
          ;
          mov       di,ds               ;save segment of command; command
                                        ;  address stays in DI:SI for a bit
          loadseg   ds,cs               ;set DS to local
          test      cs:LMFlags,IGNORE23+INSHELL  ;in the shell or swapping?
           jnz      I2EErr              ;if so complain
          test      LMFlags,DOS3        ;DOS 3 or above?
           jz       I2EErr              ;if not go on
          calldos   GETPSP              ;get caller's PSP
          mov       cx,bx               ;copy it
          mov       bx,PSPSeg           ;get our PSP segment
          calldos   SETPSP              ;set the current PSP
          mov       bx,0FFFFh           ;get max paragraphs
          calldos   ALLOC               ;get size of largest block
          mov       dx,bx               ;copy size of largest block
          calldos   ALLOC               ;allocate that block
           jc       I2EErr              ;if can't get it complain
          sub       dx,TPLenP           ;get length without transient
          add       dx,ax               ;add base to get new server address
          mov       ServSeg,dx          ;save it for swappers
          mov       es,ax               ;copy segment
          calldos   FREE                ;and free it
          ;
          ;         Next 3 items are restored by server
          ;
          push      cx                  ;save caller's PSP 
          push      wptr LMFlags        ;save flags (2 bytes)
          push      wptr ExecSig        ;save EXEC signature (and next byte)
          ;
          pushm     di,si               ;save command address
          call      wptr SwapInP        ;swap server back in
          popm      si,di               ;restore address to DI:SI for server
          loadseg   es,cs               ;set ES for server
          mov       ax,cs:ServSeg       ;get server segment
          mov       ds,ax               ;set DS for server
          push      ax                  ;on stack for RETF
          push      cs:ServI2E          ;get offset of INT 2E routine
          retf                          ;and go to it (never returns)
          ;
I2EErr:   mov       ax,0FFFFh           ;set error value
          iret                          ;and return
          ;
Int2EF    endp
          ;
          EndMod    I2EF                ;define loadable module end
          ;
          ;
          ; ------- Critical error text module -------
          ;
          ;
          StartMod  Crit                ;define loadable module start
          ;
          ifdef     ENGLISH
          include   CRITERRS.ASM        ;load critical error text
          endif
          ;
          ifdef     GERMAN
          include   CRITERRS.ASD        ;load critical error text
          endif
          ;
          EndMod    Crit                ;define loadable module end
          ;
          ;
          ; ------- COMMAND.COM message handler initialization module -------
          ;
          ;
          StartMod  CMIni               ;define loadable module start
          ;
          ;
          ; Get COMMAND.COM message server information; entered via a jump
          ; from INT 2F handler if this code is active and AX = 122Eh.
          ;
          ;         DL = 0, 2, or 4:  Return info on Extended, Parse, or
          ;         critical error tables respectively; this code returns
          ;         ES = 1 (like COMMAND.COM) and DI = address of the 
          ;         appropriate error table for message lookup.
          ;
          ;         DL = 6:  Return info on unknown error table, returns
          ;         ES:DI = 0:0.
          ;
          ;         DL = 8:  Return address of error message routine, returns
          ;         the contents of CMHPtr, which (by the time this code
          ;         runs) points to the message handler code below.
          ;
CCMsg:    popf                          ;clean up stack
          push      bx                  ;save BX
          test      dl,1                ;odd function?
          jnz       CCRet               ;if so give up
          mov       bx,1                ;get default for ES
          mov       es,bx               ;copy it to ES
          mov       di,wptr cs:ShellInf.ShCETAdr  ;get offset for crit errs
          mov       bl,dl               ;get function
          shr       bl,1                ;divide by 2 so we can use DECs
          dec       bl                  ;check if it was 0 or 2
          jl        CCRet               ;0, we're all done
          je        CCPrsAdr            ;2, get parse error info
          dec       bl                  ;maybe it was 4?
          je        CCRet               ;if so same as 0
          sub       bl,2                ;no, check for 6, 8, or larger
          je        CCSubAdr            ;if 8 go get subroutine address
          jmp       short CCZero        ;otherwise clear ES:DI and return
          ;
CCPrsAdr: mov       di,cs:PEPtr         ;parse error table address
          jmp       short CCRet         ;and return to caller
          ;
CCZero:   xor       di,di               ;unknown function, return 0
          mov       es,di               ;in ES too
          jmp       short CCRet         ;and return to caller
          ;
CCSubAdr: mov       di,cs:CMHPtr        ;get offset of message routine
          loadseg   es,cs               ;get segment
          ;
CCRet:    pop       bx                  ;restore BX
          iret                          ;all done
          ;
          EndMod    CMIni               ;define loadable module end
          ;
          ;
          ; ------- COMMAND.COM message handler module -------
          ;
          ;
          StartMod  CMHdl               ;define loadable module start
          ;
          ;
          ; COMMAND.COM error message handler
          ;
          ; On entry:
          ;         AX = error number
          ;         ES:DI = 1:base to add to message number (returned from
          ;                 INT 2F fn 122E code above)
          ;
          ; On exit:
          ;         Carry set if error, clear if message retrieved
          ;         ES:DI = message buffer address if carry clear
          ;
          ; Notes:
          ;         Uses bottom of local stack as buffer.  This area won't be
          ;         in use as an external program must be running when this
          ;         code is called, and most of the stack is unused when
          ;         externals are running.  Leaves the very bottom 16 bytes
          ;         of the stack untouched as PCED uses this area to detect
          ;         secondary shells.
          ;
          ;
CCGetMsg  proc      far                 ;COMMAND.COM message retriever
          ;
          pushm     ax,bx,cx,dx,si,ds   ;save everything
          ;
          loadseg   ds,cs               ;set DS
          mov       bx,es               ;get ES
          cmp       bx,1                ;should be 1
          jne       CCGFail             ;holler if it failed
          mov       si,di               ;copy table address
          loadseg   es,cs               ;point to this segment
          mov       di,(offset LocStack + 16)  ;use bottom of stack as buffer
          push      di                  ;save that
          inc       di                  ;point to text area
          call      dword ptr EMOffset  ;get error message
          pop       di                  ;restore buffer address
          mov       [di],cl             ;store byte count (preserves carry)
          jmp       short CCGRet        ;and return with carry flag from
                                        ;  ErrMsg
          ;
CCGFail:  stc                           ;force failure
          ;
CCGRet:   popm      ds,si,dx,cx,bx,ax   ;restore everything
          ret                           ;and return
          ;
CCGetMsg  endp
          ;
          EndMod    CMHdl               ;define loadable module end
          ;
          ;
          ; ------- Parse error text module -------
          ;
          ;
          StartMod  PErr                ;define loadable module start
          ;
          ifdef     ENGLISH
          include   PARSERRS.ASM        ;load English parse error text
          endif
          ;
          ifdef     GERMAN
          include   PARSERRS.ASD        ;load German parse error text
          endif
          ;
          EndMod    PErr                ;define loadable module end
          ;
; =========================================================================
          ;
          ;
          ; "Slider" code used to move the transient portion of 4DOS down
          ; over the initialization code
          ;
          ;
          assume    ds:nothing          ;no assumptions
          ;
Slider    label     near                ;slider code starts here
public    Slider
          ;
          pop       dx                  ;get length to swap, in paragraphs
          pop       ds                  ;get starting source paragraph
          pop       es                  ;get starting destination paragraph
          cld                           ;move forward
          ;
          ; Slide 4DOS down in memory
          ;
SlidLoop: mov       bx,dx               ;copy remaining paragraphs
          maxu      bx,SLIDEMAX         ;reduce to max transfer size
          mov       cx,bx               ;copy transfer paragraph count
          shln      cx,3                ;make word count
          xor       si,si               ;source starts at byte 0
          xor       di,di               ;so does destination
          rep       movsw               ;copy the block
          mov       ax,ds               ;get source address
          add       ax,bx               ;add transfer paragraphs
          mov       ds,ax               ;put it back
          mov       ax,es               ;get destination address
          add       ax,bx               ;add transfer paragraphs
          mov       es,ax               ;put it back
          sub       dx,bx               ;decrement remaining paragraph count
          ja        SlidLoop            ;loop if there is more to transfer
          ;
          ; Free our own memory block
          ;
          loadseg   es,cs               ;get segment of our block
          calldos   FREE                ;free it
          ;
          ; Adjust size of 4DOS memory block
          ;
          ; Note:  Since the slider is loaded just above the transient
          ; portion as read from the EXE file, but the actual size of the
          ; transient portion is considerably larger (due to _BSS, stack,
          ; and heap areas), this memory block adjustment will actually
          ; expand the block so that it ends after the slider code instead
          ; of before.  This "envelops" the slider inside the stack / heap
          ; area, but should not cause any trouble as the slider is *much*
          ; smaller than the size of the exapnsion and hence the new MCB
          ; at the end of the transient portion will be far above the slider
          ; code, and will not clobber it.  The same is true for the
          ; transient portion's stack, which is used below to hold the
          ; address for starting the transient code.
          ;
          pop       cx                  ;get psp segment
          mov       es,cx               ;copy psp for reallocation and later
          pop       ax                  ;get new end of allocation block
          mov       es:PSP_MEND,ax      ;save it
          pop       bx                  ;get new total length
          calldos   REALLOC             ;and reallocate the block
          ;
          ; Set up registers and start 4DOS
          ;
          pop       bx                  ;get initial SS / DS
          pop       cx                  ;get initial SP
          pop       ax                  ;get DGROUP length
          popm      si,di               ;get offset / segment of 4DOS entry
          mov       ds,bx               ;set ds
          cli                           ;hold interrupts
          mov       ss,bx               ;set SS
          mov       sp,cx               ;set SP
          pushm     di,si               ;push segment / offset of 4DOS entry
          retf                          ;start 4DOS
          ;
SlCodLen  equ       $ - Slider          ;code length in bytes
          ;
          ; Define initialization endpoint; leave room for an MCB after it,
          ; so that  we can reduce memory allocation to this point without
          ; the new MCB clobbering the transient portion.
          ;
InitEnd   db        ?                   ;last byte needed to complete
                                        ;  initialization after transient
                                        ;  portion is moved
          db        16 dup (?)
          ;
@curseg   ends                          ;close segment
          ;
          end

