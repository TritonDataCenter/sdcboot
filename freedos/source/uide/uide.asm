	page	59,132
	title	UIDE -- DOS "Universal IDE" Caching Driver.
;
; This is a general-purpose caching driver for DOS drives handled thru
; Int 13h BIOS I-O requests.   UIDE runs SATA and UltraDMA disks on up
; to 10 "Legacy" or "Native PCI" controllers via an "internal" driver.
; It caches up to 34 BIOS units, including A: and B: diskettes, and it
; also runs up to 8 CD/DVD drives and caches their data-file requests.
; CD/DVD "raw" input (audio & trackwriters) is handled but not cached.
; "Write Through" caching is done for output.     UIDE uses XMS memory
; and will cache 5 Megabytes to 4 GIGABYTES of data!    See the README
; file for full details about its switches and usage.
;
; When assembled using  /dPMDVR  the "protected caching" UIDE2 variant
; is created.   UIDE2 has these two major changes --
;
;   1) UIDE2 allows a maximum 1900-MB cache and has other HMA limits.
;
;   2) UIDE2 does only "protected caching" and ignores the /F switch.
;
; "Protected caching" puts the cache binary-search table either in the
; HMA (/H or /HL), or in upper/DOS memory, not XMS!   This runs faster
; using protected-mode (JEMM386, etc.), due to less XMS calls.   UIDE2
; is limited to a maximum 165-MB cache for /H and a maximum 280-MB for
; /HL, with V7.10 MS-DOS.   /H makes UIDE2 take 944 bytes of upper/DOS
; memory, with the rest of its logic plus its search table in the HMA.
; /HL makes UIDE2 take 4608 bytes of memory but allows a 115-MB larger
; HMA cache!   Other DOS systems can set at least a 315-MB /H cache or
; a 425-MB /HL cache, depending on free HMA (The author's V6.22 MS-DOS
; can set a 600-MB /HL cache!).
;
; If free HMA is too little for a desired cache's binary-search table,
; UIDE2 defaults down to 80-MB, 40-MB, or 15-MB, whichever "fits" into
; free HMA.    A 5-MB HMA cache can be requested, but for other sizes,
; if free HMA is too low for even 15-MB, UIDE2 will default to loading
; in only upper- or DOS memory.
;
; In upper/DOS memory (no /H nor /HL, or "by default" as above), UIDE2
; can set caches up to 1900-MB, and its size increases by 32 bytes per
; MB of cache.    A 250-MB cache takes 12.5K of memory, a 500-MB cache
; takes 20K, and a 1900-MB cache takes 64K.   UIDE2 loads into 7K+, so
; it can always set up to 80-MB caches in upper/DOS memory.    If more
; than 80-MB is requested but memory is inadequate, UIDE2 will set its
; default 80-MB cache.
;
; JEMM386/EMM386 systems having more than one area of upper-memory may
; wish to load UIDE2 in upper-memory as follows --
;
;    DEVICEHIGH=/L:2 C:\...\UIDE2.SYS ...
;
; The /L:2 switch tells the DOS loader to put UIDE2 in the second area
; of upper-memory, avoiding the "monochrome video" area from B0000h to
; B7FFFh (if "mapped" as upper-memory) which may be inadequate to hold
; larger UIDE2 binary-search tables!   Note again that with /H or /HL,
; UIDE2 will set its search table in the HMA, and the driver then uses
; 944/4608 bytes of upper-memory, usually small enough to "fit" in the
; first or any upper-memory area, with no specific /L: loader switch.
;
; Excepting the above, UIDE2 offers all features and switch options of
; the standard UIDE driver.    UIDE2 handles a /N3 "No XMS" switch and
; defaults to a "UIDE$" driver name, when no CD/DVD drives were found.
; Thus, UIDE2 can be used with FreeDOS "automatic loader" scripts.
;
; UIDE, UIDE-S, and UIDE2 can all be called by a user driver, to cache
; data for its reads and writes.    See the file UIDE.TXT for details.
;
;
; General Program Equations.
;
	.386p			;Allow use of 80386 instructions.
	.lfcond			;List conditionally-unassembled lines.
s	equ	<short>		;Make a conditional jump "short".
SBUFSZ	equ	512		;Regular 512-byte search buffer.  UIDE
				;  will assemble with "SBUFSZ equ 256"
				;  to use less HMA.  Ignored by UIDE2.
MAXBIOS	equ	34		;Maximum 34 BIOS units supported.
STACK	equ	520		;Driver local-stack size.
CDTYP	equ	07Ch		;CD and DVD device type code.
CDUNIT	equ	48		;CD/DVD starting cache-unit number.
RMAXLBA	equ	00006DD39h	;Redbook (audio) maximum LBA value.
COOKSL	equ	2048		;CD/DVD "cooked" sector length.
RAWSL	equ	2352		;CD/DVD "raw" sector length.
CMDTO	equ	00Ah		;CD/DVD 500-msec min. command timeout.
SEEKTO	equ	037h		;CD/DVD 3-second min. "seek"  timeout.
STARTTO	equ	07Fh		;Disk/CD/DVD 7-second startup timeout.
HDWRFL	equ	00410h		;BIOS hardware-installed flag.
DKTSTAT	equ	00441h		;BIOS diskette status byte.
BIOSTMR equ	0046Ch		;BIOS "tick" timer address.
HDISKS	equ	00475h		;BIOS hard-disk count address.
VDSFLAG equ	0047Bh		;BIOS "Virtual DMA" flag address.
HDI_OFS	equ	0048Eh-BIOSTMR	;BIOS hard-disk int. flag "offset".
MCHDWFL	equ	0048Fh		;Diskette hardware media-change flags.
IXM	equ	4096		;IOCTL transfer-length multiplier.
CR	equ	00Dh		;ASCII carriage-return.
LF	equ	00Ah		;ASCII line-feed.
TAB	equ	009h		;ASCII "tab".
;
; Driver Error Return Codes.
;
MCHANGE	equ	006h		;Diskette "media change".
GENERR	equ	00Ch		;CD/DVD "general error".
DMAERR	equ	00Fh		;Hard disk DMA error.
CTLRERR equ	020h		;Hard disk controller not-ready.
DISKERR equ	0AAh		;Hard disk drive not-ready.
FAULTED	equ	0CCh		;Hard disk drive FAULTED.
HARDERR equ	0E0h		;Hard disk I-O error.
XMSERR	equ	0FFh		;XMS memory error.
;
; "Legacy IDE" Controller I-O Base Addresses.
;
NPDATA	equ	001F0h		;Normal primary      base address.
NSDATA	equ	00170h		;Normal secondary    base address.
APDATA	equ	001E8h		;Alternate primary   base address.
ASDATA	equ	00168h		;Alternate secondary base address.
;
; IDE Controller Register Offsets.
;
CDATA	equ	0		;Data port offset.
CDSEL	equ	6		;Disk-select and upper LBA offset.
CCMD	equ	7		;Command-register offset.
CSTAT	equ	7		;Primary-status register offset.
;
; Controller Status and Command Definitions.
;
BSY	equ	080h		;IDE controller is busy.
RDY	equ	040h		;IDE disk is "ready".
FLT	equ	020h		;IDE disk has a "fault".
DRQ	equ	008h		;IDE data request.
ERR	equ	001h		;IDE general error flag.
DMI	equ	004h		;DMA interrupt occured.
DME	equ	002h		;DMA error occurred.
MSEL	equ	0A0h		;"Master" device-select bits.
SSEL	equ	0B0h		;"Slave"  device-select bits.
DRCMD	equ	0C8h		;DMA read command (write is 0CAh,
				;    LBA48 commands are 025h/035h).
LBABITS equ	0E0h		;Fixed LBA command bits.
;
; Byte, Word, and Double-Word Definitions.
;
BDF	struc
lb	db	?
hb	db	?
BDF	ends

WDF	struc
lw	dw	?
hw	dw	?
WDF	ends

DDF	struc
dwd	dd	?
DDF	ends
;
; LBA "Device Address Packet" Layout.
;
DAP	struc
DapPL	db	?		;Packet length.
	db	?		;(Reserved).
DapSC	db	?		;I-O sector count.
	db	?		;(Reserved).
DapBuf	dd	?		;I-O buffer address (segment:offset).
DapLBA	dw	?		;48-bit logical block address (LBA).
DapLBA1	dd	?
DAP	ends
;
; DOS "Request Packet" Layout.
;
RP	struc
RPHLen	db	?		;Header byte count.
RPSubU	db	?		;Subunit number.
RPOp	db	?		;Opcode.
RPStat	dw	?		;Status word.
	db	8 dup (?)	;(Unused by us).
RPUnit	db	?		;Number of units found.
RPLen	dw	?		;Resident driver 32-bit length.
RPSeg	dw	?		;(Segment must ALWAYS be set!).
RPCL	dd	?		;Command-line data pointer.
RP	ends
RPERR	equ	08003h		;Packet "error" flags.
RPDON	equ	00100h		;Packet "done" flag.
RPBUSY	equ	00200h		;Packet "busy" flag.
;
; IOCTL "Request Packet" Layout.
;
IOC	struc
	db	13 dup (?)	;Request "header" (unused by us).
	db	?		;Media descriptor byte (Unused by us).
IOCAdr	dd	? 		;Data-transfer address.
IOCLen	dw	?		;Data-transfer length.
	dw	?		;Starting sector (unused by us).
	dd	?		;Volume I.D. pointer (unused by us).
IOC	ends
;
; Read Long "Request Packet" Layout.
;
RL	struc
	db	13 dup (?)	;Request "header" (unused by us).
RLAM	db	?		;Addressing mode.
RLAddr	dd	?		;Data-transfer address.
RLSC	dw	?		;Data-transfer sector count.
RLSec	dd	?		;Starting sector number.
RLDM	db	?		;Data-transfer mode.
RLIntlv	db	?		;Interleave size.
RLISkip	db	?		;Interleave skip factor.
RL	ends
;
; Segment Declarations.
;
CODE	segment	public use16 'CODE'
	assume	cs:CODE,ds:CODE
;
; DOS Driver Device Header.
;
@	dd	0FFFFFFFFh	;Link to next header block.
	dw	0C800h		;Driver "device attributes".
StratP	dw	(I_Stra-@)	;"Strategy" routine pointer.
	dw	(I_Init-@)	;Device-Interrupt routine pointer.
DvrNam	db	'UDVD1   ',0	;Driver name for use by SHCDX33E, etc.
VLF	db	0		;VDS "lock" flag (001h = buffer lock).
	db	0		;First assigned drive letter.
CDUnits	db	0		;Number of CD/DVD drives (0 to 8).
USRPtr	dw	(Switch-@)	;User-driver entry pointer.
;
; General Driver "Data Page" Variables.
;
IOF	db	002h		;I-O control flags --
				;  Bit 7:  Driver is busy.
				;  Bit 1:  Cache must be flushed.
	db	0		;User-driver cache unit "bitmap".
XMF	db	0		;CD/DVD XMS move  flag (01h if so).
DMF	db	0		;CD/DVD DMA input flag (01h if so).
DMAAd	dw	0		;DMA status/command register address.
IdeDA	dw	0		;IDE data register address.
RqSec	db	0		;Request I-O sector count.
RqCmd	db	0		;Request I-O command bits.
RqCSC	db	0		;Request current-block sector count.
RqTyp	db	0		;Request device-type flag.
RqPkt	dd	0		;Request I-O packet address.
RqBuf	dd	0		;Request I-O buffer address.
RqXMS	dd	128		;Request XMS buffer offset.
RqLBA	equ	[$].lw		;Request initial LBA address.
RqUNo	equ	[$+6].lb	;Request "cache unit" number.
CBLBA	equ	[$+8].lw	;Current-buffer initial LBA.
CBUNo	equ	[$+14].lb	;Current-buffer "cache unit" number.
CBSec	equ	[$+15].lb	;Current-buffer sector count.
CBLst	equ	[$+16].lw	;Current-buffer "last" LRU index.
CBNxt	equ	[$+18].lw	;Current-buffer "next" LRU index.
AudAP	equ	[$+20].lw	;CD/DVD audio-start address pointer.
STLmt	equ	[$+22].lw	;Cache binary-search limit index.
LUTop	equ	[$+24].lw	;Least-recently-used "top" index.
LUEnd	equ	[$+26].lw	;Least-recently-used "end" index.
;
; Driver "Final Initialization" Routine.   This logic becomes the
;   variables listed above after Init.   The driver stack must be
;   zeroed here, as the stack address for protected-cache drivers
;   varies when loaded in upper- or DOS memory!
;
I_ClrS:	dec	bx		;Clear driver stack (helps debug).
	mov	[bx],al
	loop	I_ClrS
	cmp	cx,@LastU	;Any BIOS disks/diskettes to use?
	je s	I_End		;No, reload all registers and exit.
	mov	ax,02513h	;"Hook" this driver into Int 13h.
	mov	dx,offset (Entry-@)
	int	021h
I_End:	popad			;Reload all 32-bit registers.
I_Bye:	pop	dx		;Reload 16-bit registers we used.
	pop	bx
	pop	ax
	pop	es		;Reload CPU segment registers.
	pop	ds
	popf			;Reload CPU flags and exit.
	retf
;
; ATAPI "Packet" Area (always 12 bytes for a CD/DVD).
;
Packet	db	0		;Opcode.
	db	0		;Unused (LUN and reserved).
PktLBA	dd	0		;CD/DVD logical block address.
PktLH	db	0		;"Transfer length" (sector count).
PktLn	dw	0		;Middle- and low-order sector count.
PktRM	db	0		;Read mode ("Raw" Read Long only).
	dw	0		;Unused ATAPI "pad" bytes (required).
;
; VDS Parameter Block.
;
VDSLn	dd	(NormEnd-@)	;VDS block length.
VDSOf	dd	0		;VDS 32-bit buffer offset.
VDSSg	dd	0		;VDS 16-bit segment (hi-order zero).
VDSAd	dd	0		;VDS 32-bit physical buffer address.
;
; UltraDMA Command-List.
;
IOAdr	dd	0		;DMA 32-bit buffer address.
IOLen	dd	080000000h	;DMA byte count and "end" flag.
;
; "PRD" Address and IDE Parameter Area.
;
PRDAd	dd	(IOAdr-@)	;PRD 32-bit command addr. (Init set).
	db	0		;IDE "upper" sector count (always 0).
LBA2	db	0,0,0		;IDE "upper" LBA bits 24-47.
SecCt	db	0		;IDE "lower" sector count.
LBA	db	0,0,0		;IDE "lower" LBA bits 0-23.
DSCmd	db	0		;IDE disk-select and LBA commands.
IOCmd	db	0		;IDE command byte.
Try	equ	LBA2.lb		;CD/DVD I-O retry counter.
UserB	equ	SecCt.dwd	;CD/DVD working I-O buffer address.
UserL	equ	DSCmd.lw	;CD/DVD working I-O buffer length.
;
; BIOS "Active Unit" Table.   The beginning of this table MUST be at
;   driver address 07Fh or below, for BX-reg. addressing at "Entry".
;
Units	db	MAXBIOS dup (0)	;BIOS unit number for each device.
;
; Miscellaneous Driver Variables, not requiring BX-reg. addressing.
;
CStack	dd	0		;Caller's saved stack pointer.
	ifndef	PMDVR		;For UIDE only --
STblAdr	dd	0		;  Search table address (Init set).
	else			;For UIDE2 only --
STblAdr	dd	(SrchT-@)	;  Search table address (Init set).
	endif
LBABuf	dw	00082h,0	;"Calculated LBA" buffer (8 bytes).
PCISubC	dw	00100h		;PCI subclass/function (Init only).
HMASize	dw	HMALEN2		;HMA space required    (Init only).
;
; Cache "Working Buffer" (shares the CD/DVD Audio Function Buffer).
;
WBLBA	equ	[$+4].lw	;Working-buffer initial LBA.
WBUNo	equ	[$+10].lb	;Working-buffer "cache unit" number.
WBSec	equ	[$+11].lb	;Working-buffer sector count.
WBLst	equ	[$+12].lw	;Working-buffer "last" LRU index.
WBNxt	equ	[$+14].lw	;Working-buffer "next" LRU index.
;
; CD/DVD Audio Function Buffer (16 bytes) for most CD/DVD "audio"
;   requests.    Its variables below are only for initialization.
;
InBuf	equ	$
UTblP	dw	UTable		;Initialization unit table pointer.
NoDMA	db	0		;"No UltraDMA" flag.
UMode	dw	0		;UltraDMA "mode select" bits.
UFlag	db	0		;UltraDMA "mode valid" flags.
USNdx	db	5		;Unit-select index.
USelB	db	082h		;PCI "modes" and unit-select byte.
DVDTblA	dw	(DVD1Tbl-@)	;PCI "scan" address table pointer.
DVDTblB	dw	(DVD1Tbl-@)	;CD/DVD setup address table pointer.
BiosHD	db	0FFh		;Number of BIOS hard disks.
HDUnit	db	0		;Current BIOS unit number.
SFlag	db	CSDflt		;User cache-size flag.
HFlag	db	0		;"Use HMA space" flag.
;
; Hard-Disk and Diskette Int 13h Entry Routine.   For a CHS request,
;   the registers are:
;
;   AH      Request code.  We handle 002h read and 003h write.
;   AL      I-O sector count.
;   CH      Lower 8 bits of starting cylinder.
;   CL      Starting sector and upper 2 bits of cylinder.
;   DH      Starting head.
;   DL      BIOS unit number:  000h/001h diskettes, 080h+ hard-disks.
;   ES:BX   I-O buffer address.
;
; For an LBA request, the registers are:
;
;   AH      Request code.  We handle 042h read and 043h write.
;   DL      BIOS unit number:  000h/001h diskettes, 080h+ hard-disks.
;   DS:SI   Pointer to Device Address Packet ("DAP") as shown above.
;
Entry:	pushf			;Save CPU flags and BP-reg.
	push	bp
	mov	bp,0		;Reset active-units table index.
@LastU	equ	[$-2].lw	;(Last-unit index, set by Init).
NextU:	dec	bp		;Any more active units to check?
	js s	QuickX		    ;No, not for us -- exit quick!
	cmp	dl,cs:[bp+Units-@]  ;Does request unit match table?
	jne s	NextU		    ;No, see if more units remain.
	cli			;Disable CPU interrupts.
	bts	cs:IOF,7	;Is this driver currently "busy"?
	jc s	InvalF		;Yes?  Set "invalid function" & exit!
Switch:	mov	cs:CStack.lw,sp	;Save caller's stack pointer.
	mov	cs:CStack.hw,ss
	push	cs		;Switch to our driver stack, to avoid
	pop	ss		;  CONFIG.SYS "STACKS=0,0" problems!!
	mov	sp,0
@Stack	equ	[$-2].lw	;(Starting stack pointer, Init set).
	sti			;Re-enable CPU interrupts.
	pushad			;Save all CPU registers.
	push	ds
	push	es
	pusha			;Issue "A20 local-enable" request.
	mov	ah,005h
	call	A20Req
	popa
	jc s	A20Er		;If "A20" error, use routine below.
	db	0EAh		;Go to main Int 13h request handler.
@HDMain	dd	(HDReq-@)	;(Main entry address, set by Init).
PassRq:	call	A20Dis		;Pass request -- Disable "A20" line.
	cli			;Disable CPU interrupts.
	and	[bx+IOF-@].lb,2	;Reset all but "flush cache" flag.
	pop	es		;Reload all CPU registers.
	pop	ds
	popad
	lss	sp,cs:CStack	;Switch back to caller's stack.
QuickX:	pop	bp		;Reload BP-register and CPU flags.
	popf
	nop			;(Unused alignment "filler").
	db	0EAh		;Go to next routine in Int 13h chain.
@Prev13	dd	0		;(Prior Int 13h vector, set by Init).
HDExit:	pushf			;Exit -- save error flag and code.
	push	ax
	call	VDUnlk		;Do VDS "unlock" and disable "A20".
	pop	ax		;Reload error code and error flag.
	popf
Abandn:	mov	bp,sp		;Point to saved registers on stack.
	mov	[bp+33],al	;Set error code in exiting AH-reg.
	cli			;Disable CPU interrupts.
	lahf			;Save error flag (carry) for below.
	and	IOF,002h	;Reset all but "flush cache" flag.
	sahf			;Reload error flag (carry bit).
	pop	es		;Reload all CPU registers.
	pop	ds
	popad
	lss	sp,cs:CStack	;Switch back to caller's stack.
GetOut:	mov	bp,sp		;Set error flag in exiting carry bit.
	rcr	[bp+8].lb,1	;(One of the flags saved by Int 13h).
	rol	[bp+8].lb,1
	pop	bp		;Reload BP-register and CPU flags.
	popf
	iret			;Exit.
A20Er:	mov	al,XMSERR	;"A20" error!  Post "XMS error" code.
	mov	IOF,al		;Flush cache on next caching request.
	jmp s	Abandn		;Abandon this request and exit QUICK!
InvalF:	mov	ah,001h		;We are BUSY, you fool!  Set "invalid
	jmp s	GetOut		;  function" code and get out, QUICK!
;
; DOS Device-Interrupt Routine.   This logic handles CD/DVD requests.
;   Our driver stack is not used, as SHCDX33E/MSCDEX provide a stack.
;
DevInt:	pushf			;Save CPU flags & disable interrupts.
	cli
	pushad			;Save all CPU registers but ES-reg.
	push	ds
	lds	si,cs:RqPkt	   ;Get DOS request-packet address.
	mov	[si].RPStat,0810Ch ;Post default "general error".
	bts	cs:IOF,7	   ;Is this driver currently "busy"?
	jc s	DvIntX		   ;Yes?  Reload registers and exit!
	sti			;Re-enable CPU interrupts.
	push	es		;Save ES-register now.
	mov	ah,005h		;Issue "A20 local-enable" request.
	call	A20Req		;(Makes critical settings on exit).
	jc s	DvIntE		;If "A20" failure, go exit below.
	db	09Ah		;Call main CD/DVD request handler.
@CDMain	dd	(CDReq-@)	;(CD/DVD request address, Init set).
	call	VDUnlk		;Do "VDS unlock" and disable "A20".
DvIntE:	pop	es		;Reload ES-register now.
	cli			;Disable CPU interrupts.
	and	[bx+IOF-@].lb,2	;Reset all but "flush cache" flag.
DvIntX:	pop	ds		;Reload all remaining CPU registers.
	popad
	popf			;Reload CPU flags and exit.
	retf
;
; DOS "Strategy" Routine, used only for CD/DVD requests.
;
Strat:	push	es		;Save DOS request-packet address.
	push	bx
	pop	cs:RqPkt
	retf			;Exit and await "Device Interrupt".
;
; Subroutine to issue a VDS "unlock" request.   Note that this logic
;   is only used on exit from I-O requests, so it "falls through" to
;   issue an "A20 local-disable" before the driver actually exits.
;
VDUnlk:	shr	[bx+VLF-@].lb,1	;"Unlock" -- Is I-O buffer "locked"?
	jnc s	A20Dis		;No, go issue "A20 local disable".
	mov	ax,08104h	;Get VDS "unlock" parameters.
	xor	dx,dx
	mov	di,(VDSLn-@)	;Point to VDS parameter block.
	push	ds
	pop	es
	int	04Bh		;Execute "VDS unlock" request.
;
; Subroutine to issue "A20" local-enable and local-disable requests.
;
A20Dis:	mov	ah,006h		;"A20 local disable" -- get XMS code.
A20Req:	db	09Ah		;Call XMS manager for "A20" request.
@XEntry	equ	dword ptr $	;  (XMS "entry" address, set by Init).
	nop			;  If no XMS memory, set AX-reg. to 1.
	mov	ax,00001h
	sti			;RESTORE all critical driver settings!
	cld			;(Never-NEVER "trust" external code!).
	push	cs
	pop	ds
	xor	bx,bx		;Rezero BX-reg. for relative commands.
	sub	al,1		;Zero AL-reg. if success, -1 if error.
	ret			;Exit.
RqIndex	dw	(Ctl1Tbl-@)	;Request cache-table index (put here
				;  to allow more "Units" table space).
NormEnd	equ	($+STACK)	;End of driver's data and entry logic.
				;  All that follows can go in the HMA.
;
; Main hard-disk request handler.
;
HDReq:	cmp	bp,CDUNIT	;Is this a CD or DVD request?
	jae	SetBuf		;Yes, go set I-O parameters below.
	mov	dl,0BEh		;Mask out LBA & write request bits.
	and	dl,ah
	cmp	dl,002h		    ;CHS or LBA read/write request?
	mov	dl,cs:[bp+TypeF-@]  ;(Set unit "type" flag anyway).
@TypeF	equ	[$-2].lw
	mov	RqTyp,dl
	je s	ChkLBA		;Yes, check for actual LBA request.
	cmp	dl,0C0h		;Not for us -- Diskette I-O request?
	jne s	Pass		;No, "flush" cache and pass request.
	cmp	ah,005h		;Diskette track-format request?
	jne s	Pass1		;No, "pass" other diskette requests.
	nop			;(Unused alignment "filler").
Pass:	or	IOF,002h	;Flush cache on next caching request.
Pass1:	push	ss		;Return to request "pass" routine.
	push	(PassRq-@)
	retf
ChkLBA:	mov	di,sp		;Point DI-reg. to current stack.
	shl	ah,1		;Is this an LBA read or write request?
	jns s	ValSC		;No, go validate CHS sector count.
	push	ds		;Save this driver's DS-reg.
	mov	ds,[di+2]	;Reload "DAP" segment into DS-reg.
	cmp	[si].DapBuf,-1	;Check for 64-bit "DAP" I-O buffer.
	mov	al,[si].DapSC	;(Get "DAP" I-O sector count).
	les	dx,[si].DapLBA1	;(Get "DAP" LBA bits 16-47).
	mov	di,es
	les	cx,[si].DapBuf	;(Get "DAP" I-O buffer address).
	mov	si,[si].DapLBA	;(Get "DAP" LBA bits 0-15).
	pop	ds		;(Reload this driver's DS-reg.).
	je s	Pass		;If 64-bit buffer, pass this request!
ValSC:	dec	al		;Is sector count zero or over 128?
	js s	Pass		;Yes?  Let BIOS handle this "No-No!".
	inc	ax		;Restore sector count -- LBA request?
	xchg	ax,cx		;(Save sector count & command in CX).
	js s	ZeroBX		;Yes, go zero BX-reg. for our logic.
	mov	es,[di]		;CHS -- Reload I-O buffer segment.
	xor	di,di		;Reset upper LBA address bits.
	mov	si,0003Fh	;Set SI-reg. to starting sector.
	and	si,ax
	dec	si
	shr	al,6		;Set AX-reg. to starting cylinder.
	xchg	al,ah
	xchg	ax,dx		    ;Swap cylinder & head values.
	mov	al,cs:[bp+CHSec-@]  ;Get disk CHS sectors/head value.
@CHSec	equ	[$-2].lw
	or	al,al		    ;Were disk CHS values legitimate?
	jz s	Pass		    ;No?  Let BIOS have this request!
	push	ax		    ;Save CHS sectors/head value.
	mul	ah		    ;Convert head to sectors.
	add	si,ax		    ;Add result to starting sector.
	pop	ax		    ;Reload CHS sectors/head value.
	mul	cs:[bp+CHSHd-@].lb  ;Convert cylinder to sectors.
@CHSHd	equ	[$-2].lw
	mul	dx
	add	si,ax		;Add to head/sector value.
	adc	dx,di
	xchg	ax,bx		   ;Get buffer offset in AX-reg.
ZeroBX:	xor	bx,bx		   ;Zero BX-reg. for our logic.
	mov	[bx+VDSOf-@].lw,ax ;Set VDS I-O buffer address.
	mov	[bx+VDSOf-@].hw,bx
	mov	[bx+VDSSg-@],es
	and	ch,006h		   ;Mask off read/write commands.
	movzx	eax,cl		   ;Multiply sectors by 512 bytes.
	shl	eax,9
	mov	[bx+VDSLn-@],eax   ;Set VDS buffer length.
	call	VDLock		   ;Do VDS "lock" on user buffer.
	jc s	Pass		   ;VDS error:  Pass this request!
SetBuf:	mov	[bx+RqBuf-@],eax   ;Set user I-O buffer address.
	mov	[bx+RqSec-@],cx	   ;Set sector count & I-O command.
	mov	[bx+RqLBA-@],di	   ;Set initial request LBA.
	mov	[bx+RqLBA+2-@],dx
	mov	[bx+RqLBA+4-@],si
	mov	[bx+RqUNo-@],bp	   ;Set "cache unit" number.
GoMain:	jmp	Cache		;Go do caching or call "UdmaIO".
	push	ss		;If "stand alone" driver, exit now.
	push	(HDExit-@)
	retf
	db	0		;(Unused alignment "filler").
;
; Subroutine to set up and execute hard disk UltraDMA I-O requests.
;   The "DoDMA" subroutine (below) is then used to perform the I-O.
;
UdmaIO:	mov	eax,[bx+RqBuf-@]   ;Get user I-O buffer address.
	call	UseCB		   ;Can current cache buffer be used?
	jnc s	BufIO		   ;Yes, go do buffered I-O below.
	call	SetAdr		   ;Set user or Main XMS buffer addr.
	jnc s	DoDMA		     ;If user buffer, do DMA & exit.
BufIO:	test	[bx+RqCmd-@].lb,002h ;Buffered I-O:  Input request?
	jz s	UBufIn		     ;Yes, use input routine below.
	call	UBufMv		    ;Output:  Move user data to XMS.
	jnc s	DoDMA		    ;If no XMS error, do DMA & exit.
UdmaX:	ret			    ;XMS or I-O error!  Bail out now!
UBufIn:	call	DoDMA		    ;Input:  Read data to XMS buffer.
	jc s	UdmaX		    ;If any I-O errors, bail out now!
	nop			    ;(Unused alignment "filler").
UBufMv:	movzx	ecx,[bx+RqSec-@].lb ;Get number of sectors to move.
	mov	esi,[bx+IOAdr-@]    ;Set move addresses for a read.
	mov	edi,esi
	xchg	edi,[bx+RqBuf-@]
UBufM1:	jz s	UBufM2		 ;Is this a read from XMS memory?
	xchg	esi,edi		 ;No, "swap" source & destination.
UBufM2:	shl	ecx,9		 ;Convert sectors to byte count.
	jmp	MvData		 ;Move all data to/from XMS and exit.
;
; Subroutine to clear our ATAPI "packet" for the next CD/DVD request.
;
ClrPkt:	mov	bl,12		 ;Set ATAPI packet size (loop count).
ClrPk1:	dec	bx		 ;Clear next ATAPI packet byte.
	mov	[bx+Packet-@],bh
	jnz s	ClrPk1		 ;If more bytes to clear, loop back.
	ret			 ;Exit.
;
; Subroutine to handle hard-disk UltraDMA I-O requests for "UdmaIO".
;
DoDMA:	mov	cx,[bx+RqSec-@]	  ;Get sector count and I-O command.
	mov	si,[bx+RqLBA+4-@] ;Get 48-bit logical block address.
	les	di,[bx+RqLBA-@]
	mov	dx,es
DiskIO:	mov	[bx+LBA-@],si	  ;Set 48-bit LBA in IDE commands.
	mov	[bx+LBA+2-@],dl
	mov	[bx+LBA2-@],dh
	mov	[bx+LBA2+1-@],di  ;Set LBA bits 16 thru 47.
	mov	bp,[bx+RqTyp-@]	  ;Get this disk's "type" flags.
	mov	ax,bp		  ;Get disk's IDE controller number.
	shr	al,2
	mov	ah,6		  ;Point to disk's I-O addresses.
	mul	ah
	add	ax,offset (Ctl1Tbl-@)
@CtlTbl	equ	[$-2].lw
	xchg	ax,si
	lods	cs:[si].dwd	  ;Set controller base address and
	mov	[bx+DMAAd-@],eax  ;  primary-channel data address.
	rcr	bp,2		  ;Primary channel I-O request?
	jnc s	GetCmd		  ;Yes, get LBA28/LBA48 commands.
	add	[bx+DMAAd-@].lw,8 ;Use secondary DMA controller.
	lods	cs:[si].lw	  ;Set secondary channel data addr.
	mov	[bx+IdeDA-@],ax
GetCmd:	shr	dx,12		  ;Shift out LBA bits 16-27.
	or	di,dx		  ;Anything in LBA bits 28-47?
	mov	dl,(LBABITS/32)	  ;(Init LBA cmd. now for alignment).
	jnz s	LBA48		  ;Yes, use LBA48 read/write command.
	xchg	dh,[bx+LBA2-@]	  ;LBA28:  Reload & reset bits 24-27.
	or	ch,(DRCMD+1)	  ;Get LBA28 read/write command + 5.
	jmp s	SetCmd		  ;Go set LBA and IDE command bytes.
LBA48:	shl	ch,3		  ;LBA48 -- get command as 20h/30h.
SetCmd:	shl	bp,1		  ;Shift master/slave into LBA cmds.
	rcl	dl,5
	or	dl,dh		  ;"Or" in LBA28 bits 24-27 (if any).
	mov	dh,005h		  ;Get final IDE command byte --
	xor	dh,ch		  ;  LBA28 C8h/CAh, LBA48 25h/35h.
	mov	[bx+DSCmd-@],dx	  ;Set LBA and IDE command bytes.
	mov	[bx+SecCt-@],cl	  ;Set disk I-O sector count.
	mov	[bx+IOLen-@],bl	  ;Set 24-bit DMA buffer length.
	mov	ch,0		  ;  We include bit 16 for 65536
	shl	cx,1		  ;  bytes, as a few "odd" chips
	mov	[bx+IOLen+1-@],cx ;  may NOT run O.K. without it!
	call	MovCLX		  ;Move DMA command-list up to XMS.
	jc s	DoDMAX		;Command-list move ERROR?  Exit NOW!
	mov	dx,[bx+DMAAd-@]	;Ensure any previous DMA is stopped!
	in	al,dx		;(On some older chipsets, if DMA is
	and	al,0FEh		;  running, reading an IDE register
	out	dx,al		;  causes the chipset to "HANG"!!).
	mov	di,[bx+IdeDA-@]	;Get our disk's IDE base address.
	mov	al,[bx+DSCmd-@]	;Select our desired disk.
	and	al,0F0h
	lea	dx,[di+CDSEL]
	out	dx,al
	mov	es,bx		;Point to low-memory BIOS timer.
	mov	si,BIOSTMR
	mov	cx,((STARTTO*256)+FLT) ;Get timeout & "fault" mask.
	add	ch,es:[si]	       ;Set timeout limit in CH-reg.
	nop			       ;Await controller/disk ready.
	call	ChkRdy		       ;(KEEP above "nop" as-is!).
	jc s	DoDMAX		       ;If any errors, exit below!
	test	[bx+IOCmd-@].lb,012h   ;Is this a write request?
	jnz s	SetDMA		       ;Yes, init DMA transfer.
	mov	al,008h		;Get "DMA read" command bit.
SetDMA:	push	si		;Save BIOS timer pointer.
	call	IniDMA		;Initialize this DMA I-O transfer.
	mov	ax,001F7h	;Set IDE parameter-output flags.
NxtPar:	lea	dx,[di+CDATA+1]	;Point to IDE sector count reg. - 1.
	nop			;(Unused alignment "filler").
IDEPar:	inc	dx		;Output all ten LBA48 parameter bytes.
	outsb			;(1st 4 overlayed by 2nd 4 if LBA28!).
	shr	ax,1		;More parameters to go in this group?
	jc s	IDEPar		;Yes, loop back and output next one.
	jnz s	NxtPar		;If first 4 done, go output last 6.
	pop	si		;Reload BIOS timer pointer.
	dec	dh		;"Legacy IDE" controller channel?
@DRQ:	jmp s	DMAGo		;No, forget about awaiting 1st DRQ.
	mov	dh,003h		;Get IDE alternate-status address.
	dec	dx		;(Primary-status address | 300h - 1).
ChkDRQ:	cmp	ch,es:[si]	;Too long without 1st data-request?
	je s	ErrDMA		;Yes?  Return carry and DMA error!
	in	al,dx		;Read IDE alternate status.
	and	al,DRQ		;Has 1st data-request arrived?
	jz s	ChkDRQ		;No, loop back and check again.
DMAGo:	call	RunDMA		;Start and monitor our DMA transfer.
	jnz s	ErrDMA		;Problems?  Return carry & DMA error!
	inc	cx		;Check "fault" and hard-error at end.
ChkRdy:	lea	dx,[di+CSTAT]	;Read IDE primary status.
	in	al,dx
	cmp	ch,es:[si]	;Too long without becoming ready?
	je s	RdyErr		;Yes?  Go see what went wrong.
	test	al,BSY+RDY	;Controller or disk still busy?
	jle s	ChkRdy		;Yes, loop back and check again.
	and	al,cl		;Disk "fault" or hard-error?
	jnz s	HdwErr		;Yes?  Go see what went wrong.
DoDMAX:	ret			;End of request -- Return to "UdmaIO".
ErrDMA:	mov	al,DMAERR	;BAAAD News!  Post DMA error code.
ErrDMX:	stc			;Set carry flag (error!) and exit.
	ret
RdyErr:	test	al,BSY		;BAAAD News!  Did controller go ready?
	mov	ax,(256*CTLRERR)+DISKERR ;(Get not-ready error codes).
	jmp s	WhichE		;Go see which error code to return.
HdwErr:	test	al,FLT		;BAAAD News!  Is the disk "faulted"?
	mov	ax,(256*FAULTED)+HARDERR ;(Get hardware error codes).
WhichE:	jz s	Kaput		;If "zero", use AL-reg. return code.
	mov	al,ah		;Use AH-reg. return code of this pair.
Kaput:	stc			;Set carry flag (error!) and exit.
	ret
;
; Subroutine to issue a VDS "lock" request.
;
VDLock:	pusha			     ;Save all 16-bit CPU registers.
	push	ds
	or	[bx+VDSAd-@].dwd,-1  ;Invalidate VDS buffer address.
	mov	ax,08103h	     ;Get VDS "lock" parameters.
	mov	dx,0000Ch
	mov	di,(VDSLn-@)	     ;Point to VDS parameter block.
	push	ds
	pop	es
	int	04Bh		     ;Execute "VDS lock" VDS request.
	sti			     ;RESTORE all critical settings!
	cld			     ;(NEVER "trust" external code!).
	pop	ds		     ;Reload all 16-bit registers.
	popa
	jc s	VDLokX		     ;VDS error:  Exit immediately!
	mov	eax,[bx+VDSAd-@]     ;Get 32-bit VDS buffer address.
	cmp	eax,-1		     ;Is VDS address not "all-ones"?
	jb s	VDLokF		     ;Yes, set VDS "lock" flag.
	movzx	eax,[bx+VDSSg-@].lw  ;VDS logic is NOT present --
	shl	eax,4		     ;  set 20-bit buffer address.
	add	eax,[bx+VDSOf-@]
	mov	[bx+VDSAd-@],eax
VDLokF:	adc	[bx+VLF-@],bl	     ;Set VDS "lock" flag from carry.
VDLokX:	ret			     ;Exit.
;
; Subroutine to test if the current cache buffer can be used for DMA.
;   If so, the cache-buffer address is set for UltraDMA and the carry
;   flag is "off" at exit.   If not, or if the stand-alone driver (no
;   cache) is in use, the carry flag is "on" at exit.
;
UseCB:	mov	cl,[bx+RqCSC-@]	     ;Get current I-O sector count.
	cmp	cl,[bx+RqSec-@]	     ;Is all I-O in one cache block?
@NoCA:	jb s	ErrDMX		     ;No, exit and use regular I-O.
				     ;"jmp s ErrDMX" if no caching).
	or	[bx+RqCmd-@].lb,080h ;Do NOT cache data, after I-O!
	mov	eax,[bx+RqXMS-@]     ;Do DMA thru the cache buffer!
	mov	[bx+IOAdr-@],eax
	ret			     ;Exit.
;
; Subroutine to start and monitor the next UltraDMA I-O transfer.
;
RunDMA:	mov	es:[si+HDI_OFS],bl  ;Reset BIOS disk-interrupt flag.
	mov	dx,[bx+DMAAd-@]	    ;Point to DMA command register.
	in	al,dx		;Set DMA Start/Stop bit (starts DMA).
	inc	ax
	out	dx,al
RunDM1:	inc	dx		;Point to DMA status register.
	inc	dx
	in	ax,dx		;Read DMA controller status.
	dec	dx		;Point back to DMA command register.
	dec	dx
	and	al,DMI+DME	;DMA interrupt or DMA error?
	jnz s	RunDM2		;Yes, halt DMA and check results.
	cmp	ch,es:[si]	;Has our DMA transfer timed out?
	je s	RunDM2		    ;Yes?  Halt DMA & post timeout!
	cmp	es:[si+HDI_OFS],bl  ;Did BIOS get a disk interrupt?
	je s	RunDM1		    ;No, loop back & retest status.
	mov	al,DMI		;Set "simulated" interrupt flag.
RunDM2:	push	ax		;Save ending DMA status.
	in	al,dx		;Reset DMA Start/Stop bit.
	and	al,0FEh
	out	dx,al
	pop	ax		;Reload ending DMA status.
	cmp	al,DMI		;Did DMA end with only an interrupt?
	jne s	RunDMX		;No?  Exit with "non-zero" (error!).
	inc	dx		;Reread DMA controller status.
	inc	dx
	in	al,dx
	test	al,DME		;Any "late" DMA error after DMA end?
RunDMX:	ret			;Exit -- "Non-zero" indicates error.
;
; Subroutine to set the 32-bit DMA address for an UltraDMA transfer.
;   At entry the EAX-reg. has the address from a VDS "lock" request.
;   At exit, if this address was unsuitable for UltraDMA, the driver
;   "Main XMS buffer" address has been set and the carry flag is on.
;
SetAdr:	test	al,003h		      ;Is user buffer 32-bit aligned?
	jnz s	SetAd1		      ;No, use our XMS memory buffer.
	cmp	[bx+VDSAd-@].hw,009h  ;Is DMA beyond our 640K limit?
	ja s	SetAd1		      ;Yes, use our XMS memory buffer.
	mov	cx,[bx+IOLen-@]	  ;Get (byte count - 1) + I-O address.
	dec	cx
	add	cx,ax		  ;Would I-O cross a 64K boundary?
	jnc s	SetAd2		  ;No, set final buffer address.
SetAd1:	stc			  ;Set carry to denote "our buffer".
	mov	eax,0		  ;Get our 32-bit XMS buffer addr.
@XBAddr	equ	[$-4].dwd	  ;(XMS buffer address, Init set).
SetAd2:	mov	[bx+IOAdr-@],eax  ;Set final 32-bit buffer addr.
	ret			  ;Exit.
;
; Subroutine to move our DMA command-list up to XMS memory for I-O.
;   This avoids "DMA Hangs" if we are loaded in UMBPCI "Shadow RAM"!
;
MovCLX:	mov	ecx,8		 ;Set command-list move parameters
	mov	esi,(IOAdr-@)	 ;  and then fall thru to "MvData".
@CLAddr	equ	[$-4].dwd	 ;(Command-list address, Init set).
	mov	edi,[bx+PRDAd-@]
;
; Subroutine to do XMS moves.   At entry, the move parameters are:
;
;   ECX:   Number of bytes to move.   MUST be an even value as only
;	     whole 16-bit words are moved!   Up to a 4-GB value can
;	     be used as data is moved in 2K sections, which permits
;	     interrupts between sections.
;   ESI:   32-bit source data address (NOT segment/offset!).
;   EDI:   32-bit destination address (NOT segment/offset!).
;
; At exit, the carry bit is zero for no errors or is SET for a move
; error.   If so, the AL-reg. has a 0FFh "XMS error" code.   The DS
; and ES-regs. are saved/restored, and the BX-reg. is zero at exit.
; All other registers are NOT saved, for speed.   Only a "protected
; mode" move (JEMM386, etc.) can post an error.   "Real mode" moves
; (UMBPCI, etc.) have NO errors to post!
;
MvData:	mov	ebp,ecx		;Save byte count in EBP-reg.
	sti			;Ensure CPU interrupts are enabled.
	cld			;Ensure FORWARD "string" commands.
XMSMov:	push	ds		;Save DS- and ES-regs.
	push	es
	mov	edx,2048	;Get 2K real-mode section count.
	smsw	ax		;Get CPU "machine status word".
	shr	ax,1		;Are we running in protected-mode?
	jc s	MvProt		;Yes, go use protected-mode logic.
@MvPOfs	equ	[$-1].lb	;(Becomes "jc s MvPNxt" with /N4).
MvRNxt:	mov	ecx,edx		;Set move section count in CX-reg.
	cmp	edx,ebp		;At least 2048 bytes left?
	jbe s	MvRGo		;Yes, use full section count.
	mov	cx,bp		;Use remaining byte count.
MvRGo:	cli			;Disable CPU interrupts during move.
	db	02Eh,00Fh	;"lgdt cs:GDTP", coded the hard-way
	db	001h,016h	;   to avoid any annoying V5.1 MASM
	dw	(GDTP-@)	;   warning messages about it!
@GDTP	equ	[$-2].lw
	mov	eax,cr0		;Set CPU protected-mode control bit.
	or	al,001h
	mov	cr0,eax
	mov	bx,offset (GDT_DS-OurGDT)  ;Set segment "selectors".
	mov	ds,bx
	mov	es,bx
	shr	cx,2		;Convert byte count to dword count.
	db	0F3h,067h	;Move all 32-bit words using "rep"
	movsd			;  and "address-override" prefixes.
	db	067h,090h	;("Override" & "nop" for 386 BUG!).
	adc	cx,cx		;If "odd" 16-bit word, move it also,
	db	0F3h,067h	;  using "rep" & "address-override".
 	movsw
	db	067h,090h	;("Override" & "nop" for 386 BUG!).
	dec	ax		;Clear protected-mode control bit.
	mov	cr0,eax
	sti			;Allow interrupts after next command.
	sub	ebp,edx		;Any more data sections to move?
	ja s	MvRNxt		;Yes, go move next data section.
MvDone:	xor	ax,ax		;Success!  Reset carry and error code.
MvExit:	pop	es		;Reload caller's segment registers.
	pop	ds
	mov	bx,0		;Rezero BX-reg. but save carry bit.
	ret			;Exit.
MvProt:	shl	edx,5		;Protected-mode:  Get 64K section ct.
MvPNxt:	push	edx		;Save move section count.
	push	esi		;Save move source & destination addrs.
	push	edi
	push	cs		;Save driver segment for below.
	mov	ecx,edx		;Set move section count in CX-reg.
	cmp	edx,ebp		;At least one section left?
	jbe s	MvPGo		;Yes, use full section count.
	mov	ecx,ebp		;Use remaining byte count instead.
MvPGo:	shr	ecx,1		;Convert byte count to word count.
	pop	es		;Point ES-register to this driver.
	push	ebp		;Save remaining move byte count.
	push	edi		;Set up destination descriptor.
	mov	di,offset (DstDsc+2-@)
@MvDesc	equ	[$-2].lw
	pop	ax
	stosw
	pop	ax
	stosb
	mov	es:[di+2],ah
	push	esi		;Set up source descriptor ("sub"
	sub	di,11		;  zeros carry for our Int 15h).
	pop	ax
	stosw
	pop	ax
	stosb
	mov	es:[di+2],ah
	lea	si,[di-21]	;Point to start of descriptor table.
	mov	ah,087h		;Have JEMM386/BIOS move next section.
	int	015h
	pop	ebp		;Reload all 32-bit move parameters.
	pop	edi
	pop	esi
	pop	edx
	sti			;Ensure CPU interrupts are enabled.
	cld			;Ensure FORWARD "string" commands.
	mov	al,XMSERR	;Get our XMS error code.
	jc s	MvExit		;If any BIOS error, exit immediately.
	add	esi,edx		;Update source and dest. addresses.
	add	edi,edx
	sub	ebp,edx		;Any more data sections to move?
	ja s	MvPNxt		;Yes, go move next data section.
	jmp s	MvDone		;Done:  Set "success" code and exit.
;
; Subroutine to convert "RedBook" MSF values to an LBA sector number.
;
CvtLBA:	mov	cx,ax		;Save "seconds" & "frames" in CX-reg.
	shr	eax,16		;Get "minute" value.
	cmp	ax,99		;Is "minute" value too large?
	ja s	CvtLBE		;Yes, return -1 error value.
	cmp	ch,60		;Is "second" value too large?
	ja s	CvtLBE		;Yes, return -1 error value.
	cmp	cl,75		;Is "frame" value too large?
	ja s	CvtLBE		;Yes, return -1 error value.
	xor	edx,edx		;Zero EDX-reg. for 32-bit math below.
	mov	dl,60		;Convert "minute" value to "seconds".
	mul	dl		;(Multiply by 60, obviously!).
	mov	dl,ch		;Add in "second" value.
	add	ax,dx
	mov	dl,75		;Convert "second" value to "frames".
	mul	edx		;(Multiply by 75 "frames"/second).
	mov	dl,150		;Subtract offset - "frame".
	sub	dl,cl		;("Adds" frame, "subtracts" offset).
	sub	eax,edx
	ret			;Exit.
CvtLBE:	or	eax,-1		;Too large!  Set -1 error value.
	ret			;Exit.
;
; Main CD/DVD request handler.
;
CDReq:	call	ClrPkt		;Clear our ATAPI packet area.
	mov	[bx+XMF-@],bx	;Reset XMS-move and DMA-input flags.
	les	si,[bx+RqPkt-@]	      ;Reload DOS request-packet ptr.
	mov	es:[si.RPStat],RPDON  ;Set packet status to "done".
	mov	al,es:[si+RPSubU].lb  ;Get our CD/DVD drive number.
	mov	ah,20		      ;Set drive's "audio-start" ptr.
	mul	ah
	add	ax,offset (UTable+8-@)
@UTable	equ	[$-2].lw
	xchg	ax,di
	mov	[bx+AudAP-@],di
	mov	eax,cs:[di-8]	      ;Set drive DMA/IDE addresses.
	mov	[bx+DMAAd-@],eax
	mov	al,es:[si+RPOp]	      ;Get packet request code.
	cmp	al,14		      ;CD/DVD "DOS function" request?
	ja s	Try2D		      ;Yes, do DOS function dispatch.
	je s	CDReqR		      ;"Device Close", ignore & exit.
	cmp	al,3		      ;CD/DVD "IOCTL Input" request?
	je s	Try3D		      ;Yes, do IOCTL Input dispatch.
	cmp	al,12		      ;CD/DVD "IOCTL Output" request?
	jb s	CDReqE		      ;No?  Handle as "unsupported"!
	ja s	CDReqR		      ;"Device Open", ignore & exit.
	mov	di,offset (DspTbl4-@) ;Point to IOCTL Output table.
@DspTB4	equ	[$-2].lw	      ;(Table address, Init set).
	jmp s	TryIOC		      ;Go get actual request code.
Try2D:	sub	al,080h		      ;Not code 0-14:  subtract 128.
	mov	di,offset (DspTbl2-@) ;Point to DOS dispatch table.
@DspTB2	equ	[$-2].lw	      ;(Table address, Init set).
	jmp s	Dispat		      ;Go dispatch on DOS request.
Try3D:	mov	di,offset (DspTbl3-@) ;Point to IOCTL Input table.
@DspTB3	equ	[$-2].lw	      ;(Table address, Init set).
TryIOC:	les	si,es:[si+IOCAdr]     ;Get actual IOCTL request code.
	mov	al,es:[si]
	les	si,[bx+RqPkt-@]	;Reload DOS request-packet address.
Dispat:	cmp	al,cs:[di]	;Is request code out-of-bounds?
	inc	di		;(Skip past table-limit value).
	inc	di
	jae s	CDReqE		;Yes?  Handle as unsupported request.
	xor	ah,ah		;Point to request-handler address.
	shl	ax,1
	add	di,ax
	mov	dx,cs:[di]	;Get handler address from table.
	mov	di,00FFFh
	and	di,dx
	xor	dx,di		;IOCTL request (xfr length > 0)?
	jz s	DspGo		;No, dispatch to desired handler.
	shr	dx,12		   ;Ensure correct IOCTL transfer
	mov	es:[si+IOCLen],dx  ;  length is set in DOS packet.
	les	si,es:[si+IOCAdr]  ;Get IOCTL data-transfer address.
DspGo:	add	di,(USR-@)	   ;Dispatch to desired handler.
@DspOfs	equ	[$-2].lw	   ;(HMA dispatch offset, Init set).
	call	di
	retf			;Return to CD/DVD exit logic.
CDReqE:	call	USR		;Error!  Handle as "unsupported".
CDReqR:	retf			;Return to CD/DVD exit logic.
FFlag	db	0		;(Unused -- Init "Fast cache" flag).
;
; CD/DVD Drive Parameter Tables.
;
UTable	dw	0FFFFh		;Unit 0 DMA address (set by Init).
	dw	0FFFFh		;	IDE address (set by Init).
	dw	0FFFFh		;	(Unused alignment "filler").
	db	0FFh		;	Device-select (set by Init).
	db	0FFh		;	Media-change flag.
	dd	0FFFFFFFFh	;	Current audio-start address.
	dd	0FFFFFFFFh	;	Current audio-end   address.
	dd	0FFFFFFFFh	;	Last-session starting LBA.
	db	140 dup (0FFh)	;Unit 1 to 7 Drive Parameter Tables.
UTblEnd	equ	$		;(End of all CD/DVD drive tables).
;
; Int 15h Descriptor Table, for JEMM386 etc. protected-mode moves.
;
MoveDT	dd	0,0		       ;"Null" descriptor.
	dd	0,0		       ;GDT descriptor.
SrcDsc	dd	00000FFFFh,000009300h  ;Source segment descriptor.
DstDsc	dd	00000FFFFh,000009300h  ;Dest.  segment descriptor.
	dd	0,0		       ;(Used by BIOS).
	dd	0,0		       ;(Used by BIOS).
;
; Global Descriptor Table, for our own real-mode moves.   Its code
;   segment descriptor is a "null" as our XMS move logic for real-
;   mode is all "in line", with no "JMP" or "CALL" commands.   So,
;   the CPU does NOT access the CS-descriptor below!   Many Thanks
;   to Japheth, for confirming this poorly-documented information!
;
OurGDT	dd	0,0		;Null descriptor to start.
	dd	0,0		;Null code descriptor (unaccessed).
GDT_DS	dd	00000FFFFh	;4-GB data descriptor, using pages.
	dd	000CF9300h
GDTLen	equ	($-OurGDT)
;
; Global Descriptor Table Pointer, used by our "lgdt" command.
;
GDTP	dw	GDTLen		;GDT length (always 24 bytes).
GDTPAdr	dd	(OurGDT-@)	;GDT 32-bit address (Set By Init).
;
; Hard Disk and Diskette Parameter Tables.
;
TypeF	db	MAXBIOS dup (0)	;Device type and "sub-unit":
				;  00h-27h:  SATA/IDE UltraDMA disk.
				;  80h:      BIOS-handled disk.
				;  C0h:      BIOS-handled diskette.
CHSec	db	MAXBIOS dup (0)	;Sectors-per-head table.
CHSHd	db	MAXBIOS dup (0)	;Heads-per-cylinder table.
;
; Hard Disk Controller I-O Address Table.
;
Ctl1Tbl	dw	0FFFFh		;Controller 1 DMA base address.
Ctl1Pri	dw	NPDATA		;   "Legacy" primary   addresses.
Ctl1Sec	dw	NSDATA		;   "Legacy" secondary addresses.
Ctl2Tbl	dw	0FFFFh		;Controller 2 DMA base address.
Ctl2Pri	dw	APDATA		;   "Legacy" primary   addresses.
Ctl2Sec	dw	ASDATA		;   "Legacy" secondary addresses.
	dw	24 dup (0FFFFh)	;Controller 3 to 10 I-O addresses.
CtlTEnd	equ	$		   ;End of controller I-O tables.
CTLTSIZ	equ	(Ctl2Tbl-Ctl1Tbl)  ;Size of a controller I-O table.
;
; CD/DVD Dispatch Table for DOS request codes 128 thru 136.   Each
;   entry in the following tables has 4 bits of "IOCTL byte count"
;   and 12 bits of offset for its "handler" routine, so "handlers"
;   MUST be located within 4K bytes from the start of this driver!
;
DspTbl2	dw	DspLmt2		;Number of valid request codes.
DspTblB	dw	RqRead-USR	;128 -- Read Long.
	dw	0		;129 -- Reserved	(unused).
	dw	RqSeek-USR	;130 -- Read Long Prefetch.
	dw	RqSeek-USR	;131 -- Seek.
	dw	RqPlay-USR	;132 -- Play Audio.
	dw	RqStop-USR	;133 -- Stop Audio.
	dw	0		;134 -- Write Long	(unused).
	dw	0		;135 -- Wr. Long Verify	(unused).
	dw	RqRsum-USR	;136 -- Resume Audio.
DspLmt2	equ	($-DspTblB)/2	;Table request-code limit.
;
; CD/DVD Dispatch table for IOCTL Input requests.
;
DspTbl3	dw	DspLmt3		    ;Number of valid request codes.
DspTblC	dw	ReqDHA+(5*IXM)-USR  ;00 -- Device-header address.
	dw	RqHLoc+(6*IXM)-USR  ;01 -- Current head location.
	dw	0		    ;02 -- Reserved	    (unused).
	dw	0		    ;03 -- Error Statistics (unused).
	dw	0		    ;04 -- Audio chan. info (unused).
	dw	0		    ;05 -- Read drive bytes (unused).
	dw	ReqDS +(5*IXM)-USR  ;06 -- Device status.
	dw	ReqSS +(4*IXM)-USR  ;07 -- Sector size.
	dw	ReqVS +(5*IXM)-USR  ;08 -- Volume size.
	dw	ReqMCS+(2*IXM)-USR  ;09 -- Media-change status.
	dw	ReqADI+(7*IXM)-USR  ;10 -- Audio disk info.
	dw	ReqATI+(7*IXM)-USR  ;11 -- Audio track info.
	dw	ReqAQI+(11*IXM)-USR ;12 -- Audio Q-channel info.
	dw	0		    ;13 -- Subchannel info  (unused).
	dw	0		    ;14 -- Read UPC code    (unused).
	dw	ReqASI+(11*IXM)-USR ;15 -- Audio status info.
DspLmt3	equ	($-DspTblC)/2	    ;Table request-code limit.
;
; CD/DVD Dispatch table for IOCTL Output requests.
;
DspTbl4	dw	DspLmt4		    ;Number of valid request codes.
DspTblD	dw	ReqEJ +(1*IXM)-USR  ;00 -- Eject Disk.
	dw	ReqLU +(2*IXM)-USR  ;01 -- Lock/Unlock Door.
	dw	ReqRS +(1*IXM)-USR  ;02 -- Reset drive.
	dw	0		    ;03 -- Audio control    (unused).
	dw	0		    ;04 -- Write ctl. bytes (unused).
	dw	ReqCl +(1*IXM)-USR  ;05 -- Close tray.
DspLmt4	equ	($-DspTblD)/2	    ;Table request-code limit.
;
; CD/DVD Error Routines.
;
USR:	mov	al,3		;Unsupported request!  Get error code.
ReqErr:	les	si,[bx+RqPkt-@]	;Reload DOS request-packet address.
	mov	ah,081h		;Post error flags & code in packet.
	mov	es:[si+RPStat],ax
RqExit:	ret			;Exit ("ignored" request handler).
;
; CD/DVD subroutine to validate starting RedBook disk sector numbers.
;
ValSN:	mov	eax,es:[si+RLSec]  ;Get starting sector number.
ValSN1:	mov	dl,es:[si+RLAM]	;Get desired addressing mode.
	cmp	dl,001h		;HSG or RedBook addressing?
	ja s	ValSNE		;No?  Return "sector not found".
	jne s	RqExit		;HSG -- exit (accept any DVD value).
	call	CvtLBA		;RedBook -- get starting sector.
	cmp	eax,RMAXLBA	;Is starting sector too big?
	jbe s	RqExit		;No, all is well -- go exit above.
ValSNE:	pop	ax		;Error!  Discard our exit address.
SectNF:	mov	al,8		;Sector not found!  Get error code.
	jmp s	ReqErr		;Go post "sector not found" and exit.
;
; CD/DVD "Read Long" request handler.   All CD/DVD data is read here.
;
RqRead:	call	ValSN		;Validate starting sector number.
	call	MultiS		;Handle Multi-Session disk if needed.
	jc s	ReqErr		;If error, post return code & exit.
	mov	cx,es:[si+RLSC]	;Get request sector count.
	jcxz	RqExit		;If zero, simply exit.
	xchg	cl,ch		;Save swapped sector count.
	mov	[bx+PktLn-@],cx
	cmp	es:[si.RLDM],1	;"Cooked" or "raw" read mode?
	ja s	SectNF		;Neither?  Return "sector not found"!
	mov	dl,028h		;Get "cooked" input values.
	mov	ax,COOKSL
	jb s	RqRL1		;If "cooked" input, set values.
	mov	dl,0BEh		;Get "raw" input values.
	mov	ax,RAWSL
	mov	[bx+PktRM-@].lb,0F8h  ;Set "raw" input flags.
RqRL1:	mov	[bx+Packet-@].lb,dl   ;Set "packet" opcode.
	mul	es:[si+RLSC]	      ;Get desired input byte count.
	test	dx,dx		      ;More than 65535 bytes desired?
	jnz s	SectNF		      ;Yes?  Return sector not found!
	mov	[bx+VDSLn-@],ax	      ;Set 32-bit VDS buffer length.
	mov	[bx+VDSLn+2-@],bx
	mov	[bx+IOLen-@],ax	      ;Set 24-bit DMA buffer length.
	mov	[bx+IOLen+2-@],bl     ;(Save DMA "end of list" byte).
	les	ax,es:[si+RLAddr]     ;Set user input-buffer address.
	mov	[bx+VDSOf-@],ax
	mov	[bx+VDSSg-@],es
	mov	[bx+UserB+2-@],es
	call	VDLock		      ;Do VDS "lock" on user buffer.
	les	si,[bx+RqPkt-@]	      ;Reload request packet address.
	jc s	RqRL4		      ;"Lock" error?  Use "PIO mode"!
	test	[bx+DMAAd-@].lb,001h  ;Is this drive using UltraDMA?
	jnz s	RqRL2		      ;No, check for "raw mode" read.
	call	SetAdr		      ;Set 32-bit DMA buffer address.
@NoXM1:	adc	[bx+XMF-@].lw,00100h  ;Set XMS-move & DMA-input flags.
RqRL2:	cmp	[bx+PktRM-@],bl	      ;Is this a "raw mode" read?
@NoCB:	jne s	RqRL5		      ;Yes, use "non-cached" logic.
				      ;("jmp s RqRL5" if no caching).
	mov	[bx+RqTyp-@].lb,CDTYP ;Set CD/DVD device type code.
	movzx	cx,es:[si+RLSC].lb    ;Get sector count & read code.
	shl	cl,2		      ;Multiply by 4 for 2K sectors.
	movzx	bp,es:[si+RPSubU].lb  ;Get our CD/DVD drive number.
	add	bp,CDUNIT	      ;Convert to our "cache unit".
	mov	eax,[bx+PktLBA-@]   ;Convert packet LBA to "little
	call	Swp32		    ;  endian" format for caching.
	shl	eax,2		    ;Multiply by 4 for 2K sectors.
	push	eax		    ;Put 32-bit LBA in DX/SI-regs.
	pop	si		    ;  and zero DI-reg. (hi-order)
	pop	dx		    ;  as CD/DVD drives do not use
	xor	di,di		    ;  48-bit LBAs!
	mov	eax,[bx+VDSAd-@]    ;Get 32-bit input buffer addr.
	cli			    ;Disable interrupts, so we exit
				    ;  I-O with CPU interrupts OFF!
	pushf			    ;Save CPU flags and call "RqEntr"
	push	cs		    ;  for input -- with flags saved,
	call	RqEntr		    ;  the call acts like an Int 13h!
	mov	al,ah		    ;Move any error code into AL-reg.
	lahf			    ;Save carry flag (CD/DVD error).
	or	[bx+IOF-@].lb,080h  ;Post driver "busy" flag again.
	sahf			    ;Reload carry flag (CD error).
	sti			;NOW safe to re-enable interrupts!
	jc s	RqRL6		;Any CD/DVD errors during input?
RqRL3:	ret			;No, all is well -- exit.
RqRL4:	or	[bx+IOF-@].lb,2	;VDS error!  Post "flush cache" flag.
RqRL5:	call	RLRead		;Non-cached:  Input all desired data.
	jnc s	RqRL3		;If no errors, go exit above.
RqRL6:	cmp	al,0FFh		;Did we get an "XMS memory" error?
	jne s	RqRL7		;No, set returned error code as-is.
	mov	al,GENERR	;Use CD/DVD "general error" code.
RqRL7:	jmp	ReqErr		;Set error code in packet & exit.
;
; Subroutine to initialize the next UltraDMA I-O transfer.
;   This subroutine is place here for alignment.
;
IniDMA:	mov	dx,[bx+DMAAd-@]	;Get DMA command-register address.
	mov	si,(PRDAd-@)	;Get Physical-Region Descriptor addr.
	out	dx,al		;Reset DMA commands and set DMA mode.
	inc	dx		;Point to DMA status register.
	inc	dx
	in	al,dx		;Reset DMA status register.
	or	al,006h		;(Done this way so we do NOT alter
	out	dx,al		;  the "DMA capable" status flags!).
	inc	dx		;Point to PRD address register.
	inc	dx
	outsd			;Set PRD pointer to our DMA address.
	ret			;Exit.
;
; Subroutine to enter our hard-disk logic for cached CD/DVD input.
;
RqEntr:	pushf			;Save CPU flags and BP-reg. same as
	push	bp		;  an "Int 13h" I-O request will do.
	db	0EAh		;Go switch to UIDE stack & do input.
	dw	(Switch-@)
@RqESeg	dw	0		;(Driver segment address, Init set).
EFlag	db	0		;(Unused -- Init "emulator" flag).
;
; CD/DVD "Seek" request handler.
;
RqSeek:	call	RdAST1		;Read current "audio" status.
	call	ClrPkt		;Reset our ATAPI packet area.
	jc s	RqSK2		;If status error, do DOS seek.
	mov	al,[di+1]	;Get "audio" status flag.
	cmp	al,011h		;Is drive in "play audio" mode?
	je s	RqSK1		;Yes, validate seek address.
	cmp	al,012h		;Is drive in "pause" mode?
	jne s	RqSK2		;No, do DOS seek below.
RqSK1:	call	ValSN		;Validate desired seek address.
	mov	di,[bx+AudAP-@]	;Point to audio-start address.
	cmp	eax,cs:[di+4]	;Is address past "play" area?
	ja s	RqSK2		;Yes, do DOS seek below.
	mov	cs:[di],eax	;Update audio-start address.
	call	PlayA		;Issue "Play Audio" command.
	jc s	RqPLE		;If error, post return code & exit.
	cmp	[di+1].lb,011h	;Were we playing audio before?
	je s	RqPLX		;Yes, post "busy" status and exit.
	call	ClrPkt		;Reset our ATAPI packet area.
	jmp s	RqStop		;Go put drive back in "pause" mode.
RqSK2:	call	ValSN		;Validate desired seek address.
	call	MultiS		;Handle Multi-Session disk if needed.
	jc s	RqPLE		;If error, post return code & exit.
	mov	[bx+Packet-@].lb,02Bh  ;Set "seek" command code.
RqSK3:	call	DoCmd		;Issue desired command to drive.
	jc s	RqPLE		;If error, post return code & exit.
RqSKX:	ret			;Exit.
;
; CD/DVD "Play Audio" request handler.
;
RqPlay:	cmp	es:[si.RLSC].dwd,0  ;Is sector count zero?
	je s	RqSKX		    ;Yes, just exit above.
	mov	eax,es:[si+RLAddr]  ;Validate audio-start address.
	call	ValSN1
	mov	di,[bx+AudAP-@]	;Save drive's audio-start address.
	mov	cs:[di],eax
	add	eax,es:[si+18]	;Calculate audio-end address.
	mov	edx,RMAXLBA	;Get maximum audio address.
	jc s	RqPL1		;If "end" WAY too big, use max.
	cmp	eax,edx		;Is "end" address past maximum?
	jbe s	RqPL2		;No, use "end" address as-is.
RqPL1:	xchg	eax,edx		;Set "end" address to maximum.
RqPL2:	mov	cs:[di+4],eax	;Save drive's audio-end address.
	call	PlayA		;Issue "Play Audio" command.
RqPLE:	jc s	RqDS0		;If error, post return code & exit.
RqPLX:	jmp	RdAST4		;Go post "busy" status and exit.
;
; CD/DVD "Stop Audio" request handler.
;
RqStop:	mov	[bx+Packet-@].lb,04Bh ;Set "Pause/Resume" command.
	jmp	DoCmd		      ;Go pause "audio" and exit.
;
; CD/DVD "Resume Audio" request handler.
;
RqRsum:	inc	[bx+PktLn+1-@].lb  ;Set "Resume" flag for above.
	call	RqStop		;Issue "Pause/Resume" command.
	jmp s	RqPLE		;Go exit through "RqPlay" above.
;
; CD/DVD IOCTL Input "Device Header Address" handler.
;
ReqDHA:	push	ds		;Return our base driver address.
	push	bx
	pop	es:[si+1].dwd
	ret			;Exit.
;
; CD/DVD IOCTL Input "Current Head Location" handler.
;
RqHLoc:	mov	[bx+Packet-@].dwd,001400042h   ;Set command bytes.
	mov	al,16		;Set input byte count of 16.
	call	RdAST3		;Issue "Read Subchannel" request.
	jc s	RqDS0		;If error, post return code & exit.
	mov	es:[si+1],bl	;Return "HSG" addressing mode.
	call	SwpLBA		;Return "swapped" head location.
	mov	es:[si+2],eax
	jmp s	RqVSX		;Go post "busy" status and exit.
;
; CD/DVD IOCTL Input "Device Status" handler.
;
ReqDS:	mov	[bx+Packet-@].dwd,0002A005Ah  ;Set up mode-sense.
	mov	al,16		;Use input byte count of 16.
	call	DoBuf1		;Issue mode-sense for hardware data.
RqDS0:	jc s	RqVSE		;If error, post return code & exit.
	mov	eax,00214h	;Get our basic driver status flags.
	mov	cl,070h		;Get media status byte.
	xor	cl,[di+2]
	shr	cl,1		;Door open, or closed with no disk?
	jnz s	RqDS1		;No, check "drive locked" status.
	adc	ax,00800h	;Post "door open" & "no disk" flags.
RqDS1:	test	[di+14].lb,002h	;Drive pushbutton "locked out"?
	jnz s	RqVSS		;No, set flags in IOCTL and exit.
	or	al,002h		;Set "door locked" status flag.
	jmp s	RqVSS		;Go set flags in IOCTL and exit.
;
; CD/DVD IOCTL Input "Sector Size" handler.
;
ReqSS:	cmp	es:[si+1].lb,1	;Is read mode "cooked" or "raw"?
	ja	RqGenE		;Neither -- Post "general error".
	mov	ax,RAWSL	;Get "raw" sector length.
	je s	RqSS1		;If "raw" mode, set sector length.
	mov	ax,COOKSL	;Get "cooked" sector length.
RqSS1:	mov	es:[si+2],ax	;Post sector length in IOCTL packet.
	ret			;Exit.
;
; CD/DVD IOCTL Input "Volume Size" handler.
;
ReqVS:	mov	[bx+Packet-@].lb,025h  ;Set "Read Capacity" code.
	mov	al,008h		;Get 8 byte data-transfer length.
	call	DoBuf2		;Issue "Read Capacity" command.
RqVSE:	jc s	RqAQIE		;If error, post return code & exit.
	mov	eax,[di]	;Get and "swap" volume size.
	call	Swp32
RqVSS:	mov	es:[si+1],eax	;Set desired IOCTL data value.
RqVSX:	jmp s	RqATIX		;Go post "busy" status and exit.
;
; CD/DVD IOCTL Input "Media-Change Status" handler.
;
ReqMCS:	call	DoCmd		;Issue "Test Unit Ready" command.
	mov	di,[bx+AudAP-@]	;Get media-change flag from table.
	mov	al,cs:[di-1]
	mov	es:[si+1],al	;Return media-change flag to user.
	ret			;Exit.
;
; CD/DVD IOCTL Input "Audio Disk Info" handler.
;
ReqADI:	mov	al,0AAh		;Specify "lead-out" session number.
	call	RdTOC		;Read disk table-of-contents (TOC).
	mov	es:[si+3],eax	;Set "lead out" LBA addr. in IOCTL.
	mov	ax,[di+2]	;Set first & last tracks in IOCTL.
	mov	es:[si+1],ax
	jmp s	RqATIX		;Go post "busy" status and exit.
;
; CD/DVD IOCTL Input "Audio Track Info" handler.
;
ReqATI:	mov	al,es:[si+1]	;Specify desired session (track) no.
	call	RdTOC		;Read disk table-of-contents (TOC).
	mov	es:[si+2],eax	;Set track LBA address in IOCTL.
	mov	al,[di+5]
	shl	al,4
	mov	es:[si+6],al
RqATIX:	jmp	RdAST		;Go post "busy" status and exit.
;
; CD/DVD IOCTL Input "Audio Q-Channel Info" handler.
;
ReqAQI:	mov	ax,04010h	;Set "data in", use 16-byte count.
	call	RdAST2		;Read current "audio" status.
RqAQIE:	jc s	RqASIE		;If error, post return code & exit.
	mov	eax,[di+5]	;Get ctrl/track/index in EAX-reg.
        mov     ch,al		;Save control byte in CH-reg.
        movzx	ax,ah		;Convert track number from binary
        mov     cl,10		;  to BCD.    Thanks to Nagatoshi
        div     cl		;  Uehara from Japan, for finding
        shl     al,4		;  that this is necessary!
        add     ah,al
        mov     al,ch		;Restore control byte into AL-reg.
	mov	es:[si+1],eax	;Set ctrl/track/index in IOCTL.
	mov	eax,[di+13]	;Set time-on-track in IOCTL.
	mov	es:[si+4],eax
	mov	edx,[di+9]	;Get time-on-disk & clear high
	shl	edx,8		;  order time-on-track in IOCTL.
	jmp s	RqASI4		;Go set value in IOCTL and exit.
;
; CD/DVD IOCTL Input "Audio Status Info" handler.
;
ReqASI:	mov	ax,04010h	;Set "data in", use 16-byte count.
	call	RdAST2		;Read current "audio" status.
RqASIE:	jc s	RqErrJ		;If error, post return code & exit.
	mov	es:[si+1],bx	;Reset audio "paused" flag.
	xor	eax,eax		;Reset starting audio address.
	xor	edx,edx		;Reset ending audio address.
	cmp	[di+1].lb,011h  ;Is drive now "playing" audio?
	jne s	RqASI1		;No, check for audio "pause".
	mov	di,[bx+AudAP-@]	;Point to drive's audio data.
	mov	eax,cs:[di]	;Get current audio "start" addr.
	jmp s	RqASI2		;Go get current audio "end" addr.
RqASI1:	cmp	[di+1].lb,012h	;Is drive now in audio "pause"?
	jne s	RqASI3		;No, return "null" addresses.
	inc	es:[si+1].lb	;Set audio "paused" flag.
	call	SwpLBA		;Convert time-on-disk to LBA addr.
	call	CvtLBA
	mov	di,[bx+AudAP-@]	;Point to drive's audio data.
RqASI2:	mov	edx,cs:[di+4]	;Get current audio "end" address.
RqASI3:	mov	es:[si+3],eax	;Set audio "start" addr. in IOCTL.
RqASI4:	mov	es:[si+7],edx	;Set audio "end" address in IOCTL.
	ret			;Exit.
;
; CD/DVD IOCTL Output "Eject Disk" handler.
;
ReqEJ:	mov	[bx+Packet-@].lw,0011Bh  ;Set "eject" commands.
	mov	[bx+PktLBA+2-@].lb,002h  ;Set "eject" function.
	jmp	RqSK3			 ;Go do "eject" & exit.
;
; CD/DVD IOCTL Output "Lock/Unlock Door" handler.
;
ReqLU:	mov	al,es:[si+1]	;Get "lock" or "unlock" function.
	cmp	al,001h		;Is function byte too big?
	ja s	RqGenE		;Yes, post "general error" & exit.
	mov	cx,0001Eh	    ;Get "lock" & "unlock" commands.
RqLU1:	mov	[bx+Packet-@],cx    ;Set "packet" command bytes.
	mov	[bx+PktLBA+2-@],al  ;Set "packet" function byte.
	call	DoCmd		    ;Issue desired command to drive.
	jc s	RqErrJ		;If error, post return code & exit.
	jmp s	RdAST		;Go post "busy" status and exit.
;
; CD/DVD IOCTL Output "Reset Drive" handler.
;
ReqRS:	call	HltDMA		;Stop previous DMA & select drive.
	inc	dx		;Point to IDE command register.
	mov	al,008h		;Do an ATAPI "soft reset" command.
	out	dx,al
	call	ChkTO		;Await controller-ready.
	jnc s	SwpLBX		;If no timeout, go exit below.
RqGenE:	mov	al,GENERR	;General error!  Get return code.
RqErrJ:	jmp	ReqErr		;Go post error return code and exit.
UseXMS	db	0FFh		;(Unused -- Init "Use XMS" flag).
;
; CD/DVD IOCTL Output "Close Tray" handler.
;
ReqCL:	mov	al,003h		;Get "close tray" function byte.
	mov	cx,0011Bh	;Get "eject" & "close" commands.
	jmp s	RqLU1		;Go do "close tray" command above.
;
; CD/DVD subroutine to read disk "Table of Contents" (TOC) values.
;
RdTOCE:	pop	cx		;Error -- Discard exit address.
	jmp s	RqErrJ		;Go post return code and exit.
RdTOC:	mov	[bx+Packet-@].lw,00243h  ;Set TOC and MSF bytes.
	call	DoTOC1		;Issue "Read Table of Contents" cmd.
	jc s	RdTOCE		;If error, exit immediately.
;
; CD/DVD subroutine to "swap" the 4 bytes of a a 32-bit value.
;
SwpLBA:	mov	eax,[di+8]	;Get audio-end or buffer LBA value.
Swp32:	xchg	al,ah		;"Swap" original low-order bytes.
	rol	eax,16		;"Exchange" low- and high-order.
	xchg	al,ah		;"Swap" ending low-order bytes.
SwpLBX:	ret			;Exit.
;
; CD/DVD subroutine to read current "audio" status & disk addresses.
;
RdAST:	call	ClrPkt		  ;Status only -- reset ATAPI packet.
RdAST1:	mov	ax,00004h	  ;Clear "data in", use 4-byte count.
RdAST2:	mov	[bx+Packet-@].dwd,001000242h  ;Set command bytes.
	mov	[bx+PktLBA-@],ah  ;Set "data in" flag (RdAST2 only).
RdAST3:	call	DoBuf1		  ;Issue "Read Subchannel" command.
	jc s	RdASTX		  ;If error, exit immediately.
	cmp	[di+1].lb,011h	  ;Is a "play audio" in progress?
	clc			  ;(Zero carry flag in any case).
	jne s	RdASTX		  ;No, just exit below.
RdAST4:	push	si		  ;Save SI- and ES-regs.
	push	es
	les	si,[bx+RqPkt-@]	  ;Reload DOS request-packet addr.
	or	es:[si.RPStat],RPBUSY  ;Set "busy" status bit.
	pop	es		  ;Reload ES- and SI-regs.
	pop	si
RdASTX:	ret			  ;Exit.
;
; Subroutine to handle CD/DVD Multi-Session disks for reads & seeks.
;   Multi-Session disks require (A) saving the last-session starting
;   LBA for a new disk after any media-change and (B) "offsetting" a
;   read of the VTOC or initial directory block, sector 16 or 17, to
;   access the VTOC/directory of the disk's last session.
;
MultiS:	mov	di,[bx+AudAP-@]		;Point to drive variables.
	cmp	cs:[di+11].lb,0FFh	;Is last-session LBA valid?
	jne s	MultS1			;Yes, proceed with request.
	mov	[bx+Packet-@].lb,043h	;Set "Read TOC" command.
	inc	[bx+PktLBA-@].lb	;Set "format 1" request.
	call	DoTOC2			;Read first & last session.
	jc s	MultSX			;If any error, exit below.
	mov	[bx+PktLBA-@],bl	;Reset "format 1" request.
	mov	al,[di+3]		;Get last-session number.
	call	DoTOC1		;Read disk info for last session.
	jc s	MultSX		;If error, exit with carry set.
	call	SwpLBA		;"Swap" & save last-session LBA addr.
	mov	di,[bx+AudAP-@]
	mov	cs:[di+8],eax
	call	ClrPkt		   ;Reset our ATAPI packet area.
MultS1:	mov	eax,es:[si+RLSec]  ;Get starting sector number.
	mov	dx,ax		   ;Save low-order sector number.
	and	al,0FEh		;"Mask" sector to an even number.
	cmp	eax,16		;Sector 16 (VTOC) or 17 (directory)?
	xchg	ax,dx		;(Restore low-order sector number).
	jne s	MultS2		;No, set sector in packet.
	add	eax,cs:[di+8]	;Offset sector to last-session start.
MultS2:	call	Swp32		;"Swap" sector into packet as LBA.
	mov	[bx+PktLBA-@],eax
	clc			;Clear carry flag (no errors).
MultSX:	ret			;Exit.
;
; Subroutine to ensure CD/DVD UltraDMA is stopped and then select our
;   drive.   For some older chipsets, if UltraDMA is running, reading
;   an IDE register causes the chipset to "HANG"!!
;
HltDMA:	mov	dx,[bx+DMAAd-@]	;Get drive UltraDMA command address.
	test	dl,001h		;Is this drive using UltraDMA?
	jnz s	HltDM1		;No, just select "master" or "slave".
	in	al,dx		;Ensure any previous DMA is stopped!
	and	al,0FEh
	out	dx,al
HltDM1:	mov	dx,[bx+IdeDA-@]	;Point to IDE device-select register.
	add	dx,6
	mov	di,[bx+AudAP-@]	;Get drive's IDE device-select byte.
	mov	al,cs:[di-2]
	out	dx,al		;Select IDE "master" or "slave" unit.
	ret			;Exit.
;
; Subroutine to read all data for a CD/DVD "Read Long" request.
;
RLRead:	cmp	[bx+DMF-@],bl	  ;Will this input be using DMA?
	je s	RlRdA		  ;No, go do input using "PIO mode".
	call	UseCB		  ;Can current cache buffer be used?
	cmc			  ;If so, set XMS-move flag for below.
	adc	[bx+XMF-@],bl
	call	MovCLX		  ;Move DMA command-list up to XMS.
	jc s	RLRdE		  ;Failed?  Return "general error"!
RlRdA:	call	DoIO		  ;Do desired UltraDMA input request.
	jc s	RLRdX		  ;If any errors, exit below.
	or	[bx+XMF-@],bl	  ;Do we need an XMS buffer "move"?
	jz s	RLRdX		  ;No, just exit below.
	mov	ecx,[bx+VDSLn-@]  ;Get parameters for an XMS move.
	mov	esi,[bx+IOAdr-@]
	mov	edi,[bx+VDSAd-@]
	call	MvData		  ;Move data from XMS to user buffer.
RLRdE:	mov	al,GENERR	  ;Use "general error" code if error.
RLRdX:	ret			  ;Exit.
;
; Subroutine to issue a CD/DVD "Play Audio" command.   At entry, the
;   DI-reg. points to the audio-start address for this drive.
;
PlayA:	mov	eax,cs:[di]	;Set "packet" audio-start address.
	call	CvtMSF
	mov	[bx+PktLBA+1-@],eax
	mov	eax,cs:[di+4]	;Set "packet" audio-end address.
	call	CvtMSF
	mov	[bx+PktLH-@],eax
	mov	[bx+Packet-@].lb,047h  ;Set "Play Audio" command.
	jmp s	DoCmd		       ;Start playing audio & exit.
;
; Ye Olde CD/DVD I-O Subroutine.   All CD/DVD I-O gets executed here!
;
DoTOC1:	mov	[bx+PktLH-@],al	   ;"TOC" -- set packet session no.
DoTOC2:	mov	al,12		   ;Use 12-byte "TOC" alloc. count.
	nop			   ;(Unused alignment "filler").
DoBuf1:	mov	[bx+PktLn+1-@],al  ;Buffered -- set packet count.
DoBuf2:	mov	[bx+VDSLn-@],al	   ;Set 8-bit data-transfer length.
	mov	[bx+VDSOf-@].lw,(InBuf-@)  ;Use our buffer for I-O.
	mov	[bx+UserB+2-@],ds
	jmp s	DoCmd1		   ;Go zero hi-order and start I-O.
DoCmd:	mov	[bx+VDSLn-@],bl    ;Command only -- zero xfr length.
DoCmd1:	mov	[bx+VDSLn+1-@],bx  ;Zero hi-order VDS buffer length.
DoIO:	push	si		   ;Save SI- and ES-registers.
	push	es
	mov	[bx+Try-@].lb,4	;Set request retry count of 4.
DoIO1:	call	HltDMA		;Stop previous DMA & select drive.
	call	ChkTO		;Await controller-ready.
	jc s	DoIO4		;Timeout!  Handle as a "hard error".
	cmp	[bx+DMF-@],bl	;UltraDMA input request?
	je s	DoIO2		;No, output ATAPI "packet" command.
	mov	al,008h		;Get DMA-input command bit.
	call	IniDMA		;Initialize our DMA input transfer.
DoIO2:	mov	ax,[bx+VDSOf-@]	;Reset data-transfer buffer address.
	mov	[bx+UserB-@],ax
	mov	ax,[bx+VDSLn-@]	;Reset data-transfer byte count.
	mov	[bx+UserL-@],ax
	mov	dx,[bx+IdeDA-@]	;Point to IDE "features" register.
	inc	dx
	mov	al,[bx+DMF-@]	;If UltraDMA input, set "DMA" flag.
	out	dx,al
	add	dx,3		;Point to byte count registers.
	mov	si,(UserL-@)	;Output data-transfer length.
	outsb
	inc	dx
	outsb
	inc	dx		;Point to command register.
	inc	dx
	mov	al,0A0h		;Issue "Packet" command.
	out	dx,al
	mov	cl,DRQ		;Await controller- and data-ready.
	call	ChkTO1
	jc s	DoIO4		;Timeout!  Handle as a "hard error".
	xchg	ax,si		;Save BIOS timer address.
	mov	dx,[bx+IdeDA-@]	;Point to IDE data register.
	mov	cl,6		;Output all 12 "Packet" bytes.
	mov	si,(Packet-@)
	rep	outsw
	xchg	ax,si		;Reload BIOS timer address.
	mov	ah,STARTTO	;Allow 7 seconds for drive startup.
	cmp	[bx+DMF-@],bl	;UltraDMA input request?
	je s	DoIO12		;No, start "PIO mode" transfer below.
	mov	[bx+UserL-@],bx	;Reset transfer length (DMA does it).
	mov	ch,ah		;Set 7-second timeout in AH-reg.
	add	ch,es:[si]
	call	RunDMA		;Start and monitor next DMA transfer.
	jnz s	DoIO4		;Problems?  Handle as a "hard error"!
	call	ChkTO		;Await final controller-ready.
	jc s	DoIO4		;Timeout!  Handle as a "hard error"!
DoIO3:	mov	si,[bx+AudAP-@]	;Get drive media-change flag pointer.
	dec	si
	and	ax,00001h	;Did controller detect any errors?
	jz s	DoIO6		;No, see if all data was transferred.
	sub	dx,6		;Get controller's sense key value.
	in	al,dx
	shr	al,4
	cmp	al,006h		;Is sense key "Unit Attention"?
	je s	DoIO7		;Yes, check for prior media-change.
	mov	ah,0FFh		;Get 0FFh flag for "Not Ready".
	cmp	al,002h		;Is sense key "Drive Not Ready"?
	je s	DoIO8		;Yes, go set our media-change flag.
DoIO4:	mov	dx,[bx+IdeDA-@]	;Hard error!  Point to command reg.
	add	dx,7
	mov	al,008h		;Issue ATAPI "soft reset" to drive.
	out	dx,al
	mov	al,11		;Get "hard error" return code.
	dec	[bx+Try-@].lb	;Do we have more I-O retries left?
	jz s	DoIO9		;No, set carry & return error code.
	jmp	DoIO1		;Try re-executing this I-O request.
DoIO6:	cmp	[bx+UserL-@],bx	;Was all desired data input?
	jne s	DoIO4		;No?  Handle as a "hard error".
	mov	cs:[si].lb,001h	;Set "no media change" flag.
	jmp s	DoIO10		;Go reload regs. and exit below.
DoIO7:	mov	al,002h		;"Attention":  Get "Not Ready" code.
DoIO8:	xchg	ah,cs:[si]	;Load & set our media-change flag.
	or	[bx+IOF-@].lb,2	    ;Flush cache on next caching req.
	mov	cs:[si+12].lb,0FFh  ;Make last-session LBA invalid.
	dec	ah		    ;Media-change flag already set?
	jnz s	DoIO9		;Yes, set carry flag and exit.
	mov	al,15		;Return "Invalid Media Change".
DoIO9:	stc			;Set carry flag (error!).
DoIO10:	pop	es		;Reload ES- and SI-registers.
	pop	si
	mov	di,(InBuf-@)	;For audio, point to our buffer.
	ret			;Exit.
DoIO11:	mov	ah,SEEKTO	;"PIO mode" -- Get "seek" timeout.
DoIO12:	call	ChkTO2		;Await controller-ready.
	jc s	DoIO4		;Timeout!  Handle as a "hard error".
	test	al,DRQ		;Did we also get a data-request?
	jz s	DoIO3		;No, go check for any input errors.
	dec	dx		;Get controller-buffer byte count.
	dec	dx
	in	al,dx
	mov	ah,al
	dec	dx
	in	al,dx
	mov	dx,[bx+IdeDA-@]	;Point to IDE data register.
	mov	si,[bx+UserL-@]	;Get our data-transfer length.
	or	si,si		;Any remaining bytes to input?
	jz s	DoIO14		;No, "eat" all residual data.
	cmp	si,ax		;Remaining bytes > buffer count?
	jbe s	DoIO13		;No, input all remaining bytes.
	mov	si,ax		;Use buffer count as input count.
DoIO13:	les	di,[bx+UserB-@]	;Get input data-transfer address.
	mov	cx,si		;Input all 16-bit data words.
	shr	cx,1
	rep	insw
	add	[bx+UserB-@],si	;Increment data-transfer address.
	sub	[bx+UserL-@],si	;Decrement data-transfer length.
	sub	ax,si		;Any data left in controller buffer?
	jz s	DoIO11		;No, await next controller-ready.
DoIO14:	xchg	ax,cx		;"Eat" all residual input data.
	shr	cx,1		;(Should be NO residual data as we
DoIO15:	in	ax,dx		;  always set an exact byte count.
	loop	DoIO15		;  This logic is only to be SAFE!).
	jmp s	DoIO11		;Go await next controller-ready.
;
; Subroutine to test for CD/DVD I-O timeouts.   At entry the CL-reg.
;   is 008h to test for a data-request, also.   At exit, the DX-reg.
;   points to the drive's IDE primary-status reg.   The AH-, SI- and
;   ES-regs. will be lost.
;
ChkTO:	xor	cx,cx		;Check for only controller-ready.
ChkTO1:	mov	ah,CMDTO	;Use 500-msec command timeout.
ChkTO2:	mov	es,bx		;Point to low-memory BIOS timer.
	mov	si,BIOSTMR
	add	ah,es:[si]	;Set timeout limit in AH-reg.
ChkTO3:	cmp	ah,es:[si]	;Has our I-O timed out?
	stc			;(If so, set carry flag).
	je s	ChkTOX		;Yes?  Exit with carry flag on.
	mov	dx,[bx+IdeDA-@]	;Read IDE primary status.
	add	dx,7
	in	al,dx
	test	al,BSY		;Is our controller still busy?
	jnz s	ChkTO3		;Yes, loop back and test again.
	or	cl,cl		;Are we also awaiting I-O data?
	jz s	ChkTOX		;No, just exit.
	test	al,cl		;Is data-request (DRQ) also set?
	jz s	ChkTO3		;No, loop back and test again.
ChkTOX:	ret			;Exit:  Carry "on" denotes a timeout!
;
; Subroutine to convert an LBA sector number to "RedBook" minutes,
;   seconds, and "frames" format (MSF).
;
CvtMSF:	add	eax,150		;Add in offset.
	push	eax		;Get address in DX:AX-regs.
	pop	ax
	pop	dx
	mov	cx,75		;Divide by 75 "frames"/second.
	div	cx
	shl	eax,16		;Set "frames" remainder in upper EAX.
	mov	al,dl
	ror	eax,16
	mov	cl,60		;Divide quotient by 60 seconds/min.
	div	cl
	ret			;Exit -- EAX-reg. contains MSF value.
HMALEN1	equ	($+8-HDReq)	  ;(Length of all HMA driver logic).
	ifndef	PMDVR
;
; Main UIDE Caching Routine.   (UIDE2 routines are assembled below).
;
Cache:	cmp	[bx+RqTyp-@].lb,0C0h ;Is this a diskette request?
	jne s	Flush		     ;No, see if cache needs a flush.
	mov	es,bx		    ;Point ES-reg. to low memory.
	mov	al,es:[DKTSTAT]	    ;Get BIOS diskette status code.
	and	al,01Fh
	cmp	al,MCHANGE	    ;Diskette media-change detected?
	je s	FlushA		    ;Yes, go flush our cache now.
Flush:	test	[bx+IOF-@].lb,2	    ;Does cache need to be flushed?
	jz s	NextIO		    ;No, proceed with this request.
FlushA:	mov	[bx+STLmt-@],bx	    ;Reset binary-search limit index.
	or	[bx+LUTop-@].dwd,-1 ;Reset LRU start & end indexes.
	and	[bx+IOF-@].lb,0FDh  ;O.K. to reset flush flag now.
NextIO:	push	ds		    ;Set ES-reg. same as DS-reg.
	pop	es
	mov	al,0		;Get "granularity" (sectors/block).
@GRAN1	equ	[$-1].lb	;(Block "granularity", set by Init).
	cmp	al,[bx+RqSec-@]	;Will we need multiple cache blocks?
	jbe s	SetSC		;Yes, use "full block" sector count.
	mov	al,[bx+RqSec-@]	;Use remaining request sector count.
SetSC:	mov	[bx+RqCSC-@],al	;Set maximum I-O sector count.
	mov	[bx+RqXMS-@],bx ;Reset lower XMS buffer offset.
	mov	dx,0		;Set initial binary-search offset.
@MP1	equ	[$-2].lw	;(Search-table midpoint, Init set).
	mov	bp,dx		;Set initial binary-search index.
	dec	bp
Search:	shr	dx,1		;Divide binary-search offset by 2.
	cmp	dx,(SBUFSZ/8)	;Time to load-up our memory buffer?
	jne s	Srch1		;No, see if index is too high.
	call	LoadMB		;Load indexes into memory buffer.
	jc s	SrchE		;If any XMS error, go exit below.
Srch1:	cmp	bp,[bx+STLmt-@]	;Is our search index too high?
	jae s	SrchLo		;Yes, cut search index by offset.
	call	SIGet		;Get next binary-search table index.
	jc s	SrchE		;If any XMS error, exit immediately!
	mov	ax,WBLBA	;Get next cache entry into buffer.
	call	CBGet
SrchE:	jc	EndIO		;If any XMS error, exit immediately!
	mov	si,(RqLBA-@)	;Compare initial request & table LBA.
	call	CComp
	jae s	ChkEnd		;Request >= table:  Check for found.
SrchLo:	sub	bp,dx		;Subtract offset from search ptr.
	or	dx,dx		;Offset zero, i.e. last compare?
	jnz s	Search		;No, go compare next table entry.
	jmp s	NoFind		;Handle this request as "not found".
ChkEnd:	je	Found		;Request = table:  Treat as "found".
	mov	cl,[bx+CBSec-@]	;Calculate and set ending entry LBA.
	mov	si,(CBLBA-@)
	call	CalcEA
	mov	si,(RqLBA-@)	;Compare request start & entry end.
	call	CComp1
	jb s	Found		  ;Request < Entry:  Handle as found.
	ja s	SrchHi		  ;Request > Entry:  Bump search ptr.
	cmp	[bx+CBSec-@].lb,0 ;Is this cache entry "full"?
@GRAN2	equ	[$-1].lb	  ;(Block "granularity", Init set).
	jb s	Found		;No, handle this request as "found".
SrchHi:	add	bp,dx		;Add offset to search pointer.
	or	dx,dx		;Offset zero, i.e. last compare?
	jnz s	Search		;No, go compare next table entry.
	inc	bp		;Bump index to next table entry.
NoFind:	mov	WBLBA+2,bp	;Unfound:  Save search-table offset.
	mov	bp,[bx+STLmt-@]	;Get next "free" cache-table index
	call	STGet		;  and leave index in work buffer.
	jc s	NFind1		;If any XMS error, exit immediately!
	movzx	ecx,bp		  ;Get move-up word count.
	movzx	esi,[WBLBA+2].lw
	sub	cx,si		  ;Any search indexes to move up?
	jz s	NFind2		  ;No, go set up new cache entry.
	shl	ecx,1		  ;Set move-up byte count.
	shl	esi,1		  ;Set 32-bit move-up addresses.
	add	esi,STblAdr
	lea	edi,[esi+2]
	push	ecx		;Save move length & destination addr.
	push	edi
	mov	edi,0		;Set search-buffer destination addr.
@SBuff	equ	[$-4].dwd
	push	edi		;Save search-table buffer address.
	call	MvData		;Send needed data to search buffer.
	pop	esi		;Reload search-table buffer address.
	pop	edi		;Reload move destination & length.
	pop	ecx
	jc s	NFind1		;If any XMS error, exit immediately!
	call	MvData		;Bring data BACK from search buffer.
	jc s	NFind1		;If any XMS error, exit immediately!
	mov	bp,WBLBA+2	;Set up table-index "put" to XMS.
	inc	bx
	call	STPut		;Insert "free" index in search table.
NFind1:	jc s	UpdErr		;If XMS error, exit immediately!
NFind2:	inc	[bx+STLmt-@].lw	;Advance binary-search limit index.
	mov	si,(RqLBA-@)	;Set 48-bit LBA in new entry.
	mov	di,(CBLBA-@)
	movsd
	movsw
	movsb			;Set "cache unit" in new entry.
	mov	[di],bl		;Reset new entry's sector count.
	mov	ax,WBLBA	;Reload "free" cache-table index.
	xchg	bx,bx		;(Unused alignment "filler").
Found:	mov	RqIndex,ax	   ;Post current cache-entry index.
	mov	cx,[bx+RqLBA+4-@]  ;Get starting I-O offset in block.
	sub	cx,[bx+CBLBA+4-@]
	mov	[bx+RqXMS-@],cl	   ;Set starting XMS sector offset.
	mov	[bx+LBA2+1-@],cx   ;Save starting I-O sector offset.
	cmp	[bx+CBSec-@],bl	   ;Is this a new cache-table entry?
	je s	ReLink		   ;Yes, relink entry as top-of-list.
	push	ax		   ;Unlink this entry from LRU list.
	call	UnLink
	pop	ax
ReLink:	mov	[bx+RqXMS+2-@],bx  ;Reset upper XMS buffer offset.
	movzx	edx,ax		   ;Get 32-bit cache block number.
	shl	edx,2		   ;Shift number to starting sector.
@GRSSC	equ	[$-1].lb	   ;("Granularity" shift, Init set).
	add	[bx+RqXMS-@],edx   ;Add to "preset" sector offset.
	shl	[bx+RqXMS-@].dwd,9	  ;Convert sectors to bytes.
	add	[bx+RqXMS-@].dwd,020000h  ;Add in XMS "base" address.
@XBase	equ	[$-4].dwd		  ;(XMS "base", set by Init).
	mov	cx,0FFFFh	;Make this entry "top of list".
	or	[bx+CBLst-@],cx	;Set this entry's "last" index.
	mov	dx,ax		;Swap top-of-list & entry index.
	xchg	dx,[bx+LUTop-@]
	mov	[bx+CBNxt-@],dx ;Set this entry's "next" index.
	cmp	dx,cx		;Is this the only LRU index?
	mov	cx,ax		;(Get this entry's index in CX-reg.).
	je s	ReLnk1		;Yes, make entry last on LRU list.
	push	ax		;Link entry to prior "top of list".
	call	UnLnk3
	pop	ax
	jmp s	ReLnk2		;Go deal with I-O sector count.
Update:	mov	[bx+CBSec-@],cl	;Update this entry's total sectors.
	call	CBPut		;Update this cache-table entry.
UpdErr:	jc s	EndIOJ		;If any XMS error, exit immediately!
	movzx	cx,[bx+RqCSC-@]	;Calculate ending LBA for this I-O.
	mov	si,(RqLBA-@)
	call	CalcEA
	inc	bp		;Skip to next search index.
	nop			;(Unused alignment "filler").
Ovrlap:	cmp	bp,[bx+STLmt-@]	;More cache-table entries to check?
	jae s	CachIO		;No, O.K. to handle "found" entry.
	call	STGet		;Get next search table index.
	jc s	EndIOJ		;If any XMS error, bail out now!
	mov	ax,WBLBA	;Get next cache entry into buffer.
	call	CBGet
	jc s	EndIOJ		;If any XMS error, exit immediately!
	mov	si,(LBABuf-@)	;Compare request end & entry start.
	call	CComp
	jbe s	CachIO		;Request <= entry:  O.K. to proceed.
	push	bp		;Delete this overlapping table entry.
	call	Delete
	pop	bp
	jmp s	Ovrlap		  ;Go check for more entry overlap.
ReLnk1:	mov	[bx+LUEnd-@],ax	  ;Make entry last on LRU list, too!
ReLnk2:	mov	cx,[bx+LBA2+1-@]  ;Reload initial I-O sector offset.
	mov	ch,0		  ;Get entry's available sectors.
@GRAN3	equ	[$-1].lb	  ;(Block "granularity", Init set).
	sub	ch,cl
	cmp	ch,[bx+RqCSC-@]	;More I-O sectors than available?
	jae s	Larger		;No, retain maximum sector count.
	mov	[bx+RqCSC-@],ch	;Reduce current I-O sector count.
	nop			;(Unused alignment "filler").
Larger:	add	cl,[bx+RqCSC-@]	;Get ending I-O sector number.
	cmp	cl,[bx+CBSec-@]	;More sectors than entry has now?
	ja s	Update		;Yes, update entry sectors.
	inc	bx		;Reset Z-flag for "put" into XMS.
	call	CBPut		;Update this cache-table entry.
EndIOJ:	jc s	EndIO		      ;If any XMS error, exit fast!
	test	[bx+RqCmd-@].lb,002h  ;Is this a read request?
	jz s	BufMov		      ;Yes, move cache data to user.
CachIO:	bts	[bx+RqCmd-@].lb,6     ;I-O done during a prior block?
	jc s	BfMore		      ;Yes, buffer more cache data.
	cmp	[bx+RqTyp-@].lb,CDTYP ;What type of unit is this?
	je s	CDVDIO		      ;CD/DVD -- call "RLRead" below.
	ja s	UserIO		;BIOS or user -- go check which one.
	call	UdmaIO		;Call "UdmaIO" for SATA or IDE disks.
	jmp s	ErrChk		;Go check for any I-O errors.
CDVDIO:	call	RLRead		;Call "RLRead" for CD or DVD input.
	jmp s	ErrChk		;Go check for any I-O errors.
UserIO:	or	[bx+RqTyp-@],bl	;Is this unit handled by the BIOS?
	js s	BiosIO		;Yes, go "Call the BIOS" below.
	call	DMAAd.dwd	;Call user-driver "callback" subrtn.
	jmp s	ErrChk		;Go check for any I-O errors.
BiosIO:	lds	si,CStack	;BIOS I-O:  Reload caller CPU flags.
	push	[si+2].lw
	popf
	pop	es		;Reload all CPU registers.
	pop	ds
	popad
	pushf			;"Call the BIOS" to do this request.
	call	ss:@Prev13
	pushad			;Save all CPU registers again.
	push	ds
	push	es
	mov	al,ah		;Move any BIOS error code to AL-reg.
ErrChk:	sti			;Restore critical driver settings.
	cld
	push	ss		;Reload this driver's DS- & ES-regs.
	pop	ds
	push	ds
	pop	es
	mov	bx,0		;Rezero BX-reg. but save carry.
	jnc s	InCach		;If no error, go see about caching.
IOErr:	push	ax		;Error!  Save returned error code.
	mov	ax,RqIndex	;Delete cache entry for this I-O.
	call	SrchD
	pop	ax		;Reload returned error code.
	stc			;Set carry again to denote error.
EndIO:	db	0EAh		;Return to hard-disk exit routine.
	dw	(HDExit-@)
@HDXSeg	dw	0		;(Driver segment address, Init set).
InCach:	or	[bx+RqCmd-@],bl	;Did user data get cached during I-O?
	js s	ChkFul		;Yes, check if cache tables are full.
BfMore:	or	al,1		     ;Ensure we LOAD data into cache!
BufMov:	movzx	ecx,[bx+RqCSC-@].lb  ;Set XMS move sector count.
	mov	esi,[bx+RqXMS-@]     ;Set desired XMS buffer address.
	mov	edi,[bx+RqBuf-@]     ;Set user buffer as destination.
	call	UBufM1		     ;Move data between user and XMS.
	jc s	IOErr			;If error, use routine above.
ChkFul:	cmp	[bx+STLmt-@].lw,08000h  ;Is binary-search table full?
@TE1	equ	[$-2].lw		;(Table "end", Init set).
	jb s	MoreIO		   ;No, check for more sectors to go.
	push	ax		   ;Delete least-recently-used entry.
	mov	ax,[bx+LUEnd-@]
	call	SrchD
	pop	ax
MoreIO:	xor	ax,ax		   ;Reset error code for exit above.
	movzx	cx,[bx+RqCSC-@]	   ;Get current I-O sector count.
	sub	[bx+RqSec-@],cl	   ;More data sectors left to handle?
	jz s	EndIO		   ;No, return to our "exit" routine.
	add	[bx+RqLBA+4-@],cx  ;Update current LBA for next I-O.
	adc	[bx+RqLBA+2-@],bx
	adc	[bx+RqLBA-@],bx
	shl	cx,1		   ;Convert sector count to bytes.
	add	[bx+RqBuf+1-@],cl  ;Update user I-O buffer address.
	adc	[bx+RqBuf+2-@],bx
	jmp	NextIO		   ;Go handle next cache data block.
;
; Subroutine to calculate ending request or cache-table LBA values.
;
CalcEA:	mov	di,(LBABuf-@)	;Point to address-comparison buffer.
	push	[si].dwd	;Move high-order LBA into buffer.
	pop	[di].dwd
	add	cx,[si+4]	;Calculate ending LBA.
	mov	[di+4],cx
	adc	[di+2],bx
	adc	[di],bx
CalcX:	ret			;Exit.
;
; Subroutine to do 7-byte "cache unit" and LBA comparisons.
;
CComp:	mov	di,(CBLBA-@)	;Point to current-buffer LBA value.
CComp1:	mov	cl,[bx+RqUNo-@]	;Compare our "unit" & table "unit".
	cmp	cl,[bx+CBUNo-@]
	jne s	CalcX		;If units are different, exit now.
	mov	cx,3		;Compare our LBA & cache-table LBA.
	rep	cmpsw
	ret			;Exit -- Main routine checks results.
;
; Subroutine to "put" an LRU index into its cache-data table entry.
;
LRUPut:	push	ax		;Save all needed 16-bit regs.
	push	cx
	push	dx
	pushf			;Save CPU Z-flag (always 0 = "put").
	mov	edi,(WBLBA-@)	;Set our "working" buffer address.
@WBAdr1	equ	[$-4].dwd	;(Working-buffer address, Init set).
	mov	ecx,12		;Set cache-table entry size.
	mul	cx		;Multiply cache index by entry size.
	mov	cl,2		;Set LRU-index size ("put" 2 bytes).
	jmp s	CEMov1		;Go get desired cache-entry offset.
;
; Subroutine to "get" a search-table index, during a binary search.
;   Other "gets" of search-table indexes are done by "STGet" below.
;
SIGet:	cmp	dx,(SBUFSZ/8)	;Is desired index already in memory?
	ja s	STGet		;No, go "get" desired index from XMS.
	mov	di,(SBUFSZ/2)-1	;Get desired index number in buffer.
	and	di,bp
	shl	di,1		;Set desired index in "work" buffer.
	mov	ax,cs:[di+(MemBuf-@)]
@MBOffs	equ	[$-2].lw
	mov	WBLBA,ax
	ret			;Exit (carry zeroed by "shl" above).
;
; Subroutine to load binary-search indexes into our memory buffer.
;
LoadMB:	push	ax		;Save all needed 16-bit regs.
	push	cx
	push	dx
	mov	ecx,SBUFSZ	;Set search memory-buffer length.
	xor	esi,esi		;Get starting index number.
	lea	si,[bp-((SBUFSZ/4)-1)]
	shl	esi,1		;Set starting XMS index address.
	add	esi,STblAdr
	mov	edi,offset (MemBuf-@)  ;Set memory buffer address.
@MBAddr	equ	[$-4].dwd
	jmp s	CEMov3		;Go move indexes into buffer & exit.
;
; Subroutine to "get" or "put" a cache-data table entry (12 bytes).
;
CBGet:	xor	di,di		;Entry buffer:  Set Z-flag for "get".
CBPut:	mov	edi,(CBLBA-@)	;Set 32-bit "current" buffer address.
@CBAddr	equ	[$-4].dwd	;(Current-buffer address, Init set).
CEMov:	mov	esi,dword ptr 0	;Set cache-table "base" address.
@CTBas1	equ	[$-4].dwd	;(Cache-table "base", Init set).
	push	ax		;Save all needed 16-bit regs.
	push	cx
	push	dx
	pushf			;Save CPU Z-flag from above.
	mov	ecx,12		;Set cache-table entry size.
	mul	cx		;Multiply cache index by entry size.
CEMov1:	push	dx		;Get cache-entry offset in EAX-reg.
	push	ax
	pop	eax
	jmp s	CeMov2		;Go set final XMS data address.
;
; Subroutine to "get" or "put" a binary-search table index (2 bytes).
;
STGet:	xor	di,di		;Search index:  Set Z-flag for get.
STPut:	mov	edi,(WBLBA-@)	;Set 32-bit "working" buffer addr.
@WBAdr2	equ	[$-4].dwd	;(Working-buffer address, Init set).
	mov	esi,STblAdr	;Add starting search-table address.
	push	ax		;Save all needed 16-bit regs.
	push	cx
	push	dx
	pushf			;Save CPU Z-flag from above.
	mov	ecx,2		;Set binary-search index size.
	movzx	eax,bp		;Get 32-bit offset of requested
	shl	eax,1		;  index in binary-search table.
	xchg	bx,bx		;(Unused alignment "filler").
CeMov2:	add	esi,eax		;Set XMS data address (base + offset).
	popf			;Reload CPU flags.
	jz s	CeMov3		;Are we "getting" from XMS memory?
	xchg	esi,edi		;No, "swap" source and destination.
CeMov3:	push	bp		;Save BP-reg. now (helps alignment).
	call	MvData		;"Get" or "put" desired cache entry.
	pop	bp		;Reload all 16-bit regs. we used.
	pop	dx
	pop	cx
	pop	ax
	jc s	CEMErr		;If XMS error, post "flush" & exit.
	ret			;All is well -- exit.
CEMErr:	or	[bx+IOF-@].lb,2	;BAD News!  Post "flush cache" flag.
	stc			;Set carry again after "or" command.
	mov	al,XMSERR	;Post "XMS error" return code.
	ret			;Exit.
;
; Subroutine to search for a cache-table index, and then delete it.
;   Our binary-search table has unit/LBA order, not LRU order!   To
;   delete an LRU index, we do a binary-search for it and then pass
;   the ending BP-reg. value to our "Delete" routine below.
;
SrchD:	call	CBGet		;Get target cache-table entry.
	jc s	SrchDE		;If XMS error, go exit below.
	mov	dx,0		;Set initial binary-search offset.
@MP2	equ	[$-2].lw	;(Search-table midpoint, Init set).
	mov	bp,dx		;Set initial binary-search index.
	dec	bp
	nop			;(Unused alignment "filler").
SrchD1:	shr	dx,1		;Divide binary-search offset by 2.
	cmp	dx,(SBUFSZ/8)	;Time to load-up our memory buffer?
	jne s	SrchD2		;No, see if pointer is too high.
	call	LoadMB		;Load indexes into memory buffer.
	jc s	SrchDE		;If any XMS error, go exit below.
SrchD2:	cmp	bp,[bx+STLmt-@]	;Is our search pointer too high?
	jae s	SrchD4		;Yes, cut search pointer by offset.
	call	SIGet		;Get next binary-search table index.
	jc s	SrchDE		;If any XMS error, go exit below.
	xor	cx,cx		;Clear CX-reg., set Z-flag for "get".
	mov	edi,(WBLBA-@)	;Set 32-bit "working" buffer address.
@WBAdr3	equ	[$-4].dwd	;(Working-buffer address, Init set).
	mov	ax,WBLBA	;Get next cache entry in work buffer.
	call	CEMov
	jc s	SrchDE		;If any XMS error, go exit below.
	mov	si,(CBLBA-@)	;Set up target v.s. work comparison.
	mov	di,(WBLBA-@)
	mov	cl,[si+6]	;Compare target unit v.s. work unit.
	cmp	cl,[di+6]
	jne s	SrchD3		;If units differ, check results now.
	mov	cl,3		;Compare target LBA v.s. work LBA.
	rep	cmpsw
	je s	Delete		;Target = entry:  BP has our offset.
SrchD3:	ja s	SrchD5		;Target > entry:  Adjust offset up.
SrchD4:	sub	bp,dx		;Subtract offset from search ptr.
	jmp s	SrchD6		;Go see if we did our last compare.
SrchD5:	add	bp,dx		;Add offset to search pointer.
SrchD6:	or	dx,dx		;Offset zero, i.e. last compare?
	jnz s	SrchD1		;No, go compare next table entry.
	call	CEMErr		;Not found!  Handle as an XMS error!
SrchDE:	jmp s	UnLnkE		;Go discard stack data and exit.
	db	0		;(Unused alignment "filler").
;
; Subroutine to delete a cache index from our search table.
;
Delete:	mov	WBLBA,ax	     ;Save table index being deleted.
	movzx	ecx,[bx+STLmt-@].lw  ;Get move-down word count.
	sub	cx,bp
	dec	cx		;Any other indexes to move down?
	jz s	Delet1		;No, put our index in "free" list.
	shl	ecx,1		;Set move-down byte count.
	movzx	edi,bp		;Set 32-bit move-down addresses.
	shl	edi,1
	add	edi,STblAdr
	lea	esi,[edi+2]
	call	MvData		;Move down needed search-table data.
	jc s	UnLnkE		;If any XMS error, go exit below.
	xchg	bx,bx		;(Unused alignment "filler").
Delet1:	dec	[bx+STLmt-@].lw	;Decrement search-table limit index.
	mov	bp,[bx+STLmt-@]	;Set up table-index "put" to XMS.
	inc	bx
	call	STPut		;Put deleted index in "free" list.
	jc s	UnLnkE		;If any XMS error, go exit below.
;
; Subroutine to unlink a cache-table entry from the LRU list.
;
UnLink:	mov	cx,[bx+CBLst-@]	 ;Get current entry's "last" index.
	mov	dx,[bx+CBNxt-@]	 ;Get current entry's "next" index.
	cmp	cx,-1		 ;Is this entry top-of-list?
	je s	UnLnk1		 ;Yes, "promote" next entry.
	mov	WBLBA,dx	 ;Save this entry's "next" index.
	mov	ax,cx		 ;Get "last" entry cache-table index.
	mov	esi,dword ptr 10 ;Get cache-table addr. + "next" ofs.
@CTBas2	equ	[$-4].dwd	 ;(Cache-table address, set by Init).
	call	LRUPut		;Update last entry's "next" index.
	jnc s	UnLnk2		;If no XMS error, check end-of-list.
UnLnkE:	pop	cx		;XMS error!  Discard exit address &
	pop	cx		;  AX/BP parameter saved upon entry.
	jmp	EndIO		;Go post XMS error code and get out!
UnLnk1:	mov	[bx+LUTop-@],dx	;Make next entry top-of-list.
UnLnk2:	cmp	dx,-1		;Is this entry end-of-list?
	jne s	UnLnk3		;No, link next to prior entry.
	mov	[bx+LUEnd-@],cx	;Make prior entry end-of-list.
	ret			;Exit.
UnLnk3:	mov	WBLBA,cx	;Save this entry's "last" index.
	mov	ax,dx		;Get "next" entry cache-table index.
	mov	esi,dword ptr 8 ;Get cache-table addr. + "last" ofs.
@CTBas3	equ	[$-4].dwd	;(Cache-table address, set by Init).
	call	LRUPut		;Update next entry's "last" index.
	jc s	UnLnkE		;If any XMS error, go exit above!
	ret			;All is well -- exit.
	db	0		   ;(Unused alignment "filler").
MemBuf	equ	($+4)		   ;(Start of binary-search buffer).
HMALEN2	equ	($+4+SBUFSZ-HDReq) ;(Length of HMA caching logic).
	else
;
; Main UIDE2 Caching Routine.   All following routines are UIDE2 only.
;
Cache:	cmp	[bx+RqTyp-@].lb,0C0h ;Is this a diskette request?
	jne s	Flush		     ;No, see if cache needs a flush.
	mov	es,bx		    ;Point ES-reg. to low memory.
	mov	al,es:[DKTSTAT]	    ;Get BIOS diskette status code.
	and	al,01Fh
	cmp	al,MCHANGE	    ;Diskette media-change detected?
	je s	FlushA		    ;Yes, go flush our cache now.
Flush:	test	[bx+IOF-@].lb,2	    ;Does cache need to be flushed?
	jz s	NextIO		    ;No, proceed with this request.
FlushA:	cld			    ;Ensure FORWARD "string" cmds.!
	mov	ax,0		    ;Get binary-search table count.
@TC1	equ	[$-2].lw	    ;(Cache-entry count, Init set).
	les	di,STblAdr	    ;Get binary-search table addr.
	mov	[bx+STLmt-@],di	    ;Reset "free" cache-index ptr.
	or	[bx+LUTop-@].dwd,-1 ;Reset LRU start & end indexes.
FlushT:	dec	ax		    ;"Flush" binary-search table.
	stosw			    ;(Store cache-entry indexes in
	jnz s	FlushT		    ;  descending order, runs O.K.).
	and	[bx+IOF-@].lb,0FDh  ;O.K. to reset flush flag now.
NextIO:	push	ds		    ;Set ES-reg. same as DS-reg.
	pop	es
	mov	al,0		;Get "granularity" (sectors/block).
@GRAN1	equ	[$-1].lb	;(Block "granularity", set by Init).
	cmp	al,[bx+RqSec-@]	;Will we need multiple cache blocks?
	jbe s	SetSC		;Yes, use "full block" sector count.
	mov	al,[bx+RqSec-@]	;Use remaining request sector count.
SetSC:	mov	[bx+RqCSC-@],al	;Set maximum I-O sector count.
	mov	[bx+RqXMS-@],bx ;Reset lower XMS buffer offset.
	mov	dx,0		;Set initial binary-search offset.
@MP1	equ	[$-2].lw	      ;(Midpoint offset, Init set).
	mov	bp,offset (SrchT-2-@) ;Set table midpoint address.
@MP2	equ	[$-2].lw	      ;(Midpoint address, Init set).
Search:	shr	dx,1		;Divide binary-search offset by 2.
	and	dl,0FEh		;Ensure offset remains "even".
	cmp	bp,[bx+STLmt-@]	;Is our search pointer too high?
	jae s	SrchLo		;Yes, cut search pointer by offset.
	call	SIGet		;Get next cache entry into buffer.
@SIGet1	equ	[$-2].lw	;(Becomes "call SIGet2" for /H).
	jc	EndIO		;If any XMS error, exit immediately!
	mov	si,(RqLBA-@)	;Compare initial request & table LBA.
	call	CComp
	jae s	ChkEnd		;Request >= table:  Check for found.
SrchLo:	sub	bp,dx		;Subtract offset from search ptr.
	or	dx,dx		;Offset zero, i.e. last compare?
	jnz s	Search		;No, go compare next table entry.
	jmp s	NoFind		;Handle this request as "not found".
ChkEnd:	je s	Found		;Request = table:  Treat as "found".
	mov	cl,[bx+CBSec-@]	;Calculate and set ending entry LBA.
	mov	si,(CBLBA-@)
	call	CalcEA
	mov	si,(RqLBA-@)	;Compare request start & entry end.
	call	CComp1
	jb s	Found		   ;Request < Entry:  Handle as found.
	ja s	SrchHi		   ;Request > Entry:  Bump search ptr.
	cmp	[bx+CBSec-@].lb,0  ;Is this cache entry "full"?
@GRAN2	equ	[$-1].lb	   ;(Block "granularity", Init set).
	jb s	Found		;No, handle this request as "found".
SrchHi:	add	bp,dx		;Add offset to search pointer.
	or	dx,dx		;Offset zero, i.e. last compare?
	jnz s	Search		;No, go compare next table entry.
	inc	bp		;Bump pointer to next table entry.
	inc	bp
NoFind:	mov	di,[bx+STLmt-@]	;Not found -- Point to "free" index.
	mov	es,STblAdr.hw	;Point ES-reg. to our search table.
	mov	ax,es:[di]	;Get next "free" cache-entry index.
	std			;Enable a "backward" move, and set
	lea	si,[di-2]	;  move-up source address & count.
	mov	cx,di		;  We do the "std" early, for more
	sub	cx,bp		;  time margin v.s. AMD CPU "bugs"!
	shr	cx,1
	rep	movs es:[di].lw,es:[si].lw ;Move up higher indexes.
	stosw			           ;Store new table index.
	cld			   ;Re-enable FORWARD "string" cmds.
	push	ds		   ;Reload this driver's ES-reg.
	pop	es
	add	[bx+STLmt-@].lw,2  ;Bump binary-search table limit.
	mov	si,(RqLBA-@)	   ;Set 48-bit LBA in new entry.
	mov	di,(CBLBA-@)
	movsd
	movsw
	movsb			   ;Set "cache unit" in new entry.
	mov	[di].lb,0	   ;Reset new entry's sector count.
Found:	mov	RqIndex,ax	   ;Post current cache-entry index.
	mov	cx,[bx+RqLBA+4-@]  ;Get starting I-O offset in block.
	sub	cx,[bx+CBLBA+4-@]
	mov	[bx+RqXMS-@],cl	   ;Set starting XMS sector offset.
	mov	[bx+LBA2+1-@],cx   ;Save starting I-O sector offset.
	cmp	[bx+CBSec-@],bl	   ;Is this a new cache-table entry?
	je s	ReLink		   ;Yes, relink entry as top-of-list.
	push	ax		   ;Unlink this entry from LRU list.
	call	UnLink
	pop	ax
ReLink:	mov	[bx+RqXMS+2-@],bx  ;Reset upper XMS buffer offset.
	movzx	edx,ax		   ;Get 32-bit cache block number.
	shl	edx,2		   ;Shift number to starting sector.
@GRSSC	equ	[$-1].lb	   ;("Granularity" shift, Init set).
	add	[bx+RqXMS-@],edx   ;Add to "preset" sector offset.
	shl	[bx+RqXMS-@].dwd,9	  ;Convert sectors to bytes.
	add	[bx+RqXMS-@].dwd,020000h  ;Add in XMS "base" address.
@XBase	equ	[$-4].dwd		  ;(XMS "base", set by Init).
	mov	cx,0FFFFh	;Make this entry "top of list".
	or	[bx+CBLst-@],cx	;Set this entry's "last" index.
	mov	dx,ax		;Swap top-of-list & entry index.
	xchg	dx,[bx+LUTop-@]
	mov	[bx+CBNxt-@],dx ;Set this entry's "next" index.
	cmp	dx,cx		;Is this the only LRU index?
	mov	cx,ax		;(Get this entry's index in CX-reg.).
	je s	ReLnk1		;Yes, make entry last on LRU list.
	push	ax		;Link entry to prior "top of list".
	call	UnLnk3
	pop	ax
	jmp s	ReLnk2		;Go deal with I-O sector count.
Update:	mov	[bx+CBSec-@],cl	;Update this entry's total sectors.
	call	CBPut		;Update this cache-table entry.
	jc s	EndIOJ		;If any XMS error, exit immediately!
	movzx	cx,[bx+RqCSC-@]	;Calculate ending LBA for this I-O.
	mov	si,(RqLBA-@)
	call	CalcEA
	inc	bp		;Skip to next search index.
	inc	bp
Ovrlap:	cmp	bp,[bx+STLmt-@]	;More cache-table entries to check?
	jae s	CachIO		;No, O.K. to handle "found" entry.
        call	SIGet		;Get next cache entry into buffer.
@SIGet2	equ	[$-2].lw	;(Becomes "call SIGet2" for /H).
	jc s	EndIOJ		;If any XMS error, exit immediately!
	mov	si,(LBABuf-@)	;Compare request end & entry start.
	call	CComp
	jbe s	CachIO		;Request <= entry:  O.K. to proceed.
	push	bp		;Delete this overlapping table entry.
	call	Delete
	pop	bp
	jmp s	Ovrlap		 ;Go check for more entry overlap.
ReLnk1:	mov	[bx+LUEnd-@],ax	 ;Make entry last on LRU list, too!
ReLnk2:	mov	cx,[bx+LBA2+1-@] ;Reload initial I-O sector offset.
	mov	ch,0		 ;Get entry's available sectors.
@GRAN3	equ	[$-1].lb	 ;(Block "granularity", Init set).
	sub	ch,cl
	cmp	ch,[bx+RqCSC-@]	;More I-O sectors than available?
	jae s	Larger		;No, retain maximum sector count.
	mov	[bx+RqCSC-@],ch	;Reduce current I-O sector count.
	nop			;(Unused alignment "filler").
Larger:	add	cl,[bx+RqCSC-@]	;Get ending I-O sector number.
	cmp	cl,[bx+CBSec-@]	;More sectors than entry has now?
	ja s	Update		;Yes, update entry sectors.
	inc	bx		;Reset Z-flag for "put" into XMS.
	call	CBPut		;Update this cache-table entry.
EndIOJ:	jc s	EndIO		      ;If any XMS error, exit fast!
	test	[bx+RqCmd-@].lb,002h  ;Is this a read request?
	jz s	BfMove		      ;Yes, move cache data to user.
CachIO:	bts	[bx+RqCmd-@].lb,6     ;I-O done during a prior block?
	jc s	BufMor		      ;Yes, buffer more cache data.
	cmp	[bx+RqTyp-@].lb,CDTYP ;What type of unit is this?
	je s	CDVDIO		      ;CD/DVD -- call "RLRead" below.
	ja s	UserIO		;User or BIOS -- go check which one.
	call	UdmaIO		;Call "UdmaIO" for SATA/IDE disks.
	jmp s	ErrChk		;Go check for any I-O errors.
CDVDIO:	call	RLRead		;Call "RLRead" for CD/DVD input.
	jmp s	ErrChk		;Go check for any I-O errors.
UserIO:	or	[bx+RqTyp-@],bl	;Is this unit handled by the BIOS?
	js s	BiosIO		;Yes, go "Call the BIOS" below.
	call	DMAAd.dwd	;Call user entry-point handler.
	jmp s	ErrChk		;Go check for any I-O errors.
BiosIO:	lds	si,CStack	;BIOS I-O:  Reload caller CPU flags.
	push	[si+2].lw
	popf
	pop	es		;Reload all CPU registers.
	pop	ds
	popad
	pushf			;"Call the BIOS" to do this request.
	call	ss:@Prev13
	pushad			;Save all CPU registers again.
	push	ds
	push	es
	mov	al,ah		;Move any BIOS error code to AL-reg.
ErrChk:	sti			;Restore critical driver settings.
	cld
	push	ss		;Reload this driver's DS- & ES-regs.
	pop	ds
	push	ds
	pop	es
	mov	bx,0		;Rezero BX-reg. but save carry.
	jnc s	InCach		;If no error, go see about caching.
IOErr:	push	ax		;Error!  Save returned error code.
	mov	ax,RqIndex	;Delete cache entry for this I-O.
	call	SrchD
@SrchD1	equ	[$-2].lw	;(Becomes "call Scnd" for /H /HL).
	pop	ax		;Reload returned error code.
	stc			;Set carry again to denote error.
EndIO:	db	0EAh		;Return to hard-disk exit routine.
	dw	(HDExit-@)
@HDXSeg	dw	0		;(Driver segment address, Init set).
InCach:	or	[bx+RqCmd-@],bl	;Did user data get cached during I-O?
	js s	ChkFul		;Yes, check if cache tables are full.
BufMor:	or	al,1		     ;Ensure we LOAD data into cache!
BfMove:	movzx	ecx,[bx+RqCSC-@].lb  ;Set XMS move sector count.
	mov	esi,[bx+RqXMS-@]     ;Set desired XMS buffer address.
	mov	edi,[bx+RqBuf-@]     ;Set user buffer as destination.
	call	UBufM1		     ;Move data between user and XMS.
	jc s	IOErr			   ;If error, use rtn. above.
ChkFul:	cmp	[bx+STLmt-@].lw,(SrchT-@)  ;Binary-search table full?
@TE1	equ	[$-2].lw		   ;(Table "end", Init set).
	jb s	MoreIO		   ;No, check for more sectors to go.
	push	ax		   ;Delete least-recently-used entry.
	mov	ax,[bx+LUEnd-@]
	call	SrchD
@SrchD2	equ	[$-2].lw	   ;(Becomes "call Scnd" for /H /HL).
	pop	ax
MoreIO:	xor	ax,ax		   ;Reset error code for exit above.
	movzx	cx,[bx+RqCSC-@]	   ;Get current I-O sector count.
	sub	[bx+RqSec-@],cl	   ;More data sectors left to handle?
	jz s	EndIO		   ;No, return to our "exit" routine.
	add	[bx+RqLBA+4-@],cx  ;Update current LBA for next I-O.
	adc	[bx+RqLBA+2-@],bx
	adc	[bx+RqLBA-@],bx
	shl	cx,1		   ;Convert sector count to bytes.
	add	[bx+RqBuf+1-@],cl  ;Update user I-O buffer address.
	adc	[bx+RqBuf+2-@],bx
	jmp	NextIO		   ;Go handle next cache data block.
;
; Subroutine to calculate ending request or cache-table LBA values.
;
CalcEA:	mov	di,(LBABuf-@)	;Point to address-comparison buffer.
	push	[si].dwd	;Move high-order LBA into buffer.
	pop	[di].dwd
	add	cx,[si+4]	;Calculate ending LBA.
	mov	[di+4],cx
	adc	[di+2],bx
	adc	[di],bx
CalcX:	ret			;Exit.
;
; Subroutine to do 7-byte "cache unit" and LBA comparisons.
;
CComp:	mov	di,(CBLBA-@)	;Point to current-buffer LBA value.
CComp1:	mov	cl,[bx+RqUNo-@]	;Compare our "unit" & table "unit".
	cmp	cl,[bx+CBUNo-@]
	jne s	CalcX		;If units are different, exit now.
	mov	cx,3		;Compare our LBA & cache-table LBA.
	rep	cmpsw
	ret			;Exit -- Main routine checks results.
;
; Subroutine to "put" an LRU index into its cache-data table entry.
;
LRUPut:	mov	edi,(WBLBA-@)	;LRU index:  Set "work" buffer addr.
@WBAdr1	equ	[$-4].dwd	;(Working-buffer address, Init set).
	push	ax		;Save all needed 16-bit regs.
	push	cx
	push	dx
	xor	ecx,ecx		;Set our cache-table entry size.
	or	cl,12		;(Also resets CPU Z-flag for "put").
	pushf			;Save CPU Z-flag from above.
	mul	cx		;Multiply cache index by entry size.
	mov	cl,2		;Set LRU index size ("put" 2 bytes).
	jmp s	CEMov1		;Go get desired cache-entry offset.
;
; Subroutine to "get" a cache-data table entry for binary-searches.
;   Upon entering here, the search-table pointer is in the BP-reg.
;
SIGet:	mov	es,STblAdr.hw	;Point ES-reg. to our search table.
	mov	ax,es:[bp]	;Get next binary-search table index.
SIGet2:	mov	di,ds		;Reload this driver's ES-reg.
	mov	es,di
;
; Subroutine to "get" or "put" a cache-data table entry.
;
CBGet:	xor	di,di		;Entry buffer:  Set Z-flag for "get".
CBPut:	mov	edi,(CBLBA-@)	;Set 32-bit "current" buffer address.
@CBAddr	equ	[$-4].dwd	;(Current-buffer address, Init set).
CEMov:	mov	esi,dword ptr 0	;Set cache-table "base" address.
@CTBas1	equ	[$-4].dwd	;(Cache-table "base", Init set).
	push	ax		;Save all needed 16-bit regs.
	push	cx
	push	dx
	pushf			;Save CPU Z-flag from above.
	xor	ecx,ecx		;Set cache-table entry size.
	mov	cl,12
	mul	cx		;Multiply cache index by entry size.
CEMov1:	push	dx		;Get cache-entry offset in EAX-reg.
	push	ax
	pop	eax
	add	esi,eax		;Set XMS data address (base + offset).
	popf			;Reload CPU flags.
	jz s	CEMov2		;Are we "getting" from XMS memory?
	xchg	esi,edi		;No, "swap" source and destination.
CEMov2:	push	bp		;Save BP-reg. now (helps alignment).
	call	MvData		;"Get" or "put" desired cache entry.
	pop	bp		;Reload all 16-bit regs. we used.
	pop	dx
	pop	cx
	pop	ax
	jc s	CEMovE		;Any JEMM386 or BIOS move errors?
	ret			;No, all is well -- exit.
CEMovE:	or	[bx+IOF-@].lb,2	;BAD News!  Post "flush cache" flag.
	stc			;Set carry again after "or" command.
	mov	al,XMSERR	;Post "XMS error" return code.
	ret			;Exit.
;
; Subroutine to search for a cache-table index, and then delete it.
;   Our binary-search table has unit/LBA order, not LRU order!   To
;   delete an LRU index, we do a binary-search for it and then pass
;   the ending BP-reg. value to our "Delete" routine below.
;
SrchD:	xor	di,di		;Set CPU Z-flag for "get".
	mov	edi,(WBLBA-@)	;Get entry to delete in work buffer.
@WBAdr2	equ	[$-4].dwd	;(Working-buffer address, Init set).
	call	CEMov
	jc s	SrchDE		;If XMS error, go exit below.
	mov	dx,0		;Set initial binary-search offset.
@MP3	equ	[$-2].lw	      ;(Midpoint offset, Init set).
	mov	bp,offset (SrchT-2-@) ;Set table midpoint address.
@MP4	equ	[$-2].lw	      ;(Midpoint address, Init set).
SrchD1:	shr	dx,1		;Divide binary-search offset by 2.
	and	dl,0FEh		;Ensure offset remains "even".
	cmp	bp,[bx+STLmt-@]	;Is our search pointer too high?
	jae s	SrchD3		;Yes, cut search pointer by offset.
	call	SIGet		;Get next cache entry into buffer.
	jc s	SrchDE		;If any XMS error, go exit below.
	mov	si,(WBLBA-@)	;Set up work v.s. current compare.
	mov	di,(CBLBA-@)
	mov	cl,[si+6]	;Compare work v.s. current unit no.
	cmp	cl,[di+6]
	jne s	SrchD2		;If units differ, check results now.
	mov	cx,3		;Compare work v.s. current LBA.
	rep	cmpsw
	je s	Delete		;Work = current:  BP has our offset.
SrchD2:	ja s	SrchD4		;Work > current:  Adjust offset up.
SrchD3:	sub	bp,dx		;Subtract offset from search ptr.
	jmp s	SrchD5		;Go see if we did our last compare.
SrchD4:	add	bp,dx		;Add offset to search pointer.
SrchD5:	or	dx,dx		;Offset zero, i.e. last compare?
	jnz s	SrchD1		;No, go compare next table entry.
	call	CEMovE		;Not found!  Handle as an XMS error!
SrchDE:	jmp s	UnLnkE		;Go discard stack data and exit.
;
; Subroutine to scan for a binary-search table index and delete it.
;   If /H or /HL is given, this routine is used instead of "SrchD".
;
ScnD:	mov	cx,0		;Set up a 16-bit "scan" command.
@TC2	equ	[$-2].lw	;(Search-table count, Init set).
	les	di,STblAdr
	repne	scasw		;"Scan" for index in search table.
	lea	bp,[di-2]	;Index is at [di-2], due to "scasw".
	call	CBGet		;Get cache entry from XMS to buffer.
	jc s	UnLnkE		;If any XMS error, go exit below.
	xchg	bx,bx		;(Unused alignment "filler").
;
; Subroutine to delete a cache index from our search table.
;
Delete:	mov	es,STblAdr.hw	   ;Point ES-reg. to search table.
	mov	di,bp		   ;Point DI-reg. to index in table.
	lea	si,[di+2]	   ;Set move-down source and count.
	mov	cx,[bx+STLmt-@]
	sub	cx,si
	shr	cx,1
	rep	movs es:[di].lw,es:[si].lw ;Move down higher indexes.
	stosw			   ;Put deleted index on "free" list.
	mov	cx,ds		   ;Reload this driver's ES-register.
	mov	es,cx
	sub	[bx+STLmt-@].lw,2  ;Decrement search-table limit.
;
; Subroutine to unlink a cache-table entry from the LRU list.
;
UnLink:	mov	cx,[bx+CBLst-@]	 ;Get current entry's "last" index.
	mov	dx,[bx+CBNxt-@]	 ;Get current entry's "next" index.
	cmp	cx,-1		 ;Is this entry top-of-list?
	je s	UnLnk1		 ;Yes, "promote" next entry.
	mov	WBLBA,dx	 ;Save this entry's "next" index.
	mov	ax,cx		 ;Get "last" entry cache-table index.
	mov	esi,dword ptr 10 ;Get cache-table addr. + "next" ofs.
@CTBas2	equ	[$-4].dwd	 ;(Cache-table address, set by Init).
	call	LRUPut		;Update last entry's "next" index.
	jnc s	UnLnk2		;If no XMS error, check end-of-list.
UnLnkE:	pop	cx		;XMS error!  Discard exit address &
	pop	cx		;  AX/BP parameter saved upon entry.
	jmp	EndIO		;Go post XMS error code and get out!
UnLnk1:	mov	[bx+LUTop-@],dx	;Make next entry top-of-list.
UnLnk2:	cmp	dx,-1		;Is this entry end-of-list?
	jne s	UnLnk3		;No, link next to prior entry.
	mov	[bx+LUEnd-@],cx	;Make prior entry end-of-list.
	ret			;Exit.
UnLnk3:	mov	WBLBA,cx	;Save this entry's "last" index.
	mov	ax,dx		;Get "next" entry cache-table index.
	mov	esi,dword ptr 8 ;Get cache-table addr. + "last" ofs.
@CTBas3	equ	[$-4].dwd	;(Cache-table address, set by Init).
	call	LRUPut		;Update next entry's "last" index.
	jc s	UnLnkE		;If any XMS error, go exit above!
	ret			;All is well -- exit.
	db	0		 ;(Unused alignment "filler").
SrchT	equ	($+8)		 ;(Resident search-table start).
ResEnd	equ	($+8+STACK)	 ;(End of resident caching driver).
HMALEN2	equ	($+8-HDReq)	 ;(Length of HMA logic using /H).
LMT165	equ	HMALEN2+(165*32) ;(V7.10 MS-DOS HMA limits for /H
LMT280	equ	(280*32)	 ;   and /HL -- Largest is used!).
	endif			 ;(End of all caching routines).
;
; CD/DVD I-O Address Table.   The 1st word for each set is its DMA
;   controller base address, which is set by Init.   A value of -1
;   denotes an unused controller.   A value of 0 in the DVD1 table
;   indicates an old system having no DMA controller, whose CD/DVD
;   drives must run in "PIO mode" only.    The second word is each
;   controller's default "legacy" base I-O address.    For "native
;   PCI mode", all base I-O addresses are set during Init.   There
;   is an EXTRA address set, so that on old systems, the first set
;   of "legacy" addresses can be used even with no DMA controller.
;   Note that this table is used only during init.    At run-time,
;   a CD/DVD drive's addresses are taken from its parameter table.
;
DVD1Tbl	dw	0		;IDE1 primary-master parameters.
DVD1Pri	dw	NPDATA
	dw	0		;IDE1 secondary-master parameters.
DVD1Sec	dw	NSDATA
DVD2Tbl	dw	0FFFFh		;IDE2 primary-master parameters.
DVD2Pri	dw	APDATA
	dw	0FFFFh		;IDE2 secondary-master parameters.
DVD2Sec	dw	ASDATA
	dd	18 dup (0FFFFh)	;IDE3 to IDE10 & extra parameters.
DVDTEnd	equ	$		;End of CD/DVD addresses table.
DVDTSIZ	equ	DVD2Tbl-DVD1Tbl	;Size of a CD/DVD address table.
;
; Initialization UltraDMA "Mode" Table (digit count in low 4 bits).
;
Modes	dw	01602h		;Mode 0, ATA-16.
	dw	02502h		;Mode 1, ATA-25.
	dw	03302h		;Mode 2, ATA-33.
	dw	04402h		;Mode 3, ATA-44.
	dw	06602h		;Mode 4, ATA-66.
	dw	01003h		;Mode 5, ATA-100.
	dw	01333h		;Mode 6, ATA-133.
	dw	01663h		;Mode 7, ATA-166.
EDDBuff equ	$		;(Start of EDD BIOS data buffer).
	ifndef	PMDVR
;
; UIDE Cache-Sizes Table.   "I_Adj" below adjusts 40-MB to 1023-MB
;   caches to have 16K data blocks, also 1024-MB to 2047-MB caches
;   to have 32K data blocks.   Inefficient 64K data blocks are now
;   used only if a 2048-MB cache (2 Gigabytes) or more is desired.
;   The /F switch will prevent such adjustments.
;
CachSiz	dw	(5*128)		; 5-MB "tiny" cache.
	db	16		;   8K sectors per cache block
	db	4		;      and sector-shift count.
	dw	512		;   512 binary-search midpoint.
	dw	10		;   10K cache-tables XMS memory.
	db	"   5"		;   Title message cache-size text.
	dw	(15*64)		;15-MB "small" cache.
	db	32,5		;   16K granularity values.
	dw	512		;   512 binary-search midpoint.
	dw	15		;   15K cache-tables XMS memory.
	db	"  15"		;   Title message cache-size text.
	dw	(25*64)		;25-MB "medium 1" cache.
	db	32,5		;   16K granularity values.
	dw	1024		;   1024 binary-search midpoint.
	dw	25		;   25K cache-tables XMS memory.
	db	"  25"		;   Title message cache-size text.
	dw	(40*32)		;40-MB "medium 2" cache.
	db	64,6		;   32K granularity values.
	dw	1024		;   1024 binary-search midpoint.
	dw	20		;   20K cache-tables XMS memory.
	db	"  40"		;   Title message cache-size text.
	dw	(50*32)		;50-MB "medium 3" cache.
	db	64,6		;   32K granularity values.
	dw	1024		;   1024 binary-search midpoint.
	dw	25		;   25K cache-tables XMS memory.
	db	"  50"		;   Title message cache-size text.
LCBlks	dw	0FFFFh		;80-MB to 4093-MB "large" cache.
	db	128,7		;   64K granularity values.
	dw	1024		;   1024 to 32768 search midpoint.
	dw	32		;   32K to 1152K cache-tables XMS.
	db	"  80"		;   Title message cache-size text.
CSDflt	equ	5		;Default cache-size flag if no /S.
CSMax	equ	4093		;Maximum cache size in megabytes.
	else
;
; UIDE2 Cache-Sizes Table.   This table is arranged so that, when
;   inadequate HMA or upper/DOS memory is available for a desired
;   cache, UIDE2 can use the previous table entry to set a lesser
;   "default" cache size, instead!
;
CachSiz	dw	(5*128)		; 5-MB "tiny" cache.
	db	16		;   8K sectors per cache block
	db	4		;      and sector-shift count.
	dw	1024		;   1024 binary-search midpoint.
	dw	8		;   8K cache-tables XMS memory.
	db	"   5"		;   Title message cache-size text.
	dw	(15*64)		;15-MB "small" cache.
	db	32,5		;   16K granularity values.
	dw	1024		;   1024 binary-search midpoint.
	dw	12		;   12K cache-tables XMS memory.
	db	"  15"		;   Title message cache-size text.
	dw	(25*64)		;25-MB "medium 1" cache.
	db	32,5		;   16K granularity values.
	dw	2048		;   2048 binary-search midpoint.
	dw	20		;   20K cache-tables XMS memory.
	db	"  25"		;   Title message cache-size text.
	dw	(40*32)		;40-MB "medium 2" cache.
	db	64,6		;   32K granularity values.
	dw	2048		;   2048 binary-search midpoint.
	dw	16		;   16K cache-tables XMS memory.
	db	"  40"		;   Title message cache-size text.
	dw	(50*32)		;50-MB "medium 3" cache.
	db	64,6		;   32K granularity values.
	dw	2048		;   2048 binary-search midpoint.
	dw	20		;   20K cache-tables XMS memory.
	db	"  50"		;   Title message cache-size text.
	dw	(80*16)		;80-MB "default" cache, set when a
	db	128,7		;   desired "large" cache does NOT
	dw	2048		;   fit our available free memory!
	dw	16
	db	"  80"
LCBlks	dw	0FFFFh		;80-MB to 1900-MB "large" cache.
	db	128,7		;   64K granularity values.
	dw	2048		;   2048 to 32768 search midpoint.
	dw	24		;   24K to 384K cache-tables XMS.
	db	"  80"		;   Title message cache-size text.
CSDflt	equ	6		;Default cache-size flag if no /S.
CSMax	equ	1900		;Maximum cache size in megabytes.
	endif
;
; Initialization Messages.
;
TTLMsg	db	CR,LF,'UIDE'
	ifdef	PMDVR
	db	'2'		;Add 2 if assembling UIDE2.
	endif
	db	', 10-16-2011.   '
TTL2	db	'$   -MB Cache, $'
TTL3	db	'CD/DVD name is '
TTLName	db	'         ',CR,LF,0
NPMsg	db	'No V2.0C+ PCI, BIOS I-O only!',CR,LF,'$'
NIMsg	db	'/N3 illegal$'
NXMsg	db	'No XMS manager$'
DNMsg	db	' d'
CDName	db	'isk is '
DName	equ	$
DNEnd	equ	DName+40
BCMsg	db	'BAD$'
PCMsg	db	'IDE0'
PCMsg1	db	' Controller at I-O address '
PCMsg2	db	'PCI h, Chip I.D. '
PCMsg3	db	'UDVD1   h.',CR,LF,'$'
NonUMsg	db	'Disks run by the BIOS:  '
NonUCt	db	'0.',CR,LF,'$'
EDDMsg	db	'EDD BIOS$'
CHSMsg	db	'CHS'
UnitMsg	db	' data ERROR, D'
UnitNam	db	'rive '
UnitNo	db	'A: ',CR,LF,'$'
CDMsg	db	'CD'
CDMsgNo	db	'0:  '
CtlMsg	db	'IDE'
CtlrNo	db	'0 $'
PriMsg	db	'Primary-$'
SecMsg	db	'Secondary-$'
MstMsg	db	'master$'
SlvMsg	db	'slave$'
IEMsg	db	' Identify ERROR!',CR,LF,'$'
UEMsg	db	' is not UltraDMA!',CR,LF,'$'
NDMsg	db	'Nothing to use$'
VEMsg	db	'VDS init error$'
CPUMsg	db	'No 80386+ CPU'
Suffix	db	'; UIDE'
	ifdef	PMDVR
	db	'2'		;Add 2 if assembling UIDE2.
	endif
	db	' not loaded!',CR,LF,'$'
;
; Initialization "Strategy" Routine.   This MUST be placed above all
;   run-time logic, to prevent CPU cache "code modification" ERRORS!
;
I_Stra:	mov	cs:RqPkt.lw,bx	;Save DOS request-packet address.
	mov	cs:RqPkt.hw,es
	retf			;Exit & await DOS "Device Interrupt".
;
; Initialization "Device Interrupt" Routine.   This is the main init
;   routine for the driver.
;
I_Init:	pushf			;Entry -- save CPU flags.
	push	ds		;Save CPU segment registers.
	push	es
	push	ax		;Save needed 16-bit CPU registers.
	push	bx
	push	dx
	xor	ax,ax		;Get a zero for following logic.
	lds	bx,cs:RqPkt	;Point to DOS "Init" packet.
	cmp	[bx].RPOp,al	;Is this really an "Init" packet?
	jne s	I_Exit		;No?  Reload registers and get out!
	ifndef	PMDVR			;For UIDE only --
	mov	[bx].RPStat,RPDON+RPERR	;  Set "Init" packet defaults
	mov	[bx].RPSeg,cs		;    and "null" driver length.
	and	[bx].RPLen,ax
	else				;For UIDE2 only --
	mov	[bx].RPStat,RPDON+RPERR	;  Set "Init" packet defaults.
	xchg	ax,[bx].RPLen		;  Set "null" driver length &
	mov	dx,cs			;    get "free memory" limit.
	xchg	dx,[bx].RPSeg
	endif
	push	cs		;NOW point DS-reg. to this driver!
	pop	ds
	ifdef	PMDVR		;For UIDE2 only --
	mov	PktLBA.lw,ax	;  Save lower "free memory" limit.
	endif
	push	sp		;See if CPU is an 80286 or newer.
	pop	ax		;(80286+ push SP, then decrement it).
	cmp	ax,sp		;Did SP-reg. get saved "decremented"?
	jne s	I_Junk		;Yes?  CPU is an 8086/80186, TOO OLD!
	pushf			;80386 test -- save CPU flags.
	push	07000h		;Try to set NT|IOPL status flags.
	popf
	pushf			;Get resulting CPU status flags.
	pop	ax
	popf			;Reload starting CPU flags.
	test	ah,070h		;Did any NT|IOPL bits get set?
	jnz s	I_Sv32		;Yes, go save 32-bit CPU registers.
I_Junk:	mov	dx,(CPUMsg-@)	;Point to "No 80386+ CPU" message.
I_Quit:	call	I_Msg		;Display "No 80386" or msg. suffix.
I_Exit:	jmp	I_Bye		;Go reload 16-bit regs. and exit.
I_VErr:	mov	VEMsg.dwd,eax	;Set prefix in "VDS init error" msg.
	mov	dx,(VEMsg-@)	;Point to "VDS init error" message.
I_Err:	push	dx		;Init ERROR!  Save message pointer.
	mov	ax,08104h	;"Unlock" this driver from memory.
	xor	dx,dx
	call	I_VDS
I_XDis:	mov	dx,CStack.lw	;Load our XMS "handle" number.
	or	dx,dx		;Have we reserved any XMS memory?
	jz s	I_LDMP		;No, reload pointer & display msg.
	mov	ah,00Dh		;Unlock and "free" our XMS memory.
	push	dx
	call	I_XMS
	mov	ah,00Ah
	pop	dx
	call	I_XMS
I_LDMP:	pop	dx		;Reload error message pointer.
I_EMsg:	call	I_Msg		;Display desired error message.
	popad			;Reload all 32-bit CPU registers.
	mov	dx,(Suffix-@)	;Display message suffix and exit!
	jmp s	I_Quit
I_Sv32:	pushad			;Save all 32-bit CPU registers.
	les	si,es:[bx].RPCL	;Get command-line data pointer.
	xor	bx,bx		;Zero BX-reg. for relative logic.
	ifdef	PMDVR		   ;For UIDE2 only --
	mov	[bx+PktLBA+2-@],dx ;  Save upper "free memory" limit.
	endif
I_NxtC:	mov	al,es:[si]	;Get next command-line byte.
	inc	si		;Bump pointer past first byte.
	cmp	al,'/'		;Is byte a slash?
	je s	I_NxtS		;Yes, see what next "switch" byte is.
	cmp	al,0		;Is byte the command-line terminator?
	je s	I_TrmJ		;Yes, go validate desired cache size.
	cmp	al,LF		;Is byte an ASCII line-feed?
	je s	I_TrmJ		;Yes, go validate desired cache size.
	cmp	al,CR		;Is byte an ASCII carriage-return?
I_TrmJ:	je	I_Term		;Yes, go validate desired cache size.
	cmp	al,'-'		;Is byte a dash?
	jne s	I_NxtC		;No, check next command-line byte.
I_NxtS:	mov	ax,es:[si]	;Get next 2 command-line bytes.
	and	al,0DFh		;Mask out 1st byte's lower-case bit.
	cmp	ax,"3N"		;Are these 2 switch bytes "N3"?
	jne s	I_ChkA		;No, see if 1st byte is "A" or "a".
	mov	UseXMS,bl	;Reset "Use XMS memory" flag.
	jmp s	I_CkBA		;Go set up our stand-alone driver.
I_ChkA:	cmp	al,'A'		;Is switch byte an "A" or "a"?
	jne s	I_ChkB		  ;No, see if byte is "B" or "b".
	mov	al,(ASDATA-0100h) ;Reverse all "Legacy IDE" addrs.
	mov	Ctl1Sec.lb,al
	mov	DVD1Sec.lb,al
	mov	al,(APDATA-0100h)
	mov	Ctl1Pri.lb,al
	mov	DVD1Pri.lb,al
	mov	al,(NSDATA-0100h)
	mov	Ctl2Sec.lb,al
	mov	DVD2Sec.lb,al
	mov	al,(NPDATA-0100h)
	mov	Ctl2Pri.lb,al
	mov	DVD2Pri.lb,al
I_ChkB:	cmp	al,'B'		  ;Is switch byte a "B" or "b"?
	jne s	I_ChkE		  ;No, see if byte is "E" or "e".
I_CkBA:	mov	HMASize,HMALEN1	  ;Set stand-alone driver HMA size.
	mov	USRPtr,bx	  ;Reset user-driver entry pointer.
	db	066h,0B8h	  ;Enable "stand alone" disk driver
	db	0E8h		  ;  via "call UdmaIO" at "GoMain".
	dw	(UdmaIO-GoMain-3)
	push	ss
	mov	GoMain.dwd,eax
	mov	al,0EBh		;Disable I-O thru cache buffers.
	mov	@NoCA.lb,al
	mov	@NoCB.lb,al	;Disable all CD/DVD caching.
I_ChkE:	cmp	al,'E'		;Is switch byte an "E" or "e"?
	jne s	I_ChkF		;No, see if byte is "F" or "f".
	mov	EFlag,al	;Set hard-disk "emulator" flag.
I_ChkF:	ifndef	PMDVR		;For UIDE only --
	cmp	al,'F'		;  Is switch byte an "F" or "f"?
	jne s	I_ChkH		;  No, see if byte is "H" or "h".
	mov	FFlag,al	;  Set our "Fast XMS cache" flag.
	endif
I_ChkH:	cmp	al,'H'		;Is switch byte an "H" or "h"?
	jne s	I_CkN1		;No, see if these 2 bytes are "N1".
	ifndef	PMDVR		;For UIDE only --
	mov	HFlag,al	;  Set "use HMA space" flag.
	else			;For UIDE2 only --
	mov	HFlag,001h	;  Set "use HMA space" flag.
	and	ah,0DFh		;  Mask out 2nd byte lower-case bit.
	cmp	ah,'L'		;  Is 2nd switch byte an "L" or "l"?
	jne s	I_CkN1		;  No, just ignore it for now.
	ror	HFlag,1		;  Set "large HMA cache" flag.
	endif
I_CkN1:	cmp	ax,"1N"		;Are these 2 switch bytes "N1"?
	jne s	I_CkN2		;No, see if switch bytes are "N2".
	mov	BiosHD,bl	;Disable all hard-disk handling.
I_CkN2:	cmp	ax,"2N"		;Are these 2 switch bytes "N2"?
	jne s	I_CkN4		;No, see if switch bytes are "N4".
	mov	DVDTblB,ax	;Disable all CD/DVD drive handling.
I_CkN4:	cmp	ax,"4N"		;Are these 2 switch bytes "N4"?
	je s	I_ChkZ		;Yes, enable 2K protected XMS moves.
	cmp	al,'Z'		;Is byte a "Z" or "z" (as for XMGR)?
	jne s	I_ChkQ		;No, see if byte is "Q" or "q".
I_ChkZ:	mov	@MvPOfs.lb,(MvPNxt-@MvpOfs-1) ;Do 2K P.M. XMS moves.
I_ChkQ:	cmp	al,'Q'		;Is switch byte a "Q" or "q"?
	jne s	I_ChkR		;No, see if byte is "R" or "r".
	mov	@DRQ.lb,075h	;Enable "data request" tests above.
I_ChkR:	cmp	al,'R'		;Is switch byte an "R" or "r"?
	jne s	I_ChkS		;No, see if byte is "S" or "s".
	mov	ax,es:[si+1]	;Get next 2 command-line bytes.
	mov	cx,15296	;Get 15-MB XMS memory size.
	cmp	ax,"51"		;Does user want 15-MB XMS reserved?
	je s	I_CkRA		;Yes, set memory size to reserve.
	mov	ch,(64448/256)	;Get 63-MB XMS memory size.
	cmp	ax,"36"		;Does user want 63-MB XMS reserved?
	jne s	I_NxtJ		;No, continue scan for a terminator.
I_CkRA:	mov	CStack.hw,cx	;Set desired XMS memory to reserve.
I_ChkS:	cmp	al,'S'		;Is switch byte an "S" or "s"?
	jne s	I_ChkD		;No, see if switch bytes are "D:".
	inc	si		;Bump scan ptr. past "S" or "s".
	mov	di,(LCBlks-@)	  ;Point to "large cache" block ct.
	mov	[di+8].dwd,"    " ;Reset "large cache" title bytes.
I_CkS0:	mov	[di].lw,08000h	  ;Invalidate cache-block count.
I_CkS1:	movzx	ax,es:[si].lb	;Get next command-line byte.
	cmp	al,'9'		;Is byte greater than a '9'?
	ja s	I_NxtJ		;Yes, ignore it and continue scan.
	cmp	al,'0'		;Is byte less than a '0'?
	jb s	I_NxtJ		;Yes, ignore it and continue scan.
	inc	si		;Bump scan pointer past digit.
	cmp	[di+8].lb,' '	;Have we already found 4 digits?
	jne s	I_CkS0		;Yes, set INVALID & keep scanning.
	push	[di+9].dwd	;Shift "title" bytes 1 place left.
	pop	[di+8].dwd
	mov	[di+11],al	;Insert next "title" message byte.
	mov	cx,[di]		;Multiply current block size by 10.
	shl	[di].lw,2
	add	[di],cx
	shl	[di].lw,1
	and	al,00Fh		;"Add in" next cache size digit.
	add	[di],ax
	jmp s	I_CkS1		;Go scan more cache-size digits.
I_ChkD:	cmp	ax,":D"		;Are next 2 bytes "D:" or "d:"?
	je s	I_Nam0		;Yes, scan desired device name.
	and	ah,0DFh		;Mask out 2nd byte's lower-case bit.
	cmp	ax,"XU"		;Are next 2 bytes "UX" or "ux".
	jne s	I_NxtJ		;No, continue scan for a terminator.
	mov	NoDMA,al	;Set CD/DVD "No UltraDMA" flag.
I_NxtJ:	jmp	I_NxtC		;Continue scanning for a terminator.
I_Nam0:	inc	si		;Device name -- Skip past "D:" bytes.
	inc	si
	mov	bl,8		;Set driver-name byte count of 8.
	mov	cx,bx
	mov	di,(DvrNam-@)	;Blank out previous driver name.
I_Nam1:	dec	bx
	mov	[bx+di].lb,' '
	jnz s	I_Nam1
I_Nam2:	mov	al,es:[si]	;Get next device-name byte.
	cmp	al,TAB		;Is byte a "tab"?
	je s	I_NxtJ		;Yes, handle above, "name" has ended!
	cmp	al,' '		;Is byte a space?
	je s	I_NxtJ		;Yes, handle above, "name" has ended!
	cmp	al,'/'		;Is byte a slash?
	je s	I_NxtJ		;Yes, handle above, "name" has ended!
	cmp	al,0		;Is byte the command-line terminator?
	je s	I_Term		;Yes, go check for valid device name.
	cmp	al,LF		;Is byte an ASCII line-feed?
	je s	I_Term		;Yes, go check for valid device name.
	cmp	al,CR		;Is byte an ASCII carriage-return?
	je s	I_Term		;Yes, go check for valid device name.
	cmp	al,'a'		;Ensure letters are upper-case.
	jb s	I_Nam3
	cmp	al,'z'
	ja s	I_Nam3
	and	al,0DFh
I_Nam3:	cmp	al,'!'		;Is this byte an exclamation point?
	je s	I_Nam4		;Yes, store it in device name.
	cmp	al,'#'		;Is byte below a pound-sign?
	jb s	I_InvN		;Yes, Invalid!  Zero first byte.
	cmp	al,')'		;Is byte a right-parenthesis or less?
	jbe s	I_Nam4		;Yes, store it in device name.
	cmp	al,'-'		;Is byte a dash?
	je s	I_Nam4		;Yes, store it in device name.
	cmp	al,'0'		;Is byte below a zero?
	jb s	I_InvN		;Yes, invalid!  Zero first byte.
	cmp	al,'9'		;Is byte a nine or less?
	jbe s	I_Nam4		;Yes, store it in device name.
	cmp	al,'@'		;Is byte below an "at sign"?
	jb s	I_InvN		;Yes, invalid!  Zero first byte.
	cmp	al,'Z'		;Is byte a "Z" or less?
	jbe s	I_Nam4		;Yes, store it in device name.
	cmp	al,'^'		;Is byte below a carat?
	jb s	I_InvN		;Yes, invalid!  Zero first byte.
	cmp	al,'~'		;Is byte above a tilde?
	ja s	I_InvN		;Yes, invalid!  Zero first byte.
	cmp	al,'|'		;Is byte an "or" symbol?
	je s	I_InvN		;Yes, invalid!  Zero first byte.
I_Nam4:	mov	[di],al		;Store next byte in device name.
	inc	si		;Bump command-line pointer.
	inc	di		;Bump device-name pointer.
	loop	I_Nam2		;If more name bytes to go, loop back.
	jmp s	I_NxtJ		 ;Go get next command byte.
I_InvN:	mov	[bx+DvrNam-@],bl ;Invalid name!  Zero first byte.
	jmp s	I_NxtJ		 ;Go get next command byte.
I_Term:	mov	di,(LCBlks-@)	;Point to "large cache" block count.
	xor	cx,cx		;Set 5-MB cache size flag.
	cmp	[di].lw,15	;Does user want less than 15-MB?
	jb s	I_CSzF		;Yes, give user a 5-MB cache.
	mov	cl,2		;Set 25-MB cache size flag.
	cmp	[di].lw,25	;Does user want exactly 25-MB?
	je s	I_CSzF		;Yes, set 25-MB cache-size flag.
	dec	cx		;Set 15-MB cache size flag.
	cmp	[di].lw,40	;Does user want less than 40-MB?
	jb s	I_CSzF		;Yes, give user a 15-MB cache.
	mov	cl,4		;Set 50-MB cache size flag.
	cmp	[di].lw,50	;Does user want exactly 50-MB?
	je s	I_CSzF		;Yes, set 50-MB cache-size flag.
	dec	cx		;Set 40-MB cache size flag.
	cmp	[di].lw,80	;Does user want less than 80-MB?
	jb s	I_CSzF		;Yes, give user a 40-MB cache.
	mov	cl,CSDflt	;Set "large" cache size flag.
	cmp	[di].lw,CSMax	;Is cache size invalid or too big?
	ja s	I_CSzE		;Yes?  Ignore & set default cache.
	mov	ax,128		;Set initial 128-MB cache limit.
I_CSz1:	cmp	[di],ax		;User cache size < current limit?
	jb s	I_CSz2		;Yes, set user cache-block count.
	shl	ax,1		;Double current cache-size limit.
	shl	[di+4].dwd,1	;Double variable cache parameters.
	jmp s	I_CSz1		;Go check user's cache size again.
I_CSz2:	shl	[di].lw,4	;Cache blocks = (16 * Megabytes).
	jmp s	I_CSzF		;Go set "large" cache-size flag.
I_CSzE:	mov	[di].lw,(80*16)   ;Error!  Restore default cache.
	mov	[di+8].dwd,"08  "
I_CSzF:	mov	SFlag,cl	  ;Set desired cache-size flag.
	mov	al,UseXMS	;If user requested "No XMS memory"
	and	HFlag,al	;  ensure we do NOT use HMA space!
	mov	ah,'.'		;Set a period for "stores" below.
	mov	cx,8		;Set driver-name byte count of 8.
	mov	di,(DvrNam-@)	;Point to our driver name.
	cmp	[di].lb,' '	;Is driver "name" valid?
	ja s	I_SetN		;Yes, set name in "title" message.
	mov	bx,cx		;Set "UDVD1" default driver name.
I_DefN:	dec	bx
	mov	al,[bx+(PCMsg3-@)]
	mov	[bx+di],al
	jnz s	I_DefN
I_SetN:	mov	al,[di]		;Get next driver "name" byte.
	cmp	al,' '		;End of driver "name"?
	je s	I_UseH			 ;Yes, check for HMA usage.
	mov	[di+(TTLName-DvrNam)],ax ;Store title byte & period.
	inc	di			 ;Bump driver "name" pointer.
	loop	I_SetN		;If more name bytes to go, loop back.
	ifndef	PMDVR
;
; Cache setup logic for UIDE.
;
I_UseH:	shl	HFlag,1		;Will we be loading in the HMA?
	jz s	I_NoHA		;No, go set upper/DOS memory size.
	mov	ax,04A01h	;Get total "free HMA" space.
	call	I_In2F
	cmp	bx,HMASize	;Enough HMA for our driver logic?
	jae s	I_Siz0		;Yes, check for stand-alone driver.
I_NoHA:	mov	HFlag,0		;Ensure NO use of HMA space!
	mov	ax,HMASize	;Set driver upper/DOS memory size.
	add	VDSLn.lw,ax
I_Siz0:	mov	cx,USRPtr	;"Stand alone" driver requested?
	jcxz	I_TTL		;Yes, go display driver "title".
	mov	al,12		;Point to desired cache-size table.
	mul	SFlag
	add	ax,(CachSiz-@)
	xchg	ax,si
	shl	FFlag,1		;Does user want a "Fast XMS cache"?
	jnz s	I_Adj1		;Yes, keep normal 64K cache blocks.
I_Adj:	cmp	[si].lw,32768	;Using 32K or more cache blocks?
	jae s	I_Adj1		;Yes, go set cache-table values.
	cmp	[si+2].lb,32	;Using an 8K or 16K granularity?
	jbe s	I_Adj1		;Yes, go set cache-table values.
	shl	[si].lw,1	;Double number of cache blocks.
	shr	[si+2].lb,1	;Halve sectors per cache block.
	dec	[si+3].lb	;Decrement sector-shift count.
	shl	[si+4].dwd,1	;Double search midpoint & cache XMS.
	jmp s	I_Adj		;For 80-MB to 1023-MB, adjust again!
I_Adj1:	movzx	eax,[si].lw	;Set binary-search limit index.
	mov	@TE1,ax
	mov	ax,[si+4]	;Set binary-search starting offsets.
	mov	@MP1,ax
	mov	@MP2,ax
	else
;
; Cache setup logic for UIDE2.
;
I_UseH:	test	HFlag,081h	;Will we be loading in the HMA?
	jz s	I_NoHM		;No, go set upper/DOS memory size.
	jns s	I_GetH		;If "small" cache, go get "free HMA".
	mov	cx,USRptr	;"Stand alone" driver requested?
	jcxz	I_LodH		;Yes, must do /H loading in the HMA.
	mov	ax,(ResEnd-@)	;Set "large cache" resident size and
	mov	VDSLn.lw,ax	;  put NO logic in the HMA!  Save it
	and	HMASize,0	;  all for big binary-search tables!
	jmp s	I_GetH		;Go get total "free HMA" space.
I_LodH:	mov	HFlag,001h	;Require /H for "stand alone" driver!
I_GetH:	mov	ax,04A01h	;Get our total "free HMA" space.
	call	I_In2F
	mov	ax,LMT280	;Get our V7.10 MS-DOS HMA limit.
	cmp	bx,ax		;Do we have less HMA than this?
	jb s	I_HSiz		;Yes, use only what HMA we have.
	cmp	bh,(14*4)	;At least 14K of "free HMA"?
	jae s	I_HSiz		;Yes, usually O.K. to use all of it.
	xchg	ax,bx		;May be V7.10 MS-DOS!  Set HMA limit.
I_HSiz:	sub	bx,HMASize	;Enough HMA for all our driver logic?
	jb s	I_NoHM		;No, we must load in normal memory!
	cmp	GoMain.lb,0E8h	;"Stand alone" driver requested?
	je s	I_TTLJ		;Yes, go display driver "title".
	cmp	bh,(5*256)/256	;Can our 5-MB cache fit in the HMA?
	jb s	I_NoHM		;No, we must load in normal memory!
	movzx	cx,SFlag	;Did user request only a 5-MB cache?
	jcxz	I_SetP		;Yes, O.K. to set it up now.
	cmp	bx,(15*128)	;Can our 15-MB cache fit in the HMA?
	jb s	I_NoHM		;No, nicer to load in normal memory!
	cmp	cl,1		;Does user want only a 15-MB cache?
	jbe s	I_SetP		;Yes, O.K. to set it up now.
	cmp	bh,(40*64)/256	;Can 40- or 80-MB cache fit the HMA?
	jae s	I_SetP		;Yes, O.K. to set up all HMA caches.
	mov	cl,1		;Must default to only a 15-MB cache.
	jmp s	I_SetP		;Go set up our 15-MB cache now.
I_NoHM:	mov	HFlag,0		;Ensure NO use of HMA space!
	mov	ax,HMASize	;Set driver upper/DOS memory size.
	add	VDSLn.lw,ax
	cmp	GoMain.lb,0E8h	;"Stand alone" driver requested?
I_TTLJ:	je	I_TTL		;Yes, go display driver "title".
	xor	ebx,ebx		;Get 32-bit "free" memory limit.
	push	bx
	xchg	bx,PktLBA.hw
	shl	ebx,4
	add	ebx,PktLBA
	push	cs		;Subtract 32-bit driver "base" size
	pop	eax		;  for available search-table space.
	add	ax,((ResEnd-@)/16)
	shl	eax,4
	sub	ebx,eax
	jb s	I_Min1		;Limit < driver?  Assume minimum!
	push	ebx		;Get free memory high-order word.
	pop	cx
	pop	cx		;64K or more of free memory?
	jcxz	I_MinM		;No, see if we have minimum needed.
	mov	bx,0FFFFh	;Use 64K - 1 as free memory size.
I_MinM:	cmp	bh,10		;Enough memory for an 80-MB cache?
	jae s	I_Size		;Yes, go see what cache is desired.
I_Min1:	mov	bx,(80*32)	;Kernel FOOLS, UIDE2 loads in > 7K!
I_Size:	mov	cl,SFlag	;Get user cache-size flag.
I_SetP:	mov	al,12		;Point to desired cache-size table.
	mul	cl
	add	ax,(CachSiz-@)
	xchg	ax,si
	mov	ax,[si]		;Get required search-table memory.
	shl	ax,1
	cmp	bx,ax		;Enough memory for requested cache?
	jae s	I_SetC		;Yes, go set it up now.
	sub	si,12		;"Default" to next-smaller cache!
I_SetC:	mov	ax,[si]		;Set cache-entry counts.
	mov	@TC1,ax
	mov	@TC2,ax
	shl	ax,1		;Set binary-search limit address.
	add	@TE1,ax
	cmp	HFlag,0		;Will we be loading in the HMA?
	jne s	I_SetM		;Yes, set search midpoint values.
	add	VDSLn.lw,ax	;Set final resident-driver size.
I_SetM:	mov	ax,[si+4]	;Set binary-search midpoint values.
	mov	@MP1,ax		;Set binary-search midpoint values.
	add	@MP2,ax
	mov	@MP3,ax
	add	@MP4,ax
	endif
	mov	ax,[si+2]	;Set cache "granularity" values.
	mov	@GRAN1,al
	mov	@GRAN2,al
	mov	@GRAN3,al
	mov	@GRSSC,ah
	mov	ah,0		;Multiply number of cache blocks
	shr	ax,1		;  times (sectors-per-block / 2).
	mul	[si].lw
	push	dx		;Get 32-bit cache data XMS size.
	push	ax
	pop	eax
	mov	RqBuf,eax	;Save cache XMS size for below.
	movzx	ecx,[si+6].lw	;Add in cache-tables XMS memory.
	add	eax,ecx
	add	RqXMS,eax	;Save total required XMS memory.
	mov	eax,[si+8]	;Set cache size in "title" message.
	mov	TTL2.dwd,eax
I_TTL:	mov	dx,(TTLMsg-@)	;Display initial "title" message.
	call	I_Msg
	mov	si,(TTL3-@)	;Point to "CD/DVD name" message.
I_TTL3:	lodsb			;Get next byte of message & "swap"
	xchg	ax,dx		;  it into DL-reg. for an Int 21h.
	push	si		;Display next message byte.   If a
	mov	ah,002h		;  dollar-sign is part of a CD/DVD
	call	I_In21		;  name, this special routine will
	pop	si		;  display it properly!
	cmp	[si].lb,0	;Are we at the terminating "null"?
	jnz s	I_TTL3		;No, loop back & display next byte.
	mov	ax,03513h	;Get and save current Int 13h vector
	call	I_In21		;  and our run-time entry addresses.
	push	es
	push	bx
	pop	@Prev13
	mov	StratP.dwd,(((DevInt-@)*65536)+(Strat-@))
	xor	eax,eax		;Zero EAX-reg. for 20-bit addressing.
	mov	es,ax		;Point ES-reg. to low memory.
	mov	al,es:[HDISKS]	;Save our BIOS hard-disk count.
	and	BiosHD,al
	mov	ax,cs		;Set driver's VDS segment address.
	mov	VDSSg.lw,ax
	mov	@HDMain.hw,ax	;Set driver's fixed segment values.
	mov	@CDMain.hw,ax
	mov	@HDXSeg,ax
	mov	@RqESeg,ax
	ifdef	PMDVR		;For UIDE2 only --
	mov	STblAdr.hw,ax	;  Set search-table segment addr.
	endif
	shl	eax,4		;Set driver's VDS 20-bit address.
	mov	VDSAd,eax
	cli			     ;Avoid interrupts in VDS tests.
	test	es:[VDSFLAG].lb,020h ;Are "VDS services" active?
	jz s	I_REnI		     ;No, re-enable CPU interrupts.
	mov	ax,08103h	;"Lock" driver in memory forever, so
	mov	dx,0000Ch	;  EMM386 may NOT re-map this driver
	call	I_VDS		;  when doing UltraDMA -- bad CRASH!
	mov	dx,(VEMsg-@)	;Point to "VDS init error" message.
	jc	I_EMsg		;"Lock" error?  Display msg. & exit!
I_REnI:	sti			;Re-enable CPU interrupts.
	mov	eax,VDSAd	;Get final driver 32-bit address.
	add	PRDAd,eax	;Set "No XMS memory" PRD address.
	add	GDTPAdr,eax	;Relocate "real mode" GDT base addr.
	add	@CLAddr,eax	;Relocate command-list source address.
	add	@WBAdr1,eax	;Relocate "working" buffer addresses.
	add	@WBAdr2,eax
	ifdef	@WBAdr3
	add	@WBAdr3,eax
	endif
	add	@CBAddr,eax	;Relocate cache-entry buffer address.
	ifdef	@MBAddr
	add	@MBAddr,eax	;Relocate search memory-buffer addr.
	endif
	xor	edi,edi		;Get PCI BIOS "I.D." code.
	mov	al,001h
	call	I_In1A
	cmp	edx,PCMsg2.dwd	;Is PCI BIOS V2.0C or newer?
	je s	I_ScnC		;Yes, scan for all IDE controllers.
	mov	dx,(NPMsg-@)	;Display "No V2.0C+ PCI" message.
	call	I_Msg
	jmp s	I_NoXM		;Go see if we need an XMS manager.
I_ScnC:	mov	al,LBABuf.lb	;Get next "interface bit" value.
	and	ax,00003h
	or	ax,PCISubC	;"Or" in subclass & current function.
	call	I_PCIC		;Test for specific PCI class/subclass.
	rol	LBABuf.lb,4	;Swap both "interface bit" values.
	mov	al,LBABuf.lb	;Load next "interface bit" value.
	or	al,al		;Both "interface" values tested?
	jns s	I_ScnC		;No, loop back and test 2nd one.
	add	PCISubC.lb,004h	;More PCI function codes to try?
	jnc s	I_ScnC		;Yes, loop back & try next function.
	cmp	DVDTblA,(DVD1Tbl-@)  ;Any CD/DVD controllers found?
	ja s	I_Scn1		;Yes, see about "Native PCI" checks.
	add	DVDTblA,DVDTSIZ	;Save "legacy" addresses for oldies!
I_Scn1:	test	al,001h		;Have we tested "Native PCI" ctlrs.?
	mov	LBABuf.lb,093h	;(Set "Native PCI" interface bits).
	jz s	I_ScnC		;No, loop back and test them, also.
;
; These additional controller tests could be added, if necessary:
;
;	mov	ax,00400h	;Scan for "RAID" controller.
;	call	I_PCIC
;	mov	ax,00520h	;Scan for ADMA "Single-stepping" ctlr.
;	call	I_PCIC
;	mov	ax,00530h	;Scan for ADMA "Continuous DMA" ctlr.
;	call	I_PCIC
;	mov	ax,00600h	;Scan for standard SATA controller.
;	call	I_PCIC
;	mov	ax,00601h	;Scan for "AHCI" SATA controller.
;	call	I_PCIC
;
I_NoXM:	shl	UseXMS,1	;Has user requested no XMS memory?
	jnz s	I_XMgr		;No, go see about an XMS manager.
	mov	dx,(NIMsg-@)	;Point to "/N3 illegal" message.
	cmp	PRDAd.hw,00Ah	;Are we loaded in upper-memory?
	jae	I_Err		;Yes?  Display error msg. and exit!
	db	066h,0B8h	;Disable hard-disk buffered I-O via
	pop	ax		;  "pop ax"/"jmp Pass1" at "BufIO".
	db	0E9h		;  The "pop ax" discards our return
	dw	(Pass1-BufIO-4)	;  address from "UdmaIO".
	mov	BufIO.dwd,eax
	db	0B8h		;Get "clc" and "ret" commands.
	clc
	ret
	mov	MovCLX.lw,ax	;Disable command-list moves to XMS.
	db	0B0h		;Get "stc" and "ret" commands.
	stc
	mov	SetAd1.lw,ax	;Disable any use of an XMS buffer.
	mov	al,090h		;Disable all "A20" request calls.
	mov	A20Req.lb,al
	mov	@NoXM1.lb,al	;Disable CD/DVD buffered input via
	db	066h,0B8h	;  "nop"/"cmc"/"adc [bx+DMF-@],bl"
	cmc			;  at "@NoXM1" above -- direct DMA
	adc	[bx+DMF-@],bl	;  if buffer O.K., PIO mode if not.
	mov	[@NoXM1+1].dwd,eax
	jmp	I_HDCh		;Go check on our hard-disk count.
I_XMgr:	mov	ax,04300h	;Inquire if we have an XMS manager.
	call	I_In2F
	mov	dx,(NXMsg-@)	;Point to "No XMS manager" message.
	cmp	al,080h		;Is an XMS manager installed?
	jne	I_Err		;No?  Display error message and exit!
	mov	ax,04310h	;Get XMS manager "entry" address.
	call	I_In2F
	push	es		;Save XMS manager "entry" addresses.
	push	bx
	pop	@XEntry
	mov	dx,CStack.hw	;Get "reserved" XMS memory size.
	or	dx,dx		;Does user want any "reserved" XMS?
	jz s	I_XGet		;No, get driver's actual XMS memory.
	mov	ah,009h		;Get 15-MB or 63-MB "reserved" XMS
	call	I_XMS		;  memory, which we "release" below.
	jnz s	I_XErr		;If error, display message and exit!
	mov	CStack.lw,dx	;Save reserved-XMS "handle" number.
I_XGet:	mov	ah,009h		;Get XMS V2.0 "allocate" command.
	cmp	RqXMS.hw,0	;Do we need 64-MB+ of XMS memory?
	je s	I_XReq		;No, request our XMS memory now.
	mov	ah,089h		;Use XMS V3.0 "allocate" command.
I_XReq:	mov	edx,RqXMS	;Request all necessary XMS memory.
	call	I_XMS
	jz s	I_XFre		;If no errors, "free" reserved XMS.
I_XErr:	mov	eax," SMX"	;BAAAD News!  Get "XMS" msg. prefix.
	jmp	I_VErr		;Go display "XMS init error" & exit!
I_XFre:	xchg	CStack.lw,dx	;Save our XMS "handle" number.
	or	dx,dx		;Any XMS reserved by /R15 or /R63?
	jz s	I_XLok		;No, go "lock" our XMS memory.
	mov	ah,00Ah		;"Free" our reserved XMS memory.
	call	I_XMS
	jnz s	I_XErr		;If error, display message and exit!
I_XLok:	mov	ah,00Ch		;"Lock" our driver's XMS memory.
	mov	dx,CStack.lw
	call	I_XMS
	jnz s	I_XErr		;If error, display message and exit!
	shl	edx,16		;Get unaligned 32-bit buffer address.
	or	dx,bx
	mov	eax,edx		;Copy 32-bit address to EAX-reg.
	jz s	I_XBAd		;Any low-order XMS buffer "offset"?
	mov	ax,0FFFFh	;Yes, align address to an even 64K.
	inc	eax
I_XBAd:	mov	@XBAddr,eax	;Save aligned "main buffer" address.
	add	@XBase,eax	;Initialize cache-data base address.
	mov	cx,ax		;Get buffer "offset" in XMS memory.
	sub	cx,dx
	mov	edx,000010000h	;Put command-list after XMS buffer.
	jcxz	I_PRDA		;Is buffer already on a 64K boundary?
	mov	edx,-32		;No, put command-list before buffer.
	dec	@XBase.hw	;Decrement cache-data base by 64K.
I_PRDA:	add	eax,edx		;Set our 32-bit PRD address.
	mov	PRDAd,eax
	mov	ax,[RqBuf+1].lw	;Get needed cache XMS in 256K blocks.
	shl	eax,18		;Convert from 256K blocks to bytes.
	add	eax,@XBase	;Add cache-data XMS base address.
	mov	@CTBas1,eax	;Set XMS cache-table base addresses.
	add	@CTBas2,eax
	add	@CTBas3,eax
	ifdef	PMDVR		;For UIDE2 only --
	mov	cx,USRPtr	;  "Stand alone" driver requested?
	jcxz	I_HDCh		;  Yes, check BIOS hard-disk count.
	else			;For UIDE only --
	xchg	eax,edx		;  Save cache-table base in EDX-reg.
	movzx	eax,@TE1	;  Get binary-search table size.
	shl	eax,1
	mov	ecx,eax		;  Save search-table size in ECX-reg.
	shl	eax,1		;  Get offset to binary-search table.
	add	eax,ecx
	shl	eax,1
	add	eax,edx		;  Set binary-search table address.
	mov	STblAdr,eax
	add	eax,ecx		;  Set search-table buffer address.
	mov	@SBuff,eax
	cmp	GoMain.lb,0E8h	;  "Stand alone" driver requested?
	je	I_HDCh		;  Yes, check BIOS hard-disk count.
	mov	ah,005h		;  Issue "A20 local-enable" request.
	call	I_XMS
	jnz	I_A20E		;  If any "A20" error, bail out NOW!
	mov	dx,@TE1		;  Initialize search-table count.
	xor	bp,bp		;  Initialize search-table index.
I_RST1:	mov	ax,bp		;  Set next 80 search-table indexes
	xor	ecx,ecx		;    in init tables & messages area.
	mov	cl,80
	mov	si,(EDDBuff-@)
I_RST2:	mov	[si],ax
	inc	ax
	inc	si
	inc	si
	loop	I_RST2
	xor	esi,esi		;  Set 32-bit move source address.
	mov	si,(EDDBuff-@)	;  (Offset of our indexes buffer +
	add	esi,VDSAd	;    32-bit driver "base" address).
	movzx	edi,bp		;  Set 32-bit move destination addr.
	shl	edi,1		;  (2 * current "reset" index +
	add	edi,STblAdr	;    binary-search table address).
	mov	bp,ax		;  Update next cache-table index.
	mov	cl,80		;  Get 80-word move length.
	cmp	cx,dx		;  At least 80 words left to go?
	jbe s	I_RST3		;  Yes, use full 80-word count.
	mov	cx,dx		;  Use remaining word count.
I_RST3:	shl	cx,1		;  Convert word count to byte count.
	push	dx		;  Save move count and cache index.
	push	bp
	call	MvData		;  Move 80 indexes into search table.
	pop	bp		;  Reload cache index and move count.
	pop	dx
	jnc s	I_RST4		;  If no XMS errors, check move count.
	call	I_A20D		;  BAD News!  Do "A20 local-disable".
	jmp	I_XErr		;  Go display "XMS init error" & exit!
I_RST4:	sub	dx,80		;  More search-table indexes to set?
	ja s	I_RST1		;  Yes, loop back and do next 80.
	call	I_A20D		;  Issue "A20 local-disable" request.
	endif
	xor	ax,ax		;Point ES-reg. to low memory.
	mov	es,ax
	mov	al,es:[HDWRFL]	;Get BIOS "hardware installed" flag.
	test	al,001h		;Any diskettes on this system?
	jz s	I_HDCh		;No, scan for available hard-disks.
	mov	al,es:[MCHDWFL]	;Use diskette media-change bits
	and	al,011h		;  for our number of diskettes.
I_FScn:	test	al,001h		;Can next unit signal media-changes?
	jz s	I_FMor		;No?  CANNOT use this old "clunker"!
	push	ax		;Save our diskette counter.
	mov	al,0C0h		;Get "diskette" device-type flags.
	call	I_CHSD		;Get and save diskette's CHS values.
	pop	ax		;Reload our diskette counter.
	inc	@LastU		;Bump unit-table index.
I_FMor:	inc	HDUnit		;Bump BIOS unit number.
	inc	UnitNo.lb	;Bump error-message unit number.
	shr	al,4		;Another diskette to check?
	jnz s	I_FScn		;Yes, loop back and do next one.
I_HDCh:	cmp	BiosHD,0	;Any BIOS hard-disks to use?
	je	I_SCD		;No, scan for CD/DVD units to use.
	mov	HDUnit,080h	    ;Set 1st BIOS hard-disk unit.
	mov	UnitNam.dwd," ksi"  ;Set messages for hard-disks.
	mov	[UnitNo+1].lw,".h"
I_Next:	mov	ah,HDUnit	    ;Set unit no. in error message.
	mov	cx,2
	mov	si,(UnitNo-1-@)
	call	I_HexA
	mov	ah,041h		;Get EDD "extensions" for this disk.
	mov	bx,055AAh
	call	I_In13
	jc s	I_DRdy		;If none, check if disk is "ready".
	cmp	bx,0AA55h	;Did BIOS "reverse" our entry code?
	jne s	I_DRdy		;No, check if this disk is "ready".
	test	cl,007h		;Any "EDD extensions" for this disk?
	jz s	I_DRdy		;No, check if this disk is "ready".
	push	cx		;Save disk "EDD extensions" flags.
	mov	si,(EDDBuff-@)	;Point to "EDD" input buffer.
	mov	[si].lw,30	;Set 30-byte buffer size.
	or	[si+26].dwd,-1	;Init NO "DPTE" data!  A bad BIOS may
				;  NOT post this dword for USB sticks
				;  etc.!  Many Thanks to Daniel Nice!
	mov	ah,048h		;Get this disk's "EDD" parameters.
	call	I_In13
	pop	cx		;Reload disk "EDD extensions" flags.
	jc s	I_Drdy		;If error, check if disk is "ready".
	test	[si+2].lb,004h	;Is this HARD disk flagged "removable"?
	jnz s	I_IgnJ		;If so, we have NO logic to SUPPORT IT!
	cmp	EFlag,0		;Are we running under a DOS "emulator"?
	jne s	I_DRdy		;Yes, check if this disk is ready.
	cmp	[si].lw,30	;Did we get at least 30 bytes?
	jb s	I_DRdy		;No, check if this disk is "ready".
	test	cl,004h		;Does this disk provide "DPTE" data?
	jz s	I_DRdy		;No, check if this disk is "ready".
	cmp	[si+26].dwd,-1	;"Null" drive parameter-table pointer?
	jne s	I_DPTE		;No, go validate "DPTE" checksum.
I_DRdy:	mov	ah,010h		;Non-UltraDMA:  Check for disk "ready".
	call	I_In13
	jc s	I_IgnJ		;If "not ready", must ignore this disk!
	call	I_CHSB		;Get and save this disk's CHS values.
	jmp	I_NoUD		;Go bump number of non-UltraDMA disks.
I_DPTE:	les	si,[si+26]	;Get this disk's "DPTE" pointer.
	mov	bx,15		;Calculate "DPTE" checksum.
	xor	cx,cx
I_CkSm:	add	cl,es:[bx+si]
	dec	bx
	jns s	I_CkSm
	jcxz	I_EDOK		;If checksum O.K., use parameters.
	mov	dx,(EDDMsg-@)	;Display "EDD BIOS ERROR" message.
	call	I_Msg
	mov	dx,(UnitMsg-@)
	call	I_Msg
I_IgnJ:	jmp	I_IgnD		;Ignore this disk and check for more.
I_EDOK:	call	I_CHSB		;Get and save this disk's CHS values.
	mov	bx,00010h	;Initialize IDE unit number index.
	and	bl,es:[si+4]
	shr	bl,4
	mov	ax,es:[si]	;Get disk's IDE base address.
	mov	CtlrNo.lb,'0'	;Reset display to "Ctlr. 0".
	mov	si,(Ctl1Tbl-@)	;Point to IDE address table.
I_ITbl:	cmp	[si].lw,-1	;Is this IDE table active?
	je s	I_NoUD		;No, go bump non-UltraDMA disk count.
	cmp	ax,[si+2]	;Is disk on this primary channel?
	je s	I_ChMS		;Yes, set disk channel and unit.
	inc	bx		;Adjust index for secondary channel.
	inc	bx
	cmp	ax,[si+4]	;Is disk on this secondary channel?
	je s	I_ChMS		;Yes, set disk channel and unit.
	inc	bx		;Adjust values for next controller.
	inc	bx
	add	si,CTLTSIZ
	inc	CtlrNo.lb
	cmp	si,(CtlTEnd-@)	;More IDE controllers to check?
	jb s	I_ITbl		;Yes, loop back and check next one.
	jmp s	I_NoUD		;Go bump non-UltraDMA disk count.
I_ChMS:	push	bx		;Save disk's caching unit number.
	mov	IdeDA,ax	;Save disk's base IDE address.
	add	ax,CDSEL	;Point to IDE device-select register.
	xchg	ax,dx
	mov	al,bl		;Get drive-select command byte.
	shl	al,4
	or	al,LBABITS
	out	dx,al		;Select master or slave disk.
	push	ax		;Save drive-select and caching unit.
	push	bx
	mov	dx,(CtlMsg-@)	;Display IDE controller number.
	call	I_Msg
	pop	ax		;Reload caching unit number.
	mov	dx,(PriMsg-@)	;Point to "Primary" channel name.
	test	al,002h		;Is this a primary-channel disk?
	jz s	I_PRNm		;Yes, display "Primary" channel.
	mov	dx,(SecMsg-@)	;Point to "Secondary" channel name.
I_PRNm:	call	I_Msg		;Display disk's IDE channel name.
	pop	ax		;Reload drive-select byte.
	mov	dx,(MstMsg-@)	;Point to "master" disk name.
	test	al,010h		;Is this disk the master?
	jz s	I_MSNm		;Yes, display "master" name.
	mov	dx,(SlvMsg-@)	;Point to "slave" disk name.
I_MSNm:	call	I_Msg		;Display disk's master/slave name.
	call	I_ValD		;Validate disk as an UltraDMA unit.
	pop	ax		;Reload caching unit number.
	jc s	I_UDEr		;If any errors, display message.
	mov	bx,@LastU	;Change this disk's device-type to
	mov	[bx+TypeF-@],al	;  "IDE" and include channel/unit.
	jmp s	I_More		;Go check for any more BIOS disks.
I_UDEr:	call	I_Msg		;NOT UltraDMA -- Display error msg.
I_NoUD:	mov	cx,USRPtr	;"Stand alone" driver requested?
	jcxz	I_IgnD		;Yes, ignore disk and scan for more.
	inc	NonUCt		;Bump number of non-UltraDMA disks.
	cmp	NonUCt,'9'	;Over 9 non-UltraDMA hard disks?
	jbe s	I_More		;No, check for more disks.
	mov	NonUCt.lw,"+9"	;Set 9+ non-UltraDMA count.
I_More:	inc	@LastU		;Bump our unit-table index.
I_IgnD:	inc	HDUnit		;Bump BIOS unit number.
	cmp	@LastU,MAXBIOS	;More entries in our units table?
	jae s	I_SCD		;No, check for any CD/DVD drives.
	dec	BiosHD		;More BIOS disks to check?
	jnz	I_Next		;Yes, loop back and do next one.
I_SCD:	xor	bx,bx		;Zero BX-reg. for CD/DVD logic.
	mov	DNMsg.lw," ,"	;Change start of disk "name" msg.
	mov	CtlrNo.lb,'0'-1	;Reset display controller number.
I_SCD1:	test	USNdx,001h	;Is this a new I-O channel?
	jnz s	I_SCD2		;No, test for new controller.
	add	DVDTblB,4	;Bump to next I-O addresses.
I_SCD2:	cmp	USNdx,004h	;Is this a new controller?
	jb s	I_SCD3		;No, get unit's I-O addresses.
	inc	CtlrNo.lb	;Bump display controller number.
	mov	USNdx,bl	;Reset unit-select index.
I_SCD3:	mov	si,DVDTblB	;Load I-O parameter pointer.
	cmp	si,(DVDTEnd-@)	;Any more IDE units to check?
	jae s	I_AnyN		;No, check for any drives to use.
	mov	al,USNdx	;Set indicated unit-select byte.
	shl	al,4
	or	al,MSEL
	mov	USelB,al
	inc	USNdx		   ;Bump unit-select index.
	mov	eax,[si]	   ;Get this unit's I-O addresses.
	cmp	ax,-1		   ;Any more controllers to check?
	je s	I_AnyN		   ;No, check for any drives to use.
	mov	[bx+DMAAd-@],eax   ;Set this unit's I-O addresses.
	cmp	NoDMA,bl	   ;Is all UltraDMA disabled?
	jne s	I_SCD4		   ;Yes, default to no DMA address.
	or	ax,ax		   ;"Legacy IDE" with no DMA ctlr.?
	jnz s	I_SCD5		   ;No, go validate this unit.
I_SCD4:	or	[bx+DMAAd-@].lw,-1 ;Invalidate UltraDMA address.
I_SCD5:	call	I_VCD		   ;Validate unit as ATAPI CD/DVD.
	jc s	I_SCD1		   ;If invalid, merely ignore it.
	mov	si,UTblP	   ;Update unit-table parameters.
	mov	eax,[bx+DMAAd-@]
	mov	[si],eax
	mov	al,USelB
	mov	[si+6],al
	add	si,20		   ;Update unit-table pointer.
	mov	UTblP,si
	inc	[bx+CDUnits-@].lb  ;Bump number of active units.
	inc	CDMsgNo		   ;Bump display unit number.
	cmp	si,(UTblEnd-@)	   ;Can we install another drive?
	jb	I_SCD1		   ;Yes, loop back & check for more.
I_AnyN:	cmp	NonUCt.lb,'0'	;Do we have any non-UltraDMA disks?
	je s	I_AnyD		;No, check for any drives to use.
	mov	dx,(NonUMsg-@)	;Display "Non-UltraDMA disks" msg.
	call	I_Msg
I_AnyD:	mov	dx,(NDMsg-@)	;Point to "Nothing to use" message.
	mov	al,CDUnits	;Get number of CD/DVD drives.
	or	al,al		;Do we have any CD/DVD drives to use?
	jnz s	I_20LE		;Yes, see if we will load in the HMA.
	or	al,@LastU.lb	   ;Any BIOS disk or diskette to use?
	mov	eax,[Suffix+2].dwd ;(Post driver's "No CD/DVD" name).
	mov	DvrNam.dwd,eax
	mov	[DvrNam+4].dwd,"   $"
	jz	I_Err		;None?  Display error msg. and exit!
I_20LE:	ifndef	PMDVR		;For UIDE only --
	shr	HFlag,1		;  Will we be loading in the HMA?
	jz	I_Done		;  No, go post "Init" packet results.
	else			;For UIDE2 only --
	cmp	HFlag,0		;  Will we be needing any HMA space?
	je	I_Done		;  No, go post "Init" packet results.
	endif
	mov	ah,005h		;Issue "A20 local-enable" request.
	call	I_XMS
I_A20E:	mov	eax," 02A"	;Get "A20" error-message prefix.
	jnz s	I_HMAX		;If any "A20" error, bail out QUICK!!
	mov	ax,04A02h	;Request needed memory in the HMA.
	ifndef	PMDVR
	mov	bx,HMASize
	else
	mov	bx,@TC1
	shl	bx,1
	add	bx,HMASize
	endif
	call	I_In2F
	mov	ax,es		;Get returned HMA segment address.
	cmp	ax,-1		;Is segment = 0FFFFh?
	jne s	I_HMA1		;No, VERY unusual but not an error.
	cmp	ax,di		;Is offset also = 0FFFFh?
	jne s	I_HMA1		;No, set binary-search table addr.
	call	I_A20D		;BAAAD News!  Do "A20 local-disable".
	mov	eax," AMH"	;Get "HMA" error-message prefix.
I_HMAX:	jmp	I_VErr		;Go display error message & exit!
I_HMA1:	ifdef	PMDVR		;For UIDE2 only --
	mov	dx,(ScnD-SrchD)	;  Use "ScnD" to do cache deletions.
	add	@SrchD1,dx
	add	@SrchD2,dx
	mov	dx,di		;  Set HMA search-table address.
	add	dx,HMASize
	mov	STblAdr.lw,dx
	mov	STblAdr.hw,ax
	mov	cx,@TC1		;  Set HMA search-table limit.
	shl	cx,1
	add	cx,dx
	mov	@TE1,cx
	mov	cx,@MP1		;  Set HMA search-midpoint addresses.
	dec	cx
	dec	cx
	add	cx,dx
	mov	@MP2,cx
	mov	@MP4,cx
	shl	HFlag,1		;  Is this our "large cache" driver?
	jc s	I_HMA2		;  Yes, do "A20 local disable" & exit.
	db	066h,0BAh	;  Use "call SiGet2" & access binary-
	mov	ax,cs:[bp]	;    search table via CS-reg. for /H.
	mov	SIGet2.dwd,edx
	mov	dx,(SIGet2-SIGet)
	add	@SIGet1,dx
	add	@SIGet2,dx
	endif
	push	ax		;Save starting HMA segment:offset.
	push	di
	movzx	eax,ax		;Get 32-bit HMA segment address.
	shl	eax,4
	movzx	ecx,di		   ;Adjust real-mode GDT base addr.
	add	cx,(OurGDT-HDReq)
	add	ecx,eax
	mov	GDTPAdr,ecx
	ifdef	@MBAddr
	add	ecx,(MemBuf-OurGDT) ;Adjust search memory-buffer addr.
	mov	@MBAddr,ecx
	endif
	pop	eax		  ;Set disk entry routine pointer.
	mov	@HDMain,eax
	add	ax,(CDReq-HDReq)  ;Set CD/DVD entry routine pointer.
	mov	@CDMain,eax
	lea	ax,[di-(HDReq-@)] ;Get starting HMA logic offset.
	add	@TypeF,ax	  ;Adjust disk "CHS tables" offsets.
	add	@CHSec,ax
	add	@CHSHd,ax
	add	@CtlTbl,ax	;Adjust disk controller-table ptr.
	add	@GDTP,ax	;Adjust "MvData" routine offsets.
	add	@MvDesc,ax
	add	@UTable,ax	  ;Adjust CD/DVD table pointers.
	add	@DspTB2,ax
	add	@DspTB3,ax
	add	@DspTB4,ax
	add	@DspOfs,ax	;Adjust CD/DVD dispatch offset.
	ifdef	@MBOffs
	add	@MBOffs,ax	;Adjust binary-search buffer address.
	endif
	mov     cx,HMASize      ;Move required logic up to the HMA.
	mov	si,(HDReq-@)
	les	di,@HDMain
	rep	movsb
I_HMA2:	call	I_A20D		;Issue "A20 local-disable" request.
I_Done:	mov	ax,VDSLn.lw	;Done!  Set initial stack pointer.
	mov	@Stack,ax
	les	bx,RqPkt	;Set results in DOS "Init" packet.
	mov	es:[bx].RPLen,ax
	mov	es:[bx].RPStat,RPDON
	xor	bx,bx		;Set up to clear driver's stack.
	xchg	ax,bx
	mov	cx,STACK
	jmp	I_ClrS		;Go clear driver's stack and exit.
;
; Init subroutine to test for and set up a PCI disk controller.
;
I_PCIX:	ret			;(Local exit, used below).
I_PCIC:	mov	IdeDA,ax	;Save subclass & function codes.
	and	LBABuf.hw,0	     ;Reset PCI device index.
I_PCI1:	cmp	RqIndex,(CtlTEnd-@)  ;More space in address tables?
 	jae s	I_PCIX		     ;No, go exit above.
	mov	ax,IdeDA	;Test PCI class 1, subclass/function.
	mov	ecx,000010003h	;(Returns bus/device/function in BX).
	xchg	ax,cx
	mov	si,LBABuf.hw
	call	I_In1A
	jc s	I_PCIX		;Controller not found -- exit above.
	inc	LBABuf.hw	;Bump device index for another test.
	xor	di,di		;Get controller Vendor & Device I.D.
	call	I_PCID
	push	ecx		;Save Vendor and Device I.D.
	mov	di,32		;Save DMA controller base address.
	call	I_PCID
	xchg	ax,cx
	and	al,0FCh
	mov	DMAAd,ax
	mov	si,(PCMsg2-@)	;Set controller address in msg.
	call	I_Hex
	mov	si,(PCMsg3-@)	;Set vendor I.D. in message.
	pop	ax
	call	I_Hex
	pop	ax		;Set Device I.D. in message.
	call	I_Hex
	mov	di,4		;Get low-order PCI command byte.
	call	I_PCID
	not	cl		;Get "Bus Master" & "I-O Space" bits.
	and	cl,005h		;Is controller using both BM and IOS?
	jz s	I_PCI2		;Yes, save this controller's data.
	mov	dx,(BCMsg-@)	;Display "BAD controller!", and hope
	call	I_Msg		;  our user can find a better BIOS!
	mov	dx,(PCMsg1-@)
	call	I_Msg
I_PCIJ:	jmp s	I_PCI1		;Go test for more same-class ctlrs.
I_PCI2:	mov	si,RqIndex	;Get current I-O address table ptr.
	mov	ax,DMAAd	;Set controller DMA base address.
	mov	[si],ax
	test	LBABuf.lb,001h	;Is this a "Native PCI" controller?
	jz s	I_PCI3		;No, set CD/DVD table addresses.
	mov	di,16		;Set primary-channel base address.
	call	I_PCID
	and	cl,0FCh
	mov	[si+2],cx
	mov	di,24		;Set secondary-channel base address.
	call	I_PCID
	and	cl,0FCh
	mov	[si+4],cx
I_PCI3:	cld			;Ensure FORWARD "string" commands.
	mov	di,DVDTblA	;Point to CD/DVD addresses table.
	push	cs
	pop	es
	lodsd			;Set CD/DVD primary addresses.
	stosd
	add	ax,8		;Set CD/DVD secondary addresses.
	stosw
	movsw
	mov	DVDTblA,di	;Bump CD/DVD table pointer.
	mov	dx,(PCMsg-@)	;Display all controller data.
	call	I_Msg
	inc	[PCMsg+3].lb	;Bump controller number in message.
	add	RqIndex,CTLTSIZ	;Bump controller address-table ptr.
	jmp s	I_PCIJ		;Go test for more same-class ctlrs.
;
; Init Subroutine to "validate" an UltraDMA hard-disk.
;
I_ValD:	xor	bx,bx		;Point ES-reg. to low memory.
	mov	es,bx
	mov	dx,[bx+IdeDA-@]	;Issue "Identify Device" command.
	add	dx,CCMD
	mov	al,0ECh
	out	dx,al
	mov	bx,BIOSTMR	;Point to low-memory BIOS timer.
	mov	cl,STARTTO	;Set I-O timeout limit in CL-reg.
	add	cl,es:[bx]
I_IDly:	cmp	cl,es:[bx]	;Has our command timed out?
	je s	I_DErr		;Yes, set "Identify" message & exit.
	in	al,dx		;Get IDE controller status.
	test	al,BSY+RDY	;Controller or disk still busy?
	jle s 	I_IDly		;Yes, loop back and check again.
	test	al,ERR		;Did command cause any errors?
	jz s	I_PIO		;No, read I.D. data using PIO.
I_DErr:	mov	dx,(IEMsg-@)	;Point to "Identify error" msg.
I_SErr:	stc			;Set carry flag (error) & exit.
	ret
I_PIO:	add	dx,(CDATA-CCMD)	;Point to PIO data register.
	in	ax,dx		;Read I.D. bytes 0 and 1.
	shl	ax,1		;Save "ATA/ATAPI" flag in carry bit.
	mov	cx,27		;Skip I.D. bytes 2-53 and
I_Skp1:	in	ax,dx		;  read I.D. bytes 54-55.
	loop	I_Skp1
	push	cs		;Point to disk-name message.
	pop	es
	mov	di,(DName-@)
	mov	cl,26		;Read & swap disk name into message.
I_RdNm:	xchg	ah,al		;(I.D. bytes 54-93.  Bytes 94-105 are
	stosw			;  also read but are ignored.   Bytes
	in	ax,dx		;  106-107 are left in the AX-reg.).
	loop	I_RdNm
	xchg	ax,bx		;Save "UltraDMA valid" flag in BL-reg.
	mov	cl,35		;Skip I.D. bytes 108-175 &
I_Skp2:	in	ax,dx		;  read I.D. bytes 176-177.
	loop	I_Skp2
	mov	bh,ah		;Save "UltraDMA mode" flags in BH-reg.
	mov	cl,167		;Skip remaining I.D. data.
I_Skp3:	in	ax,dx
	loop	I_Skp3
	mov	dx,(UEMsg-@)	;Point to "is not UltraDMA" message.
	rcr	bl,1		;Shift "ATA/ATAPI" flag into BL-reg.
	and	bl,082h		;ATAPI disk, or UltraDMA bits invalid?
	jle s	I_SErr		;Yes?  Exit & display error message.
	mov	di,(Modes-@)	;Point to UltraDMA mode table.
	or	bh,bh		;Will disk do UltraDMA mode 0?
	jz s	I_SErr		;No?  Exit & display message!
I_NxtM:	cmp	bh,bl		;Will disk do next UltraDMA mode?
	jb s	I_GotM		;No, use current mode.
	inc	di		;Point to next mode table value.
	inc	di
	shl	bl,1		;More UltraDMA modes to check?
	jnz s	I_NxtM		;Yes, loop back.
I_GotM:	mov	si,(DNEnd-@)	;Point to end of disk name.
I_NxtN:	cmp	si,(DName-@)	;Are we at the disk-name start?
	je s	I_Name		;Yes, disk name is all spaces!
	dec	si		;Decrement disk name pointer.
	cmp	[si].lb,' '	;Is this name byte a space?
	je s	I_NxtN		;No, continue scan for non-space.
	inc	si		;Skip non-space character.
	mov	[si].lw," ,"	;End disk name with comma & space.
	inc	si
	inc	si
I_Name:	jmp	I_VC17		;Go display disk name/"mode" & exit.
;
; Init subroutine to set BIOS CHS data for a hard-disk or diskette.
;
I_CHSB:	mov	al,080h		;Get "BIOS disk" device-type.
I_CHSD:	push	ax		;Save unit's device-type flag.
	mov	ah,008h		;Get BIOS CHS data for this unit.
	call	I_In13
	jc s	I_CHSE		;If BIOS error, zero both values.
	and	cl,03Fh		;Get sectors/head value (low 6 bits).
	jz s	I_CHSE		;If zero, ERROR!  Zero both values.
	inc	dh		;Get heads/cylinder (BIOS value + 1).
	jnz s	I_CHST		;If non-zero, save data in our table.
I_CHSE:	xor	cl,cl		;Error!  Zero CHS data for this unit.
	xor	dh,dh
I_CHST:	mov	di,@LastU	;Point to "active units" table.
	mov	al,HDUnit	;Set BIOS unit number in our table.
	mov	[di+Units-@],al
	mov	[di+CHSec-@],cl ;Set unit's CHS data in our table.
	mov	[di+CHSHd-@],dh
	pop	ax		;Reload and set device-type flag.
	mov	[di+TypeF-@],al
	or	cl,cl		;Valid CHS values for this unit?
	jnz s	I_VC7		;Yes, go exit below.
	mov	dx,(CHSMsg-@)	;Display "CHS data error" & exit.
	jmp	I_Msg
;
; Init subroutine to "validate" an IDE unit as an ATAPI CD/DVD drive.
;
I_VCD:	mov	dx,[bx+DMAAd-@]	;Get unit UltraDMA command address.
	test	dl,001h		;Will this unit be using UltraDMA?
	jnz s	I_VC0		;No, just select "master" or "slave".
	in	al,dx		;Ensure any previous DMA is stopped!
	and	al,0FEh
	out	dx,al
I_VC0:	mov	dx,[bx+IdeDA-@]	;Point to IDE device-select register.
	add	dx,6
	mov	al,USelB	;Select IDE "master" or "slave" unit.
	out	dx,al
	ifndef	PMDVR		;Await controller-ready.
	call	ChkTO
	else
	call	I_CkTO
	endif
	jc s	I_VC7		;If timeout, go exit below.
	mov	al,0A1h		;Issue "Identify Packet Device" cmd.
	out	dx,al
	ifndef	PMDVR		;Await controller-ready.
	call	ChkTO
	else
	call	I_CkTO
	endif
	jc s	I_VC7		;If timeout, go exit below.
	test	al,DRQ		;Did we also get a data-request?
	jz s	I_VC6		;No, go set carry & exit below.
	sub	dx,7		;Point back to IDE data register.
	in	ax,dx		;Read I.D. word 0, main device flags.
	and	ax,0DF03h	;Mask off flags for an ATAPI CD/DVD.
	xchg	ax,si		;Save main device flags in SI-reg.
	mov	cx,26		;Skip I.D. words 1-26 (unimportant).
I_VC1:	in	ax,dx
	loop	I_VC1
	mov	di,(CDName-@)	;Point to drive "name" buffer.
	push	cs
	pop	es
	mov	cl,20		;Read & swap words 27-46 into buffer.
I_VC2:	in	ax,dx		;(Manufacturer "name" of this drive).
	xchg	ah,al
	stosw
	loop	I_VC2
	mov	cl,7		;Skip I.D. words 47-52 (unimportant)
I_VC3:	in	ax,dx		;  and read I.D. word 53 into AX-reg.
	loop	I_VC3
	mov	UFlag,al	;Save UltraDMA "valid" flags.
	mov	cl,35		;Skip I.D. words 54-87 (unimportant)
I_VC4:	in	ax,dx		;  and read I.D. word 88 into AX-reg.
	loop	I_VC4
	mov	UMode.lb,ah	;Save posted UltraDMA "mode" value.
	mov	cl,167		;Skip all remaining I.D. data.
I_VC5:	in	ax,dx
	loop	I_VC5
	cmp	si,08500h	;Do device flags say "ATAPI CD/DVD"?
	je s	I_VC8		;Yes, see about UltraDMA use.
I_VC6:	stc			;Set carry flag on (error!).
I_VC7:	ret			;Exit.
I_VC8:	test	UFlag,004h	;Valid UltraDMA "mode" bits?
	jz s	I_VC9		;No, disable drive UltraDMA.
	cmp	UMode.lb,bl	      ;Can drive do mode 0 minimum?
	jne s	I_VC10		      ;Yes, display "Unit n:" msg.
I_VC9:	or	[bx+DMAAd-@].lb,1     ;Disable this drive's UltraDMA.
I_VC10:	mov	dx,(CDMsg-@)	      ;Display "CDn:  " message.
	call	I_Msg
	mov	dx,(PriMsg-@)	      ;Point to "Primary" message.
	test	[bx+IdeDA-@].lb,080h  ;Primary-channel drive?
	jnz s	I_VC11		      ;Yes, display "Primary" msg.
	mov	dx,(SecMsg-@)	      ;Point to "Secondary" message.
I_VC11:	call	I_Msg		   ;Display CD/DVD's IDE channel.
	mov	dx,(MstMsg-@)	   ;Point to "Master" message.
	cmp	USelB,SSEL	   ;Is this drive a "slave"?
	jnz s	I_VC12		   ;No, display "Master".
	mov	dx,(SlvMsg-@)	   ;Point to "Slave" message.
I_VC12:	call	I_Msg		   ;Display "Master" or "Slave".
	mov	si,(CDName+40-@)   ;Point to end of CD/DVD name.
I_VC13:	cmp	si,(CDName-@)	   ;Are we at the vendor-name start?
	je s	I_VC14		   ;Yes, vendor name is all spaces!
	dec	si		   ;Decrement vendor-name pointer.
	cmp	[si].lb,' '	   ;Is this name byte a space?
	je s	I_VC13		   ;No, continue scan for non-space.
	inc	si		   ;Skip non-space character.
	mov	[si].lw," ,"	   ;End disk name with comma/space.
	inc	si		   ;Skip comma and space.
	inc	si
I_VC14:	test	[bx+DMAAd-@].lb,1  ;Will this drive use "PIO mode"?
	jz s	I_VC15		   ;No, get drive's UltraDMA "mode".
	mov	[si].dwd,"OIP"	   ;Set "PIO" after drive name.
	add	si,3
	jmp s	I_VC18		;Go set message terminators.
I_VC15:	mov	cx,UMode	;Initialize UltraDMA "mode" scan.
	mov	di,(Modes-2-@)
I_VC16:	inc	di		;Advance to next UltraDMA "mode".
	inc	di
	shr	cx,1		;Will drive do next "mode"?
	jnz s	I_VC16		;Yes, keep scanning for maximum.
I_VC17:	mov	[si].dwd,"-ATA"	;Set "ATA-" after drive name.
	add	si,4
	mov	ax,[di]		;Set UltraDMA "mode" in message.
	mov	cl,00Fh
	and	cl,al
	call	I_HexA
I_VC18:	mov	[si].dwd,0240A0D2Eh  ;Set message terminators.
	mov	dx,(DNMsg-@)	     ;Display vendor name & "mode".
	call	I_Msg
	clc			;Reset carry (no errors) and exit.
	ret
	ifdef	PMDVR		;For UIDE2 only --
;
; Subroutine to test for CD/DVD I-O timeouts.   This is a "copy" of
;   the "ChkTO" routine above.   It is needed to prevent code-cache
;   errors with UIDE2, which updates its "@TE1", "@MP2", and "@MP4"
;   variables for use with HMA space after it does its CD/DVD init.
;   Thus, UIDE2's CD/DVD init logic MUST NOT call "ChkTO"!!
;
I_CkTO:	mov	ah,CMDTO	;Use 500-msec command timeout.
	mov	es,bx		;Point to low-memory BIOS timer.
	mov	si,BIOSTMR
	add	ah,es:[si]	;Set timeout limit in AH-reg.
I_CkT1:	cmp	ah,es:[si]	;Has our I-O timed out?
	stc			;(If so, set carry flag).
	je s	I_CkTX		;Yes?  Exit with carry flag on.
	mov	dx,[bx+IdeDA-@]	;Read IDE primary status.
	add	dx,7
	in	al,dx
	test	al,BSY		;Is our controller still busy?
	jnz s	I_CkT1		;Yes, loop back and test again.
I_CkTX:	ret			;Exit.  Carry set denotes timeout!
	endif
;
; Subroutines to issue initialization "external" calls.
;
I_A20D:	mov	ah,006h		;"A20 local-disable" -- get XMS code.
I_XMS:	call	@XEntry		;XMS -- issue desired request.
	dec	ax		;Zero AX-reg. if success, -1 if error.
	jmp s	I_IntX		;Restore driver settings, then exit.
I_VDS:	mov	di,(VDSLn-@)	;VDS -- Point to parameter block.
	push	cs
	pop	es
	int	04Bh		;Execute VDS "lock" or "unlock".
	jmp s	I_IntX		;Restore driver settings, then exit.
I_In13:	mov	dl,HDUnit	;BIOS data -- set BIOS unit in DL-reg.
	int	013h		;Issue BIOS data interrupt.
	jmp s	I_IntX		;Restore driver settings, then exit.
I_PCID:	push	bx		;Save PCI bus/device/function codes.
	push	si		;Save IDE address-table pointer.
	mov	al,00Ah		;Set "PCI doubleword" request code.
	call	I_In1A		;Get desired 32-bit word from PCI.
	pop	si		;Reload IDE address-table pointer.
	pop	bx		;Reload PCI bus/device/function.
	ret			;Exit.
I_In1A:	mov	ah,0B1h		;Issue PCI BIOS interrupt.
	int	01Ah
	jmp s	I_IntX		;Restore driver settings, then exit.
I_Msg:	push	bx		;Message -- save our BX-register.
	mov	ah,009h		;Issue DOS "display string" request.
	int	021h
	pop	bx		;Reload our BX-register.
	jmp s	I_IntX		;Restore driver settings, then exit.
I_In21:	int	021h		;General DOS request -- issue Int 21h.
	jmp s	I_IntX		;Restore driver settings, then exit.
I_In2F:	int	02Fh		;"Multiplex" -- issue XMS/HMA request.
I_IntX:	sti			;RESTORE all critical driver settings!
	cld			;(Never-NEVER "trust" external code!).
	push	cs
	pop	ds
	ret			;Exit.
;
; Subroutine to convert a 4-digit hex number to ASCII for messages.
;   At entry, the number is in the AX-reg., and the message pointer
;   is in the SI-reg.   At exit, the SI-reg. is updated and the CX-
;   reg. is zero.
;
I_Hex:	mov	cx,4		;Set 4-digit count.
I_HexA:	rol	ax,4		;Get next hex digit in low-order.
	push	ax		;Save remaining digits.
	and	al,00Fh		;Mask off next hex digit.
	cmp	al,009h		;Is digit 0-9?
	jbe s	I_HexB		;Yes, convert to ASCII.
	add	al,007h		;Add A-F offset.
I_HexB:	add	al,030h		;Convert digit to ASCII.
	mov	[si],al		;Store next digit in message.
	inc	si		;Bump message pointer.
	pop	ax		;Reload remaining digits.
	loop	I_HexA		;If more digits to go, loop back.
	ret			;Exit.
CODE	ends
	end
