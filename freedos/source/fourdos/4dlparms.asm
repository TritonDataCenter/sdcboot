

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


          ;
          ; 4DLPARMS.ASM -- Include file for 4DLINIT.ASM, 4DOS.ASM, and
          ; SERVER.ASM
          ;
          ; Copyright 1989-1999, J.P. Software, All Rights Reserved
          ;
          ; Parameters for 4DOS low-memory loader (4DOS.ASM) and high-memory
          ; server (SERVER.ASM).
          ;
          ;
          ; SERVER.ASM interface parameters
          ;
SIGDISAB  equ       0FFFEh              ;signal disable pointer value
SIGENAB   equ       0FFFDh              ;signal enable pointer value
          ;
          ;
CTRLCVAL  equ       -3                  ;error return value for ^C
          ;
          ;
          ; Loader interrupt parameters
          ;
          ;    Note:  INTVER is a 16-bit internal version number
          ;    used to ensure proper alias / function / history inheritance and
          ;    reduced swapping.  It should be changed for each
          ;    release.  It also varies by product and language.
          ;    VERSION is the version number returned by an INT 2F
          ;    call; it should be set to return the correct value
          ;    a la INT 21 function 30.
          ;
PRODBITS	=	0		;default to adding nothing
          include   intver.asm          ;include file with automatically
                                        ;  incremented version number
	if	_RT
PRODBITS	=	(PRODBITS or 100h)	;set bit for runtime
	endif

	ifdef	GERMAN
PRODBITS	=	(PRODBITS or 200h)	;set bit for German version
	endif
INTVER	=	(INTVER or PRODBITS)  ;add new bits to INTVER
	;
VERSION   equ       3207h               ;4DOS version 7.50
          ;
SERVINT   equ       I_MPX               ;server uses multiplex interrupt
SERVSIG   equ       0D44Dh              ;server signature value
SERVRSIG  equ       044DDh              ;returned signature value
SERVINQ   equ       0                   ;inquiry function code
SERVSWAP  equ       1                   ;get previous code segment swap info
SERVSEC   equ       2                   ;secondary shell start / end function
SERVNEW   equ       0FFh                ;4DOS 2/3 allocate new shell function
SERVTERM  equ       0FEh                ;4DOS 2/3 terminate shell function
SERVDEL	equ	90		;ticks to delay to read error msg
          ;
          ; Shell table parameters
          ;
MAXSHELL  equ       128                 ;maximum number of shells
SHTBYTES  equ       (MAXSHELL + 7) / 8  ;bytes in table (1 bit / shell)
          ;
          ; Miscellaneous parameters
          ;
GDCNT     equ       2                   ;words in global data area
HEAPADD   equ       80h                 ;paragraphs to add to minimum to
                                        ;  allow for 2K heap space
STACKADJ  equ       256                 ;slop area for transient stack
STACKSIZ  equ       320                 ;4DOS loader local stack
LOWSTSAV  equ       64                  ;bytes of local stack to save during
                                        ;  INT 2E
MODMAX    equ       2048                ;maximum total space for all loadable
                                        ;  modules (bytes)
DRENVMAX  equ       MODMAX - 2          ;maximum total space for DR-DOS env
                                        ;  variables passed from CONFIG.SYS;
                                        ;  ***must fit within both MODMAX and
                                        ;  STACKDEF-1024 (STACKDEF is below)
BIGSTACK  equ       MODMAX              ;size of large stack needed for
                                        ;  calling INIParse -- same as MODMAX
                                        ;  since module area is used as stack
MAXREG    equ       8                   ;maximum number of regions for any
                                        ;  region-specific UMB access
MAXFREE   equ       6                   ;maximum number of memory blocks we
                                        ;  have to free (resident portion,
                                        ;  master env, global aliases / funcs /
                                        ;  cmd history / dir history)
MAXLR     equ       5                   ;maximum number of low memory blocks
                                        ;  we can reserve (master env, global
                                        ;  aliases, global functions, global cmd 
					;  history, global dir history)
RELOCMAX  equ       6900                ;maximum relocation table bytes
INISMAX   equ       1536                ;maximum length of INI file strings
INIKMAX   equ       64                  ;maximum INI file key substitutions
ERRMAX    equ       80                  ;maximum error message length
          ;
SIGBYTES  equ       8                   ;disk swap file signature length
URETRIES  equ       16                  ;unique swap file name retry count
TAILMAX   equ       128                 ;length of command tail buffer
AEMAX     equ       254                 ;max length of AUTOEXEC name + parms
LOADHIGH  equ       01h                 ;load high bit for loadexe
MINMEM    equ       02h                 ;minimum memory bit for loadexe
ALSMIN    equ       256                 ;minimum alias buffer size
ALSDEF    equ       1024                ;default alias buffer size
ALSMAX    equ       32767               ;maximum alias buffer size
HISTMIN   equ       256                 ;minimum cmd history size
HISTDEF   equ       1024                ;default cmd history size
HISTMAX   equ       8192                ;maximum cmd history size
DHISTMIN  equ       256                 ;minimum dir history size
DHISTDEF  equ       256                 ;default dir history size
DHISTMAX  equ       2048                ;maximum dir history size
ENVMFREE  equ       128                 ;minimum free environment space
ENVMIN    equ       160                 ;minimum environment size
          ife       _FLAT
ENVDEF    equ       512                 ;default environment size
          else
ENVDEF    equ       4096                ;default environment size
          endif
ENVMAX    equ       32767               ;maximum environment size
STACKDEF  equ       8192                ;default transient stack size
STACKW95  equ       10240               ;default Win95 transient stack size
SLIDEMAX  equ       4096                ;maximum paragraph size to copy in
                                        ;  one instruction for slider
EMSMPAG   equ       4                   ;maximum EMS pages we can transfer 
                                        ;  through EMS page frame
EMSMPAR   equ       (EMSMPAG shl 10)    ;maximum paragraphs we can transfer 
                                        ;  through EMS page frame
DISKXMAX  equ       4064                ;maximum paragraphs we can transfer 
                                        ;  to disk (4064 paragraphs = 65024
                                        ;  bytes = 127 sectors)
SUMMAX    equ       4096                ;maximum paragraphs we can sum 
                                        ;  without bumping segment register
EXECACT   equ       4Dh                 ;signature for EXEC active
CSOFFSET  equ       1200h               ;maximum offset of CONFIG.SYS text
                                        ;  from top of memory, in paragraphs
CCMSIG    equ       122Eh               ;AX signature for COMMAND.COM msg
                                        ;  handler init call via INT 2F
          ;
          ; Scan lengths when looking for SHELL= line in memory.  These
          ; lengths must be at least 80 bytes smaller than 64K so that
          ; if the beginning of a path is found at the end of the scan
          ; it can still be processed without segment wrap.
          ;
SCANOFFP  equ       5                   ;scan offset in paragraphs = 80 bytes
SCANOFFB  equ       (SCANOFFP shl 4)    ;scan offset in bytes
SCANMAXP  equ       1000h - SCANOFFP    ;scan max in paragraphs (64K less
                                        ;  offset)
SCANMAXB  equ       (SCANMAXP shl 4)    ;scan max in bytes
          ;
          ;
          ;
          ; Windows Modes
          ;
WINNONE   equ       0                   ;no Windows
WIN2      equ       1                   ;Windows 2 running
WIN386    equ       2                   ;Windows 3 in enhanced mode
WINSTD    equ       3                   ;Windows 3 in real or standard mode
WINOS2    equ       20                  ;Windows 3 simulated by OS/2 2.0
WINNT     equ       30                  ;Windows NT
WIN95     equ       40                  ;Windows 95
WIN98     equ       41                  ;Windows 98
WINME     equ       42                  ;Windows ME (Millennium)
          ;
          ; Shift / add count values for CHECKSUM.ASM -- to go into CX
          ; register when CheckSum routine is called.
          ;
SUMALL    equ       0300h               ;shift 3 bits, add 0 to byte pointer,
                                        ;  sums every word
SUM2      equ       0202h               ;shift 2 bits, add 2 to byte pointer,
                                        ;  sums every other word
SUM4      equ       0106h               ;shift 1 bit, add 6 to byte pointer,
                                        ;  sums every fourth word
SUM8      equ       000Eh               ;shift 0 bits, add 14 to byte
                                        ;  pointer, sums every eigth word
          ;
          ;
          ;         bits in LMFlags and SrvFlags
          ;
ROOTFLAG  equ       80h                 ;we are the root shell
NOCHAIN   equ       40h                 ;server interrupt chaining disabled
INSHELL   equ       20h                 ;shell is running
IGNORE23  equ       10h                 ;ignore INT 23
DOS3      equ       08h                 ;running DOS 3 or above
SWAPENAB  equ       04h                 ;swapping is enabled
BRKOCCUR  equ       02h                 ;^C during external program
FATALERR  equ       01h                 ;fatal error occurred in loader
          ;
          ;         bits in LMFlag2 and SrvFlag2
          ;
AUTOFAIL  equ       80h                 ;automatic fail on INT 23
DREOPEN   equ       40h                 ;disk swap file should be closed and
                                        ;  reopened
SWAPPED   equ       20h                 ;4DOS is swapped, can't use error
                                        ;  handler
NOCCMSG   equ       10h                 ;disable COMMAND.COM message handler
NORELOC   equ       08h                 ;don't do relocation as code segment
                                        ;  was not swapped in (only used by
                                        ;  server)
INDV      equ       04h                 ;running under DESQView
DVCLEAN   equ       02h                 ;DESQView cleanup enabled
RESTITLE  equ       01h                 ;OS/2 title needs to be restored
          ;
          ;         bits in InitFlag
          ;
PERMLOAD  equ       80h                 ;permanent load
TRANLOAD  equ       40h                 ;transient load
CSPSET    equ       20h                 ;COMSPEC path was set on cmd line
NOPREV    equ       10h                 ;no previous shell to talk to
EMSAVAIL  equ       08h                 ;EMS available
XMSAVAIL  equ       04h                 ;XMS available
XINHERIT  equ       02h                 ;disable alias / func / history inheritance 
DBMSG     equ       01h                 ;debug mesages on
          ;
          ;         bits in InitFlg2
          ;
INIDONE   equ       80h                 ;INI file has been processed
NOAUTOEX  equ       40h                 ;disable AUTOEXEC
W95DSWP	equ	20h		;Win95 killed disk swap inheritance
W95USWP	equ	10h       	;Win95 force unique swap name
W95DESK	equ	08h       	;started from Win95 desktop
          ;
          ;         bits in Flags  (CAUTION:  All these bits except NOREDUCE
          ;         get forced to zero in resident mode, so watch what you
          ;         put here!)
          ;
TRANSHI   equ       80h                 ;transient portion loaded high
DISKOPEN  equ       40h                 ;disk swap file is open
NOREDUCE  equ       20h                 ;don't use previous shell's code to
                                        ;  reduce swap size
TPARES    equ       10h                 ;reserve transient program area
LOADUMB   equ       08h                 ;resident portion in UMB
MENVUMB   equ       04h                 ;master environment in UMB
          ;
          ;         bits in IntFlags
          ;
INTHOOK   equ       80h                 ;INT 2F is hooked
I23HOOK   equ       40h                 ;INT 23 is hooked
I24HOOK   equ       20h                 ;INT 24 is hooked
I2EHOOK   equ       10h                 ;INT 2E is hooked
          ;
          ;         bits in DSwpFlag in ShellDat data structure
          ;
LALIAS    equ       80h                 ;aliases are local
LHISTORY  equ       40h                 ;history is local
LFUNC     equ       20h			;functions are local
          ;
          ;         bits in SwapFlag byte in region swap control blocks
          ;
CHKCALC   equ       80h                 ;checksum calculation enabled
SKIPTEST  equ       40h                 ;skip disk signature test (used for
                                        ;  aliases / functions / history)
SEGABS    equ       20h                 ;main memory segment is absolute
RLDISABL  equ       10h                 ;disable relocation if checksum on
                                        ;  this region matches -- used to
                                        ;  prevent duplicate relocation
                                        ;  when checksum matches and swap in
                                        ;  from disk is skipped
NEWRBAS   equ       08h                 ;new relocation base -- set for any
                                        ;  region used to swap out the code
                                        ;  segment
          ;
          ;         bits in ModFlags byte in ModEntry structure (loadable
          ;         module table)
          ;
LOADME    equ       80h                 ;module should be loaded
          ;
          ;         bits in Debug byte in INI file data
          ;
ID_TAIL   equ       01h                 ;display tail
ID_ELAB   equ       02h                 ;label errors
          ;
          ;
          ;         values for SwMethod
          ;
SWAPEMS   equ       0                   ;load high, swap to EMS
SWAPXMS   equ       1                   ;load high, swap to XMS
SWAPDISK  equ       2                   ;load high, swap to disk
SWAPNONE  equ       3                   ;no swapping
          ;
          ;
          ;         values for AlsType / HistType
          ;
AHLOCAL   equ       0                   ;reserve local block
AHGLNEW   equ       1                   ;reserve global block
AHGLOLD   equ       2                   ;use previous global block
          ;
          ;
          ; Error numbers
          ;
          ;         Internal errors detected by resident part of loader.
          ;         These are characters as they are displayed directly
          ;         by the error handler in 4DOS.ASM, there is no table
          ;         lookup.  
          ;
E_BADFC   equ       'FB'                ;BF - bad server function code
E_SHNEW   equ       'SN'                ;NS - no number for new shell
E_SHEXIT  equ       'ST'                ;TS - terminated inactive shell
E_PRTERM  equ       'TP'                ;PT - illegal process termination
E_XMSIN   equ       'IX'                ;XI - XMS swap in error
E_EMSMPI  equ       'ME'                ;EI - EMS map for swap in error
E_DSKINT  equ       'ID'                ;DI - disk swap in integrity error
E_DSKSK   equ       'SD'                ;DS - disk swap in seek error
E_DSKRD   equ       'RD'                ;DR - disk swap in read error
E_DSKRO   equ       'OD'                ;DO - disk swap reopen error
          ;
          ;         Internal errors detected by server; some of these
          ;         codes are also used by 4DLINIT.  The ones that are
          ;         are marked with an *.
          ;
E_TFREE   equ       01                  ;can't free transient memory when
                                        ;  starting an application
E_TALLOC  equ       02                  ;can't reserve high memory 
E_QROOT   equ       03                  ;attempt to exit from root shell
E_DAEMS   equ       04                  ;can't deallocate EMS on exit
E_DAXMS   equ       05                  ;can't deallocate XMS on exit
E_XMSMOV  equ       06                  ;can't move to/from XMS
E_EMSMAP  equ       07                  ;can't map pages for swap
E_EMSMSR  equ       08                  ;can't save or restore map state
E_SWSEEK  equ       09                  ;* can't seek swap file to swap in/out
E_SWWRIT  equ       10                  ;* error swapping out to disk
E_SWREAD  equ       11                  ;error swapping in from disk
E_SWDBAD  equ       12                  ;swap file corrupted
E_NOHDL   equ       13                  ;no file handle available
E_SMATCH  equ       14                  ;server COM / EXE version mismatch
          ;
          ;         Internal errors detected by loader initialization code
          ;
E_TRANS   equ       21                  ;first error in transient error list
E_DSIZE   equ       21                  ;can't reduce data segment size
E_MEMDA   equ       22                  ;can't reduce or deallocate memory
E_NOMEM   equ       23                  ;no memory to load
E_MODFIT  equ       24                  ;loadable modules don't fit
E_RELOC   equ       25                  ;can't find relocation table
E_RLSIZE  equ       26                  ;relocation table too big

