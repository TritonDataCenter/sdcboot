

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


.LIST
	;
	; MASM macros by Tom Rawson, J.P. Software
	;
	; Copyright 1988, 1989, 1990, 1991, J.P. Software.
	; All Rights Reserved.
	;
	; (Contents of macro file not listed)
	;
.XLIST
	;
	; For Microsoft MASM 5.0 and above.
	;
	;
	; Set up debug, memory model, and O/S equates
	;
	;
	ifndef	DEBUG
DEBUG	=	0
	endif
	;
	ife	_FLAT		;16-bit code definitions
xax	equ	ax
xbx	equ	bx
xcx	equ	cx
jxcxz	equ	jcxz
xdx	equ	dx
xsi	equ	si
xdi	equ	di
xbp	equ	bp
xsp	equ	sp
_regsize	=	2		;;bytes in a PUSHed register
_nearsz	=	2		;;bytes in a near address
_farsz	=	4		;;bytes in a far address
	else			;32-bit code definitions
xax	equ	eax
xbx	equ	ebx
xcx	equ	ecx
jxcxz	equ	jecxz
xdx	equ	edx
xsi	equ	esi
xdi	equ	edi
xbp	equ	ebp
xsp	equ	esp
_regsize	=	4		;;bytes in a PUSHed register
_nearsz	=	4		;;bytes in a near address
_farsz	=	6		;;bytes in a far address
	endif
	;
	;
	; Constants used with these macros or externally
	;
	;
	;    ASCII Characters
	;
BELL	equ	07h		;beep
BS	equ	08h		;backspace
TAB	equ	09h		;tab
LF	equ	0Ah		;line feed
CR	equ	0Dh		;carriage return
EOF	equ	1Ah		;EOF in ASCII files
	;
	;
	;    IBM PC interrupt numbers
	;
I_DIV0	equ	000h		;divide by zero
I_SSTEP	equ	001h		;single step
I_NMI	equ	002h		;non-maskable
I_BRKPNT	equ	003h		;breakpoint
I_OVF	equ	004h		;arithmetic overflow
I_PRTSC	equ	005h		;print screen
I_CLOCK	equ	008h		;hardware clock tick
I_KEYBD	equ	009h		;keyboard
I_COM0	equ	00Bh		;communications controller port 0
I_COM1	equ	00Ch		;communications controller port 1
I_RETRC	equ	00Dh		;video retrace
I_FDC	equ	00Eh		;floppy disk controller
I_PRINT	equ	00Fh		;printer port
I_BVIDEO	equ	010h		;BIOS video services
I_BEQUIP	equ	011h		;BIOS equipment list
I_BMEM	equ	012h		;BIOS memory size
I_BDISK	equ	013h		;BIOS disk services
I_BCOM	equ	014h		;BIOS communications
I_BXMEM	equ	015h		;BIOS extended memory access
I_BKEYBD	equ	016h		;BIOS keyboard services
I_BPRINT	equ	017h		;BIOS printer services
I_BBASIC	equ	018h		;BIOS ROM BASIC start
I_BBOOT	equ	019h		;BIOS reboot
I_BDATIM	equ	01Ah		;BIOS date and time
I_BBREAK	equ	01Bh		;BIOS ctrl-break
I_BTICK	equ	01Ch		;BIOS clock tick
I_BVTAB	equ	01Dh		;BIOS video tables
I_BDSKTB	equ	01Eh		;BIOS disk tables
I_BVGRPH	equ	01Fh		;BIOS video graphics tables
I_TERM	equ	020h		;DOS terminate
I_DOS	equ	021h		;DOS function
I_NOVELL	equ	I_DOS		;Netware uses DOS interrupt
I_TRMADR	equ	022h		;terminate address
I_CTRLC	equ	023h		;control-C / control-Break handler
I_CRITER	equ	024h		;critical error handler
I_ABSRD	equ	025h		;absolute disk read
I_ABSWRT	equ	026h		;absolute disk write
I_TSR	equ	027h		;terminate and stay resident
I_MPX	equ	02Fh		;multiplex
I_WIN	equ	I_MPX		;Windows interface = MPX
I_XMS	equ	I_MPX		;XMS interrupt = MPX
I_EMS	equ	067h		;EMS driver
	;
	;
	;    EMS 3.2 function codes and parameters
	;
EM_STAT	equ	040h		;get EMS status
EM_PFAD	equ	041h		;get page frame address
EM_UPCNT	equ	042h		;get unallocated page count
EM_ALLOC	equ	043h		;allocate pages
EM_MAP	equ	044h		;map pages
EM_DALOC	equ	045h		;deallocate pages
EM_VER	equ	046h		;get software version
EM_SMAP	equ	047h		;save page map
EM_RMAP	equ	048h		;restore page map
EM_HCNT	equ	04Bh		;get handle count
EM_HPCNT	equ	04Ch		;get handle page count
EM_HPAGE	equ	04Dh		;get complete handle page map
EM_PGMAP	equ	04Eh		;get / set hardware page map
EM_HNAME	equ	053h		;get / set handle name
	;
EM_PMGET	equ	000h		;PGMAP function -- get page map
EM_PMSET	equ	001h		;PGMAP function -- set page map
EM_PMGS	equ	002h		;PGMAP function -- get & set page map
EM_PMSIZ	equ	003h		;PGMAP function -- get page map size
	;
	;
	;    XMS parameters and function codes
	;
XMovData	struc			;XMS move data structure
MoveLen	dd	?		;move length
SrcHdl	dw	?		;source handle
SrcOff	dw	?		;source offset or high offset
SrcSeg	dw	?		;source segment or low offset
DestHdl	dw	?		;destination handle
DestOff	dw	?		;destination offset or high offset
DestSeg	dw	?		;destination segment or low offset
XMovData	ends
	;
XMovPtrs	struc			;template for pointers portion of
				;  XMovData (above)
XHdl	dw	?		;handle
XOff	dw	?		;memory offset or XMS high offset
XSeg	dw	?		;memory segment or XMS low offset
XMovPtrs	ends
	;
XMSTEST	equ	4300h		;value for XMS test call to INT 2F
XMSFLAG	equ	80h		;returned value from test call
XMSADDR	equ	4310h		;value for XMS address call to INT 2F
	;
XM_GVER	equ	00h		;Get XMS Version Number
XM_REQHM	equ	01h		;Request High Memory Area
XM_RELHM	equ	02h		;Release High Memory Area
XM_GEA20	equ	03h		;Global Enable A20
XM_GDA20	equ	04h		;Global Disable A20
XM_LEA20	equ	05h		;Local Enable A20
XM_LDA20	equ	06h		;Local Disable A20
XM_QA20	equ	07h		;Query A20
XM_FREE	equ	08h		;Query Free Extended Memory
XM_ALLOC	equ	09h		;Allocate Extended Memory Block
XM_DALOC	equ	0Ah		;Free Extended Memory Block
XM_MOVE	equ	0Bh		;Move Extended Memory Block
XM_LOCK	equ	0Ch		;Lock Extended Memory Block
XM_UNLK	equ	0Dh		;Unlock Extended Memory Block
XM_GTHDL	equ	0Eh		;Get Handle Information
XM_RALOC	equ	0Fh		;Reallocate Extended Memory Block
XM_REQUM	equ	10h		;Request Upper Memory Block
XM_RELUM	equ	11h		;Release Upper Memory Block
	;
	;
	;    Windows 3 Interface
	;
W_CHK386	equ	1600h		;INT 2F code to detect 386 mode
W_CHK98	equ	160Ah		;INT 2F code to detect Win98
W_CHKSTD	equ	4680h		;INT 2F code to detect real / std
	;
	;
	;    Novell Netware Interface
	;
N_SHVER	equ	0EA01h		;AX for shell version function call
N_RBLEN	equ	40		;bytes in shell version call reply
				;  buffer
	;
	;
	;    DOS Interface
	;
	;	File I/O constants
	;
PATHMAX	equ	64		;maximum length of a path name
FILEMAX	equ	12		;maximum length of a file name
PATHBLEN	equ	80		;length of a path buffer
	;
FREAD	equ	0		;open file read mode
FWRITE	equ	1		;open file write mode
FRDWRT	equ	2		;open file read/write mode
	;
SEEK_BEG	equ	0		;seek starts from beginning of file
SEEK_CUR	equ	1		;seek starts from current position
SEEK_END	equ	2		;seek starts from end of file
	;
ATR_READ	equ	01h		;read-only attribute
ATR_HIDE	equ	02h		;hidden attribute
ATR_SYS	equ	04h		;system attribute
ATR_VOL	equ	08h		;volume label attribute
ATR_DIR	equ	10h		;directory attribute
ATR_ARC	equ	20h		;archive attribute
ATR_ALL	equ	ATR_READ + ATR_HIDE + ATR_SYS + ATR_DIR  ;all files/dirs
	;
SHR_INH	equ	80h		;inheritance bit
SHR_COMP	equ	00h		;compatibility mode
SHR_DRW	equ	10h		;deny read/write
SHR_DWRT	equ	20h		;deny write
SHR_DRD	equ	30h		;deny read
SHR_DNON	equ	40h		;deny none
ACC_READ	equ	00h		;read access
ACC_WRIT	equ	01h		;write access
ACC_RDWR	equ	02h		;read/write access
	;
	; Bits in DX returned by DOS version call with AX=3306h under
	; DOS 5 or above.
	;
DOS_REV	equ	07h		;revision level (DL)
OS2_REV	equ	7FFh		;OS/2 revision level (DX)
DOS_HMA	equ	10h		;DOS in HMA (DH)
DOS_ROM	equ	08h		;DOS is in ROM (DH)
	;
	;
	;	Standard DOS file handles
	;
STDIN	equ	0		;standard input
STDOUT	equ	1		;standard output
STDERR	equ	2		;standard error
STDAUX	equ	3		;standard auxiliary (COM port)
STDPRN	equ	4		;standard printer
	;
	;	Memory allocation parameters (INT 21 function 58h)
	;
GETSTRAT	equ	0		;get strategy subfunction
SETSTRAT	equ	1		;set strategy subfunction
GETULINK	equ	2		;get UMB link status subfunction
SETULINK	equ	3		;set UMB link status subfunction
	;
LFIRST	equ	0		;low first strategy
HFIRST	equ	80h		;high first strategy
FIRSTFIT	equ	0		;first fit strategy
BESTFIT	equ	1		;last fit strategy
LASTFIT	equ	2		;best fit strategy
	;
ULINKOFF	equ	0		;disables UMB links
ULINKON	equ	1		;enables UMB links
	;
	;
	;	DR-DOS Function codes
	;
DRCHECK	equ	4452h		;function code for DR-DOS test
DRLINK	equ	4457h		;function code for UMB link / unlink
DRSTART	equ	4458h		;get startup data structure address
DRLNKOFF	equ	200h		;subfunction for UMB unlink; link
				;  is 1 higher (201h)
	;
	;
	;	DOS function numbers
	;
D_QUIT	equ	000h		;terminate
D_KEYECHO equ	001h		;read keyboard with echo
D_DISPCHR equ	002h		;display character
D_PRINTCH equ	005h		;print character
D_CONSIO	equ	006h		;console i/o
D_CONSIN	equ	007h		;console input
D_KEYREAD equ	008h		;read keyboard
D_MESSAGE equ	009h		;display string
D_KEYIN	equ	00Ah		;buffered keyboard input
D_KEYSTAT equ	00Bh		;check keyboard status
D_KFLUSH	equ	00Ch		;flush buffer and read keyboard
D_DFLUSH	equ	00Dh		;flush disk buffers
D_SETDRV	equ	00Eh		;set current default drive
D_GETDRV	equ	019h		;get current default drive
D_SETDTA	equ	01Ah		;set DTA
D_DALLOC	equ	01Bh		;get default drive allocation info
D_SALLOC	equ	01Ch		;get specified drive allocation info
D_SETINT	equ	025h		;set interrupt vector
D_MAKEPSP equ	026h		;create PSP
D_PARSE	equ	029h		;parse file name for FCB
D_GETDATE equ	02Ah		;get date
D_SETDATE equ	02Bh		;set date
D_GETTIME equ	02Ch		;get time
D_SETTIME equ	02Dh		;set time
D_VERSION equ	030h		;get version number
D_TSR	equ	031h		;terminate and stay resident
D_CTRLC	equ	033h		;Ctrl-C check
D_GETINT	equ	035h		;get interrupt vector
D_DSKFREE equ	036h		;get disk free space
D_SWCHAR	equ	037h		;get / set switch character
D_MAKEDIR equ	039h		;make subdirectory
D_REMDIR	equ	03Ah		;remove subdirectory
D_CHDIR	equ	03Bh		;change subdirectory
D_CREATE	equ	03Ch		;create file
D_OPEN	equ	03Dh		;open file
D_CLOSE	equ	03Eh		;close file
D_READ	equ	03Fh		;read from file
D_WRITE	equ	040h		;write from file
D_DELETE	equ	041h		;delete pathname
D_SEEK	equ	042h		;move file pointer
D_ATTRIB	equ	043h		;get/set file attributes
D_ATTGET	equ	04300h		;get file attributes
D_ATTSET	equ	04301h		;set file attributes
D_IOCTL	equ	044h		;I/O control
D_DUPHDL	equ	045h		;duplicate file handle
D_FORCDUP equ	046h		;force duplicate file handle
D_GETDIR	equ	047h		;get current directory
D_ALLOC	equ	048h		;allocate memory block
D_FREE	equ	049h		;free memory block
D_REALLOC equ	04Ah		;change size of memory block
D_EXEC	equ	04Bh		;load and execute program/overlay
D_TERM	equ	04Ch		;terminate process
D_GETRET	equ	04Dh		;get child process return code
D_FFIRST	equ	04Eh		;find first file match
D_FNEXT	equ	04Fh		;find next file match
D_SETPSP	equ	050h		;set current PSP
D_GETPSP	equ	051h		;get current PSP
D_LOL	equ	052h		;get list of lists address
D_RENAME	equ	056h		;rename file
D_FDATE	equ	057h		;get/set file date/time
D_SETMETH equ	058h		;set memory allocation method
D_GETERR	equ	059h		;get extended error code
D_UNIQUE	equ	05Ah		;open file with unique name
D_CREATE2 equ	05Bh		;create new file
D_RLOCK	equ	05Ch		;record lock and unlock
D_SLEEP	equ	089h		;process sleep
	;
	;
	; Windows 95 extended DOS API calls
	;
D_95_GLA	equ	5704h		;get last access date/time
D_95_SLA	equ	5705h		;set last access date/time
D_95_GCD	equ	5706h		;get creation date/time
D_95_SCD	equ	5707h		;set creation date/time
D_95_MD	equ	7139h		;make subdirectory
D_95_RD	equ	713Ah		;remove subdirectory
D_95_CD	equ	713Bh		;change subdirectory
D_95_DEL	equ	7141h		;delete pathname
D_95_ATT	equ	7143h		;get/set file attributes
D_95_GD	equ	7147h		;get current directory
D_95_FND	equ	714Eh		;find first file match
D_95_FNX	equ	714Fh		;find next file match
D_95_REN	equ	7156h		;rename file
D_95_PTH	equ	7160h		;get path name
D_95_OPN	equ	716Ch		;create or open file
D_95_VOL	equ	71A0h		;get volume info
D_95_FCL	equ	71A1h		;find close
D_95_INF	equ	71A6h		;get file information
D_95_TIM	equ	71A7h		;time conversion
	;
	;
	; Useful data structures
	;
farptr	struc			;far pointer
foff	dw	?		;offset
fseg	dw	?		;segment
farptr	ends
	;
	;
path	struc			;full file path
	db	PATHBLEN dup (?)	;room for maximum path,
				;  including terminating null byte
path	ends
	;
	;
EXECPARM	struc			;dos func 4B (exec) parameter block
EXEC_ENV	dw	?		;environment segment address
EXEC_TOF	dw	?		;command tail offset
EXEC_TSG	dw	?		;command tail segment
EXEC_F1O	dw	?		;FCB 1 offset
EXEC_F1S	dw	?		;FCB 1 segment
EXEC_F2O	dw	?		;FCB 2 offset
EXEC_F2S	dw	?		;FCB 2 segment
EXECPARM	ends
	;
	;
FCB	struc			;DOS file control block
FCB_DRIV	db	?		;drive ID
FCB_NAME	db	8 dup (?) 	;file name
FCB_EXT	db	3 dup (?) 	;extension
FCB_BNUM	dw	?		;current block number
FCB_RSIZ	dw	?		;record size
FCB_FSIZ	dd	?		;file size
FCB_DATE	dw	?		;modification date
FCB_TIME	dw	?		;modification time
FCB_RES	db	8 dup (?) 	;reserved area
FCB_CREC	db	?		;current record number
FCB_RREC	dd	?		;random record number
FCB	ends
	;
	;
DOSFIND	struc			;DOS find first data structure
				;  (items marked with * are
				;   undocumented fields)
DF_DRIVE	db	?		;* drive for search (1=A etc.)
DF_SNAME	db	11 dup (?)	;* search name and extension
DF_SATTR	db	?		;* search attribute
DF_DENUM	dw	?		;* directory entry number
DF_DCLUS	dw	?		;* first cluster of directory
	db	4 dup (?) 	;* reserved
DF_ATTR	db	?		;file attribute
DF_TIME	dw	?		;file time
DF_DATE	dw	?		;file date
DF_SIZE	dd	?		;file size in bytes
DF_NAME	db	13 dup (?)	;file name, null-terminated
DOSFIND	ends
	;
DOSFINDX	struc			;Extended DOS find first data (Win95)
				;  (items marked with * are
				;   undocumented fields)
DX_DRIVE	db	?		;* drive for search (1=A etc.)
DX_SNAME	db	11 dup (?)	;* search name and extension
DX_SATTR	db	?		;* search attribute
DX_DENUM	dw	?		;* directory entry number
DX_DCLUS	dw	?		;* first cluster of directory
	db	4 dup (?) 	;* reserved
DX_ATTR	db	?		;file attribute
DX_TIME	dw	?		;file time
DX_DATE	dw	?		;file date
DX_SIZE	dd	?		;file size in bytes
DX_NAME	db	256 dup (?)	;file name, null-terminated
DX_ANAME	db	14 dup (?)	;alternate file name, null-terminated
DX_CRTIM	dd	2 dup (?) 	;file creation name
DX_ACTIM	dd	2 dup (?) 	;file last access name
DX_WRTIM	dd	2 dup (?) 	;file last write name
DOSFINDX	ends
	;
DIRENTRY	struc			;DOS directory entry
DE_NAME	db	8 dup (?) 	;name
DE_EXT	db	3 dup (?) 	;extension
DE_ATTR	db	?		;file attribute
DE_RES	db	10 dup (?)	;reserved
DE_TIME	dw	?		;time last modified
DE_DATE	dw	?		;date last modified
DE_CLUST	dw	?		;starting cluster
DE_SIZE	dd	?		;file size
DIRENTRY	ends
	;
CTRYINFO	struc			;country info
	if	_DOS		;first two fields in DOS / WIN only
CY_ID	db	?		;country ID
CY_LEN	dw	?		;length of structure
	endif
	if	(_4OS2 eq 32) or _TCMDOS2
				;32-bit OS/2 has dword country and CP
CY_CTRY	dd	?		;country code
CY_CPAGE	dd	?		;code page
	else			;all but 32-bit OS/2 has dwords
CY_CTRY	dw	?		;country code
CY_CPAGE	dw	?		;code page
	endif
	ife	_FLAT
CY_DATE	dw	?		;date format, 0=US, 1=Europe, 2=Japan
	else
CY_DATE	dd	?		;date format, 0=US, 1=Europe, 2=Japan
	endif
CY_CURR	db	5 dup (?) 	;ASCIIZ currency symbol string
CY_TSEP	db	2 dup (?) 	;ASCIIZ thousands separator
CY_DEC	db	2 dup (?) 	;ASCIIZ decimal separator
CY_DSEP	db	2 dup (?) 	;ASCIIZ date separator
CY_MSEP	db	2 dup (?) 	;ASCIIZ time separator
CY_CUFMT	db	?		;currency format
CY_DPLAC	db	?		;decimal places in currency
CY_TFMT	db	?		;time format, 0=12 hour, 1=24 hour
CY_CFUNC	dd	?		;case map function pointer
CY_LSEP	db	2 dup (?) 	;ASCIIZ data list separator
CY_RESV	dw	5 dup (?) 	;reserved2
CTRYINFO	ends
	;
	;
FHTSIZE	equ	20		;process file handle table size
	;
	;
PSP	struc			;DOS program segment prefix (PSP)
PSP_QUIT	db	2 dup (?) 	;(00-01) terminate instruction
PSP_MEND	dw	?		;(02-03) memory end segment address
	db	2 dup (?) 	;(04-05) reserved
PSP_FREE	dw	?		;(06-07) bytes free in this block
	db	2 dup (?) 	;(08-09) reserved
PSP_TERM	dw	2 dup (?) 	;(0A-0D) terminate address
PSP_BRK	dw	2 dup (?) 	;(0E-11) ctrl-break address
PSP_CRIT	dw	2 dup (?) 	;(12-15) critical error address
PSP_CHN	dw	?		;(16-17) PSP chain pointer
PSP_FTAB	db	FHTSIZE dup (?)	;(18-2B) process file handle table
PSP_ENV	dw	?		;(2C-2D) environment segment
	db	6 dup (?) 	;(2E-33) reserved
PSP_FPTR	dd	?		;(34-37) job file table pointer
	db	18h dup (?)	;(38-4F) reserved
PSP_DOS	db	2 dup (?) 	;(50-51) DOS call
	db	0Ah dup (?)	;(52-5B) reserved
PSP_FCB1	db	10h dup (?)	;(5C-6B) default FCB 1
PSP_FCB2	db	14h dup (?)	;(6C-7F) default FCB 1
PSP_TLEN	db	?		;(80)    command tail length
PSP_TAIL	db	7Eh dup (?)	;(81-FE) command tail / DTA
PSP_LAST	db	?		;(FF)    last byte in tail and in PSP
PSP	ends
	;
PSP_END	equ	PSP_LAST + 1	;end of PSP
PSP_LEN	equ	PSP_END		;length = offset of end
PSP_LENW	equ	PSP_LEN shr 1	;word length = length / 2
PSP_LENP	equ	PSP_LEN shr 4	;paragraph length = length / 4
TAIL_MAX	equ	PSP_END - PSP_TAIL	;max length of tail
	;
	;
MCB	struc			;DOS memory control block
MCB_FLAG	db	?		;flag byte ("M"=more, "Z"=last)
MCB_OWN	dw	?		;owner's PSP
MCB_LEN	dw	?		;length in paragraphs
MCB_RES	db	3 dup (?) 	;3 bytes reserved
MCB_NAME	db	8 dup (?) 	;8 bytes of "name" in some versions
MCB_FWD	dw	?		;forward pointer used by UMBReg
MCB_BACK	dw	?		;back pointer used by UMBReg
MCB	ends
	;
	;
	; Instruction enhancements / conveniences
	;
	;    Pointers
	;
bptr	equ	<byte ptr>	;;simple byte pointer
wptr	equ	<word ptr>	;;simple word pointer
dptr	equ	<dword ptr>	;;simple dword pointer
	;
PCHAR	typedef	PTR BYTE		;;pointer to char
PWORD	typedef	PTR WORD		;;pointer to word
PDWORD	typedef	PTR DWORD		;;pointer to dword
	;
	;
	;    Stack operations
	;
pushc	macro	const		;;push a constant
	mov	ax,const		;;load constant
	push	ax		;;push it
	endm
	;
pushadr	macro	loc		;;push the address of something
	pushc	<offset loc>	;;push offset as a constant
	endm
	;
	;
	ifndef	OPTASM		;if not OPTASM, set up pushm/popm
	;
pushm	macro	r1,r2,r3,r4,r5,r6,r7,r8,r9
	irp	reg,<r1,r2,r3,r4,r5,r6,r7,r8,r9>  ;;repeat for all reg
	ifnb	<reg>		;;this argument specified?
	push	reg		;;yes, push it
	endif
	endm			;;end irp
	endm			;;end macro
	;
popm	macro	r1,r2,r3,r4,r5,r6,r7,r8,r9
	irp	reg,<r1,r2,r3,r4,r5,r6,r7,r8,r9>  ;;repeat for all reg
	ifnb	<reg>		;;this argument specified?
	pop	reg		;;yes, pop it
	endif
	endm			;;end irp
	endm			;;end macro
	;
	endif			;end not OPTASM check
	;
	;    Bit set / clear
	;
bset	macro	dest,mask 	;;set a bit
	or	dest,(mask)	;;just or it in
	endm
	;
bclr	macro	dest,mask 	;;clear a bit
	and	dest,(not (mask))	;;and with inverse
	endm
	;
	;
	;    Shifts and rotates
	;
shln	macro	reg,count,sreg	;;shift reg left n bits
	if	count		;;if count is zero do nothing
	ifb	<sreg>		;;if no shift register do repeats
	rept	count		;;repeat for count
	shl	reg,1		;;shift by 1
	endm			;;end repeat loop
	else			;;shift register specified
	mov	sreg,count	;;get count into register
	shl	reg,sreg		;;and do the shift
	endif			;;shift register test
	endif			;;zero count test
	endm
	;
shrn	macro	reg,count,sreg	;;shift reg right n bits
	if	count		;;if count is zero do nothing
	ifb	<sreg>		;;if no shift register do repeats
	rept	count		;;repeat for count
	shr	reg,1		;;shift by 1
	endm			;;end repeat loop
	else			;;shift register specified
	mov	sreg,count	;;get count into register
	shr	reg,sreg		;;and do the shift
	endif			;;shift register test
	endif			;;zero count test
	endm
	;
roln	macro	reg,count,sreg	;;rotate reg left n bits
	if	count		;;if count is zero do nothing
	ifb	<sreg>		;;if no shift register do repeats
	rept	count		;;repeat for count
	rol	reg,1		;;rotate by 1
	endm			;;end repeat loop
	else			;;shift register specified
	mov	sreg,count	;;get count into register
	rol	reg,sreg		;;and do the rotate
	endif			;;shift register test
	endif			;;zero count test
	endm
	;
rorn	macro	reg,count,sreg	;;rotate reg right n bits
	if	count		;;if count is zero do nothing
	ifb	<sreg>		;;if no shift register do repeats
	rept	count		;;repeat for count
	ror	reg,1		;;rotate by 1
	endm			;;end repeat loop
	else			;;shift register specified
	mov	sreg,count	;;get count into register
	ror	reg,sreg		;;and do the rotate
	endif			;;shift register test
	endif			;;zero count test
	endm
	;
dshl	macro	loreg,hireg	;;double reg shift left 1 bit
	shl	loreg,1		;;shift low register
	rcl	hireg,1		;;rotate new bit into high register
	endm
	;
dshr	macro	loreg,hireg	;;double reg shift right 1 bit
	shr	hireg,1		;;shift high register
	rcr	loreg,1		;;rotate new bit into low register
	endm
	;
dshln	macro	loreg,hireg,count	;;double reg shift left n bits
	if	count		;;if count is zero do nothing
	rept	count		;;repeat for count
	dshl	loreg,hireg	;;shift by 1
	endm			;;end repeat loop
	endif
	endm
	;
dshrn	macro	loreg,hireg,count	;;double reg shift right n bits
	if	count		;;if count is zero do nothing
	rept	count		;;repeat for count
	dshr	loreg,hireg	;;shift by 1
	endm			;;end repeat loop
	endif
	endm
	;
	;
	;    Double-word load / store
	;
dload	macro	loreg,hireg,source	;;double reg load from memory
	mov	loreg,wptr source	;;load low
	mov	hireg,wptr source+2 ;;load high
	endm
	;
dstore	macro	dest,loreg,hireg	;;double reg store to memory
	mov	wptr dest,loreg	;;store low
	mov	wptr dest+2,hireg	;;store high
	endm
	;
	;
	;    Segment and segment register operations
	;
	;
.cseg	macro	csprefix		;;set code segment prefix if _CSEG
				;;  is undefined
	ifndef	_CSEG		;;any definition yet?
% _CSEG	equ	<csprefix>	;;if not, make one
	endif
	endm
	;
				;;define a segment with default name
segdef	macro	userseg,defseg,align,combine,class
	ifdef	userseg		;;if userseg is defined use that
userseg	segment	align combine class
	else			;;otherwise use default
defseg	segment	align combine class
	endif			;;userseg defined
	endm
	;
				;;define a group with default name
groupdef	macro	usergrp,defgrp,segname
	ifdef	usergrp		;;if usergrp is defined use that
usergrp	group	segname
	else			;;otherwise use default
defgrp	group	segname
	endif			;;usergrp defined
	endm
	;
	;
loadseg	macro	segreg,src,reg	;;load segment register
	ifnb	<reg>		;;if register specified use it
	mov	reg,src		;;get value
	mov	segreg,reg	;;put in segment register
	else			;;no register, use stack
	push	src		;;put value on stack
	pop	segreg		;;and pop it into segment reg
	endif
	endm
	;
	;
	;    String operations
	;
stocb	macro	bval		;;store a byte into a string
	mov	al,bval		;;load byte value
	stosb			;;store it
	endm
	;
stocw	macro	wval		;;store a word into a string
	mov	ax,wval		;;load word value
	stosw			;;store it
	endm
	;
moveb	macro	sseg,soff,dseg,doff,cnt,dir  ;;do a byte move
	ifnb	<soff>		;;check source offset
	mov	si,soff		;;if non-blank use it
	endif
	ifnb	<doff>		;;check destination offset
	mov	di,doff		;;if non-blank use it
	endif
	ifnb	<cnt>		;;check count
	mov	cx,cnt		;;if non-blank use it
	endif
	ifnb	<dseg>		;;check destination segment
	loadseg	es,dseg		;;if non-blank use it
	endif
	ifnb	<sseg>		;;check source segment
	loadseg	ds,sseg		;;if non-blank use it
	endif
	ifnb	<dir>		;;check direction
	std			;;go backward if set
	else
	cld			;;otherwise forward
	endif
	rep	movsb		;;do the move
	endm
	;
movew	macro	sseg,soff,dseg,doff,cnt,dir  ;;do a byte move
	ifnb	<soff>		;;check source offset
	mov	si,soff		;;if non-blank use it
	endif
	ifnb	<doff>		;;check destination offset
	mov	di,doff		;;if non-blank use it
	endif
	ifnb	<cnt>		;;check count
	mov	cx,cnt		;;if non-blank use it
	endif
	ifnb	<dseg>		;;check destination segment
	loadseg	es,dseg		;;if non-blank use it
	endif
	ifnb	<sseg>		;;check source segment
	loadseg	ds,sseg		;;if non-blank use it
	endif
	ifnb	<dir>		;;check direction
	std			;;go backward if set
	else
	cld			;;otherwise forward
	endif
	rep	movsw		;;do the move
	endm
	;
	;
	; Minimum / Maximum
	;
min	macro	reg,limit 	;;signed minimum limit
	local	cont		;;continuation label
	cmp	reg,limit 	;;test value
	jge	cont		;;if big enough go on
	mov	reg,limit 	;;too small, raise to minimum
cont	equ	$		;;continue here
	endm
	;
minu	macro	reg,limit 	;;unsigned minimum limit
	local	cont		;;continuation label
	cmp	reg,limit 	;;test value
	jae	cont		;;if big enough go on
	mov	reg,limit 	;;too small, raise to minimum
cont	equ	$		;;continue here
	endm
	;
max	macro	reg,limit 	;;signed maximum limit
	local	cont		;;continuation label
	cmp	reg,limit 	;;test value
	jle	cont		;;if small enough go on
	mov	reg,limit 	;;too big, raise to maximum
cont	equ	$		;;continue here
	endm
	;
maxu	macro	reg,limit 	;;unsigned maximum limit
	local	cont		;;continuation label
	cmp	reg,limit 	;;test value
	jbe	cont		;;if small enough go on
	mov	reg,limit 	;;too big, raise to maximum
cont	equ	$		;;continue here
	endm
	;
	;
	; Procedure entry, exit, and arguments for MASM
	;
	; NOTE:  These macros create some global parameters, which are
	; described below.	In the descriptions, "name" refers to
	; the current procedure name.  Note that these macros do NOT
	; currently support nested procedures.
	;
	; Parameter	Description
	; ---------	-------------------------------------------
	; $$proc		current procedure name (text)
	; $$far		0 if current proc is near, 1 if far (numeric)
	; $$arg		positive byte offset from BP for access to next
	;		argument in current proc (numeric)
	; $$var		negative byte offset from BP for access to most
	;		recently defined local variable in current proc
	;		(numeric)
	;
	; ENTRY -- define an entry point.  Arguments are as follows:
	;
	;	name	entry point name, required
	;	frame	type of frame:
	;		  noframe     none (BP/SP unchanged)
	;		  argframe    set BP/SP for argument access
	;		  varframe    set BP/SP for variable and
	;			      argument access (default)
	;		  varframeC   set BP/SP for variable and
	;			      argument access under C calling
	;			      convention
	;	acc	type of access:
	;		  near	    near procedure (default)
	;		  far	    far procedure
	;	scope	entry point scope:
	;		  local	    local access only
	;		  public	    make entry point public (default)
	;
	;
entry	macro	name,frm,acc,scope	;;start procedure
	local	vartot		;;set up local total variable
	;
	;	set $$frame to 0 (no frame), 1 (argument frame only),
	;	2 (arg/var frame; default), or 3 (arg/var frame with C
	;	calling convention)
	;
	select	$$frame,frm,2,<noframe,argframe,varframe,varframeC>,<Invalid frame>
	;
	;	set $$far to 0 (near -- default) or 1 (far)
	;
	select	$$far,acc,0,<near,far>,<Must be near or far>
	;
	;	set $$scope to 0 (local), 1 (public -- default)
	;
	select	$$scope,scope,1,<local,public>,<Must be local or public>
	;
	;
$$proc	equ	<name>		;;set internal name
	;
	if	_FLAT		;;if flat model ...
$$far	=	0		;;... force everything to near
	endif
	;
	if	$$far		;;if far
name	proc	far		;;make proc far
$$arg	=	_farsz		;;bytes in return address
	else
name	proc	near		;;otherwise near
$$arg	=	_nearsz		;;bytes in return address
	endif
	;
	;
	if	$$scope gt 0	;;set up public label if necessary
	public	name		;;make entry point public
	endif
	;
	if	$$frame gt 0	;;set up stack frame if necessary
	push	xbp		;;save EBP
	mov	xbp,xsp		;;copy frame pointer
	if	$$frame ge 2	;;if there are variables ...
$$vtname	equ	<vartot>		;;save name of local total variable
	sub	xsp,vartot	;;allocate local variable space
	endif
	endif
	;
$$arg	=	$$arg + _regsize	;;account for space to save BP or EBP
$$var	=	0		;;local storage starts at 0
	;
	endm			;;end entry macro
	;
	;
select	macro	name,var,def,values,msg  ;;select value from a list
	local	found, pos	;;found indicator, list counter
	ifb	<var>		;;is it blank?
	ifnb	<def>		;;yes, default specified?
name	=	def		;;if so use default
	exitm			;;get outta here
	else			;;no default specified
	asmerr	<msg>		;;so complain
	endif
	endif
pos	=	0		;;starting in position 0
found	=	0		;;haven't found it yet
	irp	val,<values>	;;scan list
	ifidni	<var>,<val>	;;is name = list entry?
found	=	1		;;OK, found it
	exitm			;;exit irp
	endif
pos	=	pos + 1		;;next position
	endm			;;end irp
	if	found		;;did we find it?
name	=	pos		;;if so set value
	else			;;didn't find it
	asmerr	<msg>		;;so complain
	endif
	endm
	;
	;
asmerr	macro	msg		;;generate error and message
	.err			;;error
	%out msg
	endm
	;
	;
exit	macro			;;exit procedure
	if	$$frame ge 2	;;if var frame is in use, end it
	mov	xsp,xbp		;;restore SP
	endif
	if	$$frame gt 0	;;if frame is in use, end it
	pop	xbp		;;restore BP
	endif
	if	$$frame eq 3	;;C calling convention?
	ret			;;if so just return
	else			;;otherwise return with stack pop
	if	$$far		;;adjust for BP / EBP and ret addr
	ret	$$arg - _regsize - _farsz
	else
	ret	$$arg - _regsize - _nearsz
	endif
	endif
	doendp	%$$proc		;;generate endp
	endm
	;
	;
doendp	macro	name		;;generate endp
name	endp
	endm
	;
	;
arg	macro	name,size 	;;argument definition
	local	aloc		;;local parameter offset
aloc	=	$$arg		;;set local offset
$$arg	=	$$arg + size	;;bump pointer
name	equ	[XBP + aloc]	;;set argument offset
	endm
	;
	;
var	macro	name,size 	;;variable definition
	local	vloc		;;local parameter offset
	if	$$frame lt 2	;;local variables in use?
	.err			;;if not, force error
	%out Variable illegal, stack frame not set up correctly
	else			;;stack frame is in use
$$var	=	$$var + size	;;bump pointer first
vloc	=	$$var		;;set local offset
name	equ	[XBP - vloc]	;;set variable offset
	endif
	endm
	;
	;
varend	macro			;;mark end of local variables
	vtot	%$$vtname 	;;set total bytes
	endm
	;
vtot	macro	totname		;;set total bytes
totname	=	$$var		;;copy total from $$var
	endm
	;
	;
argB	macro	name		;;byte argument definition
	arg	name,1		;;define argument, size is 1
	endm
	;
argW	macro	name		;;word argument definition
	arg	name,2		;;define argument, size is 2
	endm
	;
argD	macro	name		;;double-word argument definition
	arg	name,4		;;define argument, size is 4
	endm
	;
	;
varB	macro	name		;;local byte variable definition
	var	name,1		;;define variable, size is 1
	endm
	;
varW	macro	name		;;local word variable definition
	var	name,2		;;define variable, size is 2
	endm
	;
varD	macro	name		;;local dword variable definition
	var	name,4		;;define variable, size is 4
	endm
	;
	; DOS  and EMS access
	;
calldos	macro	function,errcode	;;call DOS
	ifnb	<function>	;;if function specified
	if	D_&function ge 256	;;is it big enough to need AX?
	mov	ax,D_&function	;;load it
	else			;;otherwise use AH
	mov	ah,D_&function	;;load it
	endif
	endif
	int	I_DOS		;;call DOS
	ifnb	<errcode> 	;;if error code specified
	errc	<errcode> 	;;then error if carry flag set
	endif
	endm
	;
callems	macro	function,errcode	;;call EMS
	ifnb	<function>	;;if function specified
	mov	ah,EM_&function	;;then load it
	endif
	int	I_EMS		;;call EMS
	ifnb	<errcode> 	;;if error code specified
	or	ah,ah		;;check if any error
	errne	<errcode> 	;;then error if non-zero returned
	endif
	endm
	;
callxms	macro	function,driver,errcode    ;;call XMS
	ifnb	<function>	;;if function specified
	mov	ah,XM_&function	;;then load it
	endif
	call	dword ptr driver	;;call caller's driver
	ifnb	<errcode> 	;;if error code specified
	or	ax,ax		;;check if any error
	erre	<errcode> 	;;then error if zero returned
	endif
	endm
	;
progress	macro	msg		;;display progress message
	local	Cont, Txt 	;;continuation, message text labels
	if	DEBUG		;;only if debugging
	pushm	ax,dx,ds		;;save registers
	loadseg	ds,cs,ax		;;set ds = cs
	mov	dx,offset Txt	;;get message offset
	calldos	MESSAGE		;;display message
	popm	ds,dx,ax		;;restore registers
	jmp	Cont		;;and continue
Txt	db	msg		;;message text
	db	CR, LF, '$'	;;close with CRLF and DOS terminator
Cont	equ	$		;;continuation point
	endif
	endm			;;end progress
	;
	;
	;
	; Error handling
	;
errset	macro	name		;;set error handler name
$err	equ	<name>		;;set it
	endm
	;
error	macro	code		;;generate error
	ifnb	<code>		;;if code specified
	mov	dx,E_&code	;;load it
	endif
	doerrjmp	%$err		;;do error jump
	endm
	;
doerrjmp	macro	name		;;generate error jump
	jmp	name		;;do error jump
	endm
	;
	;
errc	macro	code		;;error if carry
	local	noerr		;;label to go to if no error
	jnc	noerr		;;branch if no error
	error	<code>		;;if error, complain
noerr	=	$		;;if no error come here
	endm
	;
errnc	macro	code		;;error if no carry
	local	noerr		;;label to go to if no error
	jc	noerr		;;branch if no error
	error	<code>		;;if error, complain
noerr	=	$		;;if no error come here
	endm
	;
errl	macro	code		;;error if less than
	local	noerr		;;label to go to if no error
	jge	noerr		;;branch if no error
	error	<code>		;;if error, complain
noerr	=	$		;;if no error come here
	endm
	;
errle	macro	code		;;error if less than or equal
	local	noerr		;;label to go to if no error
	jg	noerr		;;branch if no error
	error	<code>		;;if error, complain
noerr	=	$		;;if no error come here
	endm
	;
erre	macro	code		;;error if equal
	local	noerr		;;label to go to if no error
	jne	noerr		;;branch if no error
	error	<code>		;;if error, complain
noerr	=	$		;;if no error come here
	endm
	;
errge	macro	code		;;error if greater than or equal
	local	noerr		;;label to go to if no error
	jl	noerr		;;branch if no error
	error	<code>		;;if error, complain
noerr	=	$		;;if no error come here
	endm
	;
errg	macro	code		;;error if greater than
	local	noerr		;;label to go to if no error
	jle	noerr		;;branch if no error
	error	<code>		;;if error, complain
noerr	=	$		;;if no error come here
	endm
	;
errne	macro	code		;;error if not equal
	local	noerr		;;label to go to if no error
	je	noerr		;;branch if no error
	error	<code>		;;if error, complain
noerr	=	$		;;if no error come here
	endm
	;
errb	macro	code		;;error if below
	local	noerr		;;label to go to if no error
	jae	noerr		;;branch if no error
	error	<code>		;;if error, complain
noerr	=	$		;;if no error come here
	endm
	;
errbe	macro	code		;;error if below or equal
	local	noerr		;;label to go to if no error
	ja	noerr		;;branch if no error
	error	<code>		;;if error, complain
noerr	=	$		;;if no error come here
	endm
	;
errae	macro	code		;;error if above or equal
	local	noerr		;;label to go to if no error
	jb	noerr		;;branch if no error
	error	<code>		;;if error, complain
noerr	=	$		;;if no error come here
	endm
	;
erra	macro	code		;;error if above
	local	noerr		;;label to go to if no error
	jbe	noerr		;;branch if no error
	error	<code>		;;if error, complain
noerr	=	$		;;if no error come here
	endm
	;
	; Message table access
	;
defmsg	macro	num,text		;;macro to put message in list
	local	mbeg, mlen	;;local variables
	db	num, mlen 	;;message number, length
mbeg	equ	$		;;start of message
	db	text		;;and text
mlen	equ	$ - mbeg		;;define length
	endm
	;
.LIST


