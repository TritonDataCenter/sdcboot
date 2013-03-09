	page	59,132
	title	UIDEJR -- DOS UltraDMA Disk/CD/DVD Driver.
;
; This is a basic non-caching UltraDMA driver for DOS disks addressed
; by Int 13h BIOS I-O requests.   UIDEJR handles up to 32 SATA or IDE
; hard-disks on up to 10 "Legacy" or "Native PCI" controllers.   Also
; UIDEJR accepts requests from a "CD Redirector" (MSCDEX etc.) and it
; handles up to 4 SATA, IDE or old "PIO mode" CD/DVD drives.   Driver
; operation and switches are the same as for UIDE, except UIDEJR will
; ignore /B and /S switches, and UIDEJR has its /U8 switch to request
; handling 8 CD/DVD drives, not just 4.
;
; UIDEJR omits unnecessary driver logic if its /N2 "disk only" or /N1
; "CD/DVD only" switches are used.   /N1 also omits UIDEJR's stack as
; a "CD Redirector" always sets a stack for its drivers.   UIDEJR has
; these run-time memory sizes --
;
;    CD/DVD only:	 176 upper-memory, 1856 HMA bytes with /H.  
;			2032 upper-memory bytes with no /H switch.
;                            (When /U8 is used, 176/1936 or 2112).
;
;    Disk only:		 688 upper-memory,  816 HMA bytes with /H.
;			1504 upper-memory bytes with no /H switch.
;
;    Disk and CD/DVD:	 768 upper-memory, 2400 HMA bytes with /H.
;			3168 upper-memory bytes with no /H switch.
;                            (When /U8 is used, 768/2480 or 3248).
;
; UIDEJR can load in "640K" low-memory, instead of upper-memory.   If
; so, /H can still put most of its logic in the HMA, making it useful
; for a small "XMS only" DOS system.
;
; If no XMS manager is loaded, UIDEJR will ignore /H, default to /N3,
; and will load entirely in low-memory.    It then uses BIOS I-O (for
; disks) or "PIO mode" (for CD/DVD drives) when required, for I-O not
; suited to UltraDMA.
;
;
; General Program Equations.
;
	.386p			;Allow use of 80386 instructions.
s	equ	<short>		;Default conditional jumps to "short".
STACK	equ	424		;Hard-disk local stack size.
MAXBIOS	equ	32		;Maximum BIOS hard-disks handled.
RMAXLBA	equ	00006DD39h	;Redbook (audio) maximum LBA value.
COOKSL	equ	2048		;CD/DVD "cooked" sector length.
RAWSL	equ	2352		;CD/DVD "raw" sector length.
CMDTO	equ	00Ah		;CD/DVD 500-msec min. command timeout.
SEEKTO	equ	037h		;CD/DVD 3-second min. "seek"  timeout.
STARTTO	equ	07Fh		;Disk/CD/DVD 7-second startup timeout.
BIOSTMR equ	0046Ch		;BIOS "tick" timer address.
HDISKS	equ	00475h		;BIOS hard-disk count address.
VDSFLAG equ	0047Bh		;BIOS "Virtual DMA" flag address.
HDI_OFS	equ	0048Eh-BIOSTMR	;BIOS hard-disk int. flag "offset".
IXM	equ	4096		;IOCTL transfer-length multiplier.
CR	equ	00Dh		;ASCII carriage-return.
LF	equ	00Ah		;ASCII line-feed.
TAB	equ	009h		;ASCII "tab".
;
; Driver Error Return Codes.
;
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
	db	?		;Media descriptor byte (unused by us).
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
DevIntP	dw	(I_Init-@)	;Device-Interrupt routine pointer.
DvrNam	db	'UDVD1   '	;Driver name for use by SHCDX33E, etc.
	dw	0		;(Reserved).
	db	0		;First assigned drive letter.
CDUnits	db	0		;Number of CD/DVD drives (1 thru 3).
	dw	0		;External-entry pointer, MUST be zero.
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
; VDS Parameter Block and UltraDMA Command-List.
;
VDSLn	dd	(CombLM-@)	;VDS block length.
VDSOf	dd	0		;VDS 32-bit buffer offset.
VDSSg	dd	0		;VDS 16-bit segment (hi-order zero).
IOAdr	dd	0		;VDS/DMA 32-bit buffer address.
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
; General Driver "Data Page" Variables.
;
IOF	db	001h		;I-O busy flag (80h = driver "busy").
VLF	db	0		;VDS lock flag (01h = buffer locked).
DMAAd	dw	0		;DMA status/command register address.
IdeDA	dw	0		;IDE data register address.
RqPkt	dd	0		;Request I-O packet address.
AudAP	dw	0		;CD/DVD audio-start address pointer.
XMF	db	0		;CD/DVD XMS move  flag (01h if so).
DMF	db	0		;CD/DVD DMA input flag (01h if so).
CStack	equ	AudAP.dwd	;Caller's saved stack pointer (shares
				; the 4 prior CD/DVD variable bytes).
CDBM	equ	$		;(CD-only low memory "base" size).
;
; BIOS "Active Unit" Table.
;
Units	db	MAXBIOS dup (0)	;Disk BIOS-unit numbers, 0 = unused.
;
; Disk Int 13h Entry Routine.   For a CHS request, the registers are:
;
;   AH      Request code.  We handle 002h read and 003h write.
;   AL      I-O sector count.
;   CH      Lower 8 bits of starting cylinder.
;   CL      Starting sector and upper 2 bits of cylinder.
;   DH      Starting head.
;   DL      BIOS unit number.   We handle hard-disks, 080h and up.
;   ES:BX   I-O buffer address.
;
; For an LBA request, the registers are:
;
;   AH      Request code.  We handle 042h read and 043h write.
;   DL      BIOS unit number.   We handle hard-disks, 080h and up.
;   DS:SI   Pointer to Device Address Packet ("DAP") as shown above.
;
Entry:	pushf			;Save CPU flags for "QuickX" below &
	push	bp		;  save BP-reg. for unit tests, both
				;  get saved on the CALLER's stack!
	mov	bp,0		;Reset active-units table index.
@LastU	equ	[$-2].lw	;(Last-unit index, set by Init).
NextU:	dec	bp		;Any more active units to check?
	js s	QuickX		    ;No, not for us -- exit quick!
	cmp	dl,cs:[bp+Units-@]  ;Does request unit match table?
	jne s	NextU		    ;No, see if more units remain.
	push	ax		;Save AX-reg. for read/write test.
	and	ah,0BEh		;Mask out LBA & write request bits.
	cmp	ah,002h		;CHS or LBA read/write request?
	pop	ax		;(Reload AX-register, in case not).
	jne s	QuickX		;No, not our request -- exit quick!
	cli			;Disable CPU interrupts.
	bts	cs:IOF.lb,7	;Is this driver currently "busy"?
	jc s	InvalF		;Yes?  Set "invalid function" & exit!
	mov	cs:CStack.lw,sp	;Save caller's stack pointer.
	mov	cs:CStack.hw,ss
	push	cs		;Switch to our driver stack, to avoid
	pop	ss		;  CONFIG.SYS "STACKS=0,0" problems!!
	mov	sp,0		;Set starting stack address, always
@Stack	equ	[$-2].lw	;  upper/DOS memory end (Init set).
	sti			;Re-enable CPU interrupts.
	push	eax		;Save CPU EAX-reg. for 32-bit work.
	push	ds		;Save CPU segment registers.
	push	es
	pusha			;Save all 16-bit CPU registers.
	mov	ah,005h		;Issue "A20 local-enable" request.
	call	A20Req
	jc s	Abandn		;If "A20" failure, ABANDON request!
	popa			;Restore and restack all Int 13h I-O
	pop	es		;  parameters, after "A20 enable" --
	push	es		;  Looks wasteful, but one learns to
	pusha			;  never-NEVER "trust" external code!
	db	0EAh		;Go to main Int 13h request handler.
@HDMain	dd	(HDReq-@)	;(Main entry address, set by Init).
PassRq:	call	A20Dis		;Pass request -- Disable "A20" line.
	call	Reload		;Reload all regs. & caller's stack.
QuickX:	pop	bp		;Reload BP-reg. from caller's stack.
	popf			;Reload CPU flags saved on entry.
	db	0EAh		;Go to next routine in Int 13h chain.
@Prev13	dd	0		;(Prior Int 13h vector, set by Init).
InvalF:	mov	ah,001h		;We are BUSY, you fool!  Set "invalid
	jmp s	GetOut		;  function" code and get out, QUICK!
HDExit:	call	A20Dis		;Exit -- issue "A20 local disable".
	pop	ax		;Reload error code and error flag.
	popf
Abandn:	mov	bp,sp		;Point to saved registers on stack.
	mov	[bp+23],al	;Set error code in exiting EAX-reg.
	call	Reload		;Reload all regs. & caller's stack.
GetOut:	mov	bp,sp		;Set error flag in exiting carry bit.
	rcr	[bp+8].lb,1	;(One of the flags saved by Int 13h).
	rol	[bp+8].lb,1
	pop	bp		;Reload BP-reg. from caller's stack.
	popf			;Discard CPU flags saved on entry.
	iret			;Exit (Int 13h data reloads flags!).
;
; Subroutine to reload all registers and the caller's stack pointer.
;
Reload:	cli			;Disable CPU interrupts.
	pop	es		;Save return address in ES-reg.
	mov	[bx+IOF-@],bx	;Reset "busy" and "VDS lock" flags.
	popa			;Reload all 16-bit CPU registers.
	mov	bp,es		;Move return address into BP-reg.
	pop	es		;Reload CPU segment registers.
	pop	ds
	pop	eax		;Reload CPU EAX-register.
	lss	sp,cs:CStack	;Switch back to caller's stack.
	jmp	bp		;Exit.
;
; Subroutine to issue "A20" local-enable and local-disable requests.
;
A20Dis:	mov	ah,006h		;"A20 local disable" -- get XMS code.
A20Req:	db	09Ah		;Call XMS manager for "A20" request.
@XMgr1	equ	dword ptr $	;(XMS "entry" address, set by Init.
	clc			;  If no XMS, set carry = 0 and set
	mov	ax,00001h	;  AX-register = 1, i.e. no errors).
	sti			;RESTORE all critical driver settings!
	cld			;(Never-NEVER "trust" external code!).
	push	cs
	pop	ds
	xor	bx,bx		;Rezero BX-reg. for relative logic.
	sub	al,1		;Zero AL-reg. if success, -1 if error.
	ret			;Exit.
HDLM	equ	($+STACK)	;End of "hard-disk only" low memory.
;
; DOS "Strategy" Routine, used only for CD/DVD requests.
;
Strat:	push	es		;Save DOS request-packet address.
	push	bx
	pop	cs:RqPkt
	retf			;Exit & await DOS "Device Interrupt".
;
; DOS Device-Interrupt Routine.   This logic handles CD/DVD requests.
;   Our driver stack is not needed as CD Redirectors provide a stack.
;
DevInt:	pushf			;Save all CPU flags.
	pusha			;Save all 16-bit CPU registers.
	push	ds
	push	es
	cli			   ;Disable CPU interrupts.
	lds	si,cs:RqPkt	   ;Get DOS request-packet address.
	mov	[si].RPStat,0810Ch ;Post default "general error".
	bts	cs:IOF.lb,7	   ;Is this driver currently "busy"?
	jc s	DvIntX		   ;Yes?  Reload registers and exit!
	sti			;Re-enable CPU interrupts.
	mov	ah,005h		;Issue "A20 local-enable" request.
	call	A20Req		;(Makes critical settings on exit).
	jc s	DvIntE		;If "A20" failure, go exit below.
	db	09Ah		;Call main CD/DVD request handler.
@CDMain	dd	(CDReq-@)	;(CD/DVD request address, Init set).
	call	A20Dis		;Issue "A20 local disable" request.
DvIntE:	cli			;Disable CPU interrupts.
	mov	[bx+IOF-@],bx	;Reset "busy" and "VDS lock" flags.
DvIntX:	pop	es		;Reload all 16-bit CPU registers.
	pop	ds
	popa
	popf			;Reload CPU flags and exit.
	retf
	db	0,0		;(Unused alignment "filler").
;
; CD/DVD Audio Function Buffer (16 bytes) for most CD/DVD "audio"
;   requests.    The variables below are only for initialization.
;   This buffer is located here so it can be "dismissed" with all
;   CD/DVD logic, when our user specifies the /N2 switch.
;
InBuf	equ	$
UTblP	dw	UTable		;Initialization unit table pointer.
NoDMA	db	0		;"No UltraDMA" flag.
UMode	dw	0		;UltraDMA "mode select" bits.
UFlag	db	0		;UltraDMA "mode valid" flags.
USNdx	db	5		;Unit-select index.
USelB	db	082h		;PCI "modes" and unit-select byte.
DvdTA	dw	(DVD1Tbl-@)	;PCI "scan" address table pointer.
DvdTB	dw	(DVD1Tbl-@)	;CD/DVD setup address table pointer.
BiosHD	db	0FFh		;Number of BIOS hard disks.
HDUnit	db	080h		;Current BIOS unit number.
HFlag	db	0		;"Use HMA space" flag.
PCI_IF	db	082h		;Current PCI "interface bit" value.
CDLSiz	equ	($-A20Dis)	;Size of CD/DVD entry logic.
CDLM	equ	CDBM+CDLSiz	;Size of "CD/DVD only" low memory.
	db	0,0,0,0		;Unused combined-driver "stack zeros".
CombLM	equ	($+STACK)	;End of "combined" low memory.   All
				;  following logic can go in the HMA.
;
; Main hard-disk request handler.
;
HDReq:	shl	ah,1		;Is this an LBA read or write request?
	jns s	ValSC		;No, go validate CHS sector count.
	mov	di,sp		;Point DI-reg. to current stack.
	push	ds		;Save this driver's DS-reg.
	mov	ds,[di+18]	;Reload "DAP" segment into DS-reg.
	cmp	[si].DapBuf,-1	;Check for 64-bit "DAP" I-O buffer.
	mov	al,[si].DapSC	;(Get "DAP" I-O sector count).
	les	dx,[si].DapLBA1	;(Get "DAP" LBA bits 16-47).
	mov	di,es
	les	cx,[si].DapBuf	;(Get "DAP" I-O buffer address).
	mov	si,[si].DapLBA	;(Get "DAP" LBA bits 0-15).
	pop	ds		;(Reload this driver's DS-reg.).
	jne s	ValSC		;If no 64-bit buffer, check sector ct.
Pass:	push	ss		;Return to request "pass" routine.
	push	(PassRq-@)
	retf
ValSC:	dec	al		;Is sector count zero or over 128?
	js s	Pass		;Yes?  Let BIOS handle this "No-No!".
	inc	ax		;Restore sector count -- LBA request?
	js s	ZeroBX		;Yes, go zero driver's BX-reg.
	xchg	ax,cx		;CHS -- save request code and sectors.
	xor	di,di		;Reset upper LBA address bits.
	mov	si,0003Fh	;Set SI-reg. to starting sector.
	and	si,ax
	dec	si
	shr	al,6		;Set AX-reg. to starting cylinder.
	xchg	al,ah
	xchg	ax,dx		   ;Swap cylinder & head values.
	mov	al,cs:[bp+CHSec-@] ;Get disk CHS sectors/head value.
@CHSec	equ	[$-2].lw
	or	al,al		   ;Were disk CHS values legitimate?
	jz s	Pass		   ;No?  Let BIOS have this request!
	push	ax		   ;Save CHS sectors/head value.
	mul	ah		   ;Convert head to sectors.
	add	si,ax		   ;Add result to starting sector.
	pop	ax		   ;Reload CHS sectors/head value.
	mul	cs:[bp+CHSHd-@].lb ;Convert cylinder to sectors.
@CHSHd	equ	[$-2].lw
	mul	dx
	add	si,ax		   ;Add to head/sector value.
	adc	dx,di
	xchg	ax,bx		   ;Get buffer offset in AX-register.
	xchg	ax,cx		   ;Swap offset with request/sectors.
	nop			   ;(Unused alignment "filler").
ZeroBX:	xor	bx,bx		   ;Zero BX-reg. for relative logic.
	mov	[bx+VDSOf-@],cx    ;Set VDS I-O buffer address.
	mov	[bx+VDSOf+2-@],bx
	mov	[bx+VDSSg-@],es
	mov	[bx+LBA-@],si	   ;Set 48-bit logical block address.
	mov	[bx+LBA+2-@],dl
	mov	[bx+LBA2-@],dh
	mov	[bx+LBA2+1-@],di
	mov	bp,cs:[bp+CtlUn-@] ;Get disk controller/unit numbers.
@CtlUn	equ	[$-2].lw
	mov	si,offset (Ctl1Tbl-@)  ;Point to disk I-O addresses.
@CtlTbl	equ	[$-2].lw
	mov	cx,0007Ch
	and	cx,bp
	add	si,cx
	shr	cx,1
	add	si,cx
	push	cs:[si].dwd	   ;Set controller base address and
	pop	[bx+DMAAd-@].dwd   ;  primary-channel data address.
	rcr	bp,2		   ;Primary channel I-O request?
	jnc s	GetCmd		   ;Yes, get LBA28 or LBA48 commands.
	add	[bx+DMAAd-@].lw,8  ;Use secondary DMA controller.
	mov	cx,cs:[si+4]	   ;Set secondary channel data addr.
	mov	[bx+IdeDA-@],cx
GetCmd:	shr	dx,12		   ;Shift out LBA bits 16-27.
	or	di,dx		   ;Anything in LBA bits 28-47?
	jz s	LBA28		   ;No, use LBA28 read/write command.
	shl	ah,3		   ;LBA48 -- get command as 020h/030h.
	jmp s	SetCmd		   ;Go set LBA and IDE command bytes.
LBA28:	xchg	dh,[bx+LBA2-@]	   ;LBA28:  Reload & reset bits 24-27.
	or	ah,(DRCMD+1)	   ;Get LBA28 read/write command + 5.
SetCmd:	shl	bp,1		   ;Shift master/slave into LBA cmds.
	mov	dl,(LBABITS/32)
	rcl	dl,5
	or	dl,dh		   ;"Or" in LBA28 bits 24-27 (if any).
	mov	dh,005h		   ;Get final IDE command byte.
	xor	dh,ah		   ;(LBA28 C8h/CAh, LBA48 25h/35h).
	mov	[bx+DSCmd-@],dx	   ;Set LBA and IDE command bytes.
	mov	[bx+SecCt-@],al	   ;Set disk I-O sector count.
	mov	[bx+VDSLn-@],bl	   ;Set VDS and DMA buffer lengths.
	mov	[bx+IOLen-@],bl    ;  We set bit 16 for 65536 bytes,
	mov	ah,0		   ;  since some "odd" chips may NOT
	shl	ax,1		   ;  may NOT run O.K. without it!
	mov	[bx+VDSLn+1-@],ax
	mov	[bx+IOLen+1-@],ax
	call	VDLock		   ;Do VDS "lock" on user buffer.
	jc	Pass		   ;VDS error:  Pass this request!
	call	SetAdr		   ;Set user or Main XMS buffer addr.
	jnc s	DirDMA		     ;If user buffer, do direct DMA.
@NoXM1:	test	[bx+IOCmd-@].lb,012h ;Buffered I-O:  Write request?
	jnz s	BufOut		     ;Yes, use output routine below.
	call	DoDMA		;Buffered input -- read data to XMS.
	jc s	HDDone		;If error, post return code and exit.
	call	XMSMov		;Move data from XMS to user buffer.
	jmp s	HDDone		;Go post any return code and exit.
	db	0		;(Unused alignment "filler").
BufOut:	call	XMSMov		;Buffered output -- move data to XMS.
	jc s	HDDone		;If error, post return code and exit.
DirDMA:	call	DoDMA		;Do direct DMA or buffered output.
HDDone:	pushf			;Save error flag (carry) and code.
	push	ax
	call	VDUnlk		;Do VDS "unlock" of user I-O buffer.
	push	ss		;Return to hard-disk exit routine.
	push	(HDExit-@)
	retf
;
; Subroutine to execute hard-disk UltraDMA read and write requests.
;
DoDMA:	call	CLXMov		;Move DMA command-list up to XMS.
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
	mov	cx,((STARTTO*256)+FLT)	;Get timeout & "fault" mask.
	add	ch,es:[si]		;Set timeout limit in CH-reg.
	call	ChkRdy		      ;Await controller/disk ready.
	jc s	DoDMAX		      ;If any errors, exit below!
	test	[bx+IOCmd-@].lb,012h  ;Is this a write request?
	jnz s	SetDMA		      ;Yes, init DMA transfer.
	mov	al,008h		;Get "DMA read" command bit.
SetDMA:	push	si		;Save BIOS timer pointer.
	call	IniDMA		;Initialize this DMA I-O transfer.
	mov	ax,001F7h	;Set IDE parameter-output flags.
NxtPar:	lea	dx,[di+CDATA+1]	;Point to IDE sector count reg. - 1.
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
DoDMAX:	ret			;End of request -- exit.
ErrDMA:	mov	al,DMAERR	;BAAAD News!  Post DMA error code.
	stc			;Set carry flag (error!) and exit.
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
; Hard Disk Parameter Tables.
;
CtlUn	db   MAXBIOS dup (0)	;Controller and unit number.
CHSec	db   MAXBIOS dup (0)	;Sectors-per-head table.
CHSHd	db   MAXBIOS dup (0)	;Heads-per-cylinder table.
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
CDHMA	equ	$		   ;Start of CD-only HMA routines.
;
; Fixed Command-List XMS Block for moving our DMA command-list to XMS.
;
CLXBlk	dd	8		;Move length      (always 8 bytes).
	dw	0		;Source "handle"     (always zero).
CLXSA	dd	(IOAdr-@)	;Source address (seg. set by Init).
CLXDH	dw	0		;Destination handle  (set by Init).
CLXDA	dd	0		;Destination offset  (set by Init).
;
; User-Data XMS Block, used to move data to/from our XMS buffer.
;
XMSBlk	dd	0		;Buffer length.
XMSSH	dw	0		;Source handle (0 = segment/offset).
	dd	0		;Source address or offset.
XMSDH	dw	0		;Destination handle.
	dd	0		;Destination address or offset.
;
; Subroutine to move our DMA command-list up to XMS memory for I-O.
;   This avoids a DMA "HANG" if we loaded into UMBPCI "Shadow RAM"!
;
CLXMov:	mov	si,(CLXBlk-@)	;Point to command-list XMS block.
CLXPtr	equ	[$-2].lw
	push	cs
	pop	ds
	jmp s	XMSGo		;Move command-list to XMS and exit.
	db	0		;(Unused alignment "filler").
;
; Subroutine to move user I-O data to or from our XMS buffer.
;
XMSMov:	push	[bx+VDSSg-@].lw	 ;Save user-buffer address.
	push	[bx+VDSOf-@].lw
	mov	eax,[bx+VDSLn-@] ;Get user-buffer length.
	mov	si,(XMSBlk-@)	 ;Point to user-data XMS block.
XMSPtr	equ	[$-2].lw
	push	cs
	pop	ds
	mov	[si],eax	;Set user-buffer length.
	lea	bx,[si+4]	;Set move indexes for a read.
	lea	di,[si+10]
	jz s	XMSSet		;Is this a read request?
	xchg	bx,di		;No, "swap" indexes for a write.
XMSSet:	mov	[bx].lw,0	;Set our XMS buffer "handle" & offset.
@XMHdl	equ	[$-2].lw
	mov	[bx+2].dwd,0
@XMOffs	equ	[$-4].dwd
	and	[di].lw,0	;Set user buffer "handle" to zero.
	pop	[di+2].dwd	;Set user-buffer address.
	nop			;(Unused alignment "filler").
XMSGo:	mov	ah,00Bh		;Set XMS move-memory request code.
XMSReq:	db	09Ah		;Call XMS manager to do our request.
@XMgr2	equ	dword ptr $	;(XMS "entry" address, set by Init.
	clc			;  If no XMS, set carry = 0 and set
	mov	ax,00001h	;  AX-register = 1, i.e. no errors).
	sub	al,1		;Zero AL-reg. if success, -1 if error.
XMSRst:	sti			;RESTORE all critical driver settings!
	cld			;(Never-NEVER "trust" external code!).
	mov	bx,0		;Reload this driver's DS-register.
@DSReg	equ	[$-2].lw	;(Driver DS-reg. address, Init set).	
	mov	ds,bx
	mov	bx,0		;Rezero BX-reg. but save carry flag.
	ret			;Exit.
;
; Subroutine to issue a VDS "lock" request.
;
VDLock:	or	[bx+IOAdr-@].dwd,-1  ;Invalidate VDS buffer address.
	mov	ax,08103h	     ;Get VDS "lock" parameters.
	mov	dx,0000Ch
	mov	di,(VDSLn-@)	     ;Point to VDS parameter block.
	push	ds
	pop	es
	int	04Bh		     ;Issue VDS "lock" request.
	call	XMSRst		     ;Restore all critical settings.
	jc s	VDLokX		     ;VDS error:  Exit immediately!
	cmp	[bx+IOAdr-@].dwd,-1  ;Is VDS address still all-ones?
	jb s	VDLokF		     ;No, go set VDS "lock" flag.
	movzx	eax,[bx+VDSSg-@].lw  ;VDS logic is NOT present --
	shl	eax,4		     ;  set 20-bit buffer address.
	add	eax,[bx+VDSOf-@]
	mov	[bx+IOAdr-@],eax
	nop			     ;(Unused alignment "filler").
VDLokF:	adc	[bx+VLF-@],bl	     ;Set VDS "lock" flag from carry.
VDLokX:	ret			     ;Exit.
;
; Subroutine to set the 32-bit DMA address for an UltraDMA transfer.
;
SetAdr:	test	[bx+IOAdr-@].lb,003h ;Is user buffer 32-bit aligned?
	jnz s	SetAd1		     ;No, use our XMS memory buffer.
	cmp	[bx+IOAdr-@].hw,009h ;Is DMA beyond our 640K limit?
	ja s	SetAd1		     ;Yes, use our XMS memory buffer.
	mov	cx,[bx+IOLen-@]	     ;Get (length - 1) + I-O address.
	dec	cx
	add	cx,[bx+IOAdr-@].lw   ;Will I-O cross a 64K boundary?
	jnc s	SetAdX		     ;No, user-buffer DMA is O.K.
SetAd1:	stc			     ;Set carry to denote our buffer.
	mov	[bx+IOAdr-@].dwd,0   ;Set our 32-bit XMS buffer addr.
@XBAddr	equ	[$-4].dwd	     ;(XMS buffer address, Init set).
SetAdX:	ret			     ;Exit.
	db	0		     ;(Unused alignment "filler").
;
; Subroutine to initialize the next disk/CD/DVD UltraDMA I-O transfer.
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
; Subroutine to issue a VDS "unlock" request.
;
VDUnlk:	shr	[bx+VLF-@].lb,1	;Was user buffer "locked" by VDS?
	jnc s	VDUnlX		;No, just go exit below.
	mov	ax,08104h	;Get VDS "unlock" parameters.
	xor	dx,dx
	mov	di,(VDSLn-@)	;Point to VDS parameter block.
	push	ds
	pop	es
	int	04Bh		;Issue VDS "unlock" request.
VDUnlX:	ret			;Exit.
HMALEN1	equ	($-HDReq)	;Length of hard-disk only HMA logic.
;
; CD/DVD Dispatch Table for DOS request codes 128 thru 136.   Each
;   entry in the following tables has 4 bits of "IOCTL byte count"
;   and 12 bits of offset from symbol "USR" for its handler logic.
;   Any handler MUST be located no more than 4K bytes above "USR"!
;
DspTbl2	dw	DspLmt2		;Number of valid request codes.
DspTblB	dw	RqRead-USR	;128 -- Read Long.
	dw	0		;129 -- Reserved	  (unused).
	dw	RqSeek-USR	;130 -- Read Long Prefetch.
	dw	RqSeek-USR	;131 -- Seek.
	dw	RqPlay-USR	;132 -- Play Audio.
	dw	RqStop-USR	;133 -- Stop Audio.
	dw	0		;134 -- Write Long	  (unused).
	dw	0		;135 -- Write Long Verify (unused).
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
; Main CD/DVD request handler.
;
CDReq:	push	eax		 ;Save EAX- & EDX-regs. for CD logic.
	push	edx
	call	ClrPkt		 ;Clear our ATAPI packet area.
	mov	[bx+XMF-@],bx	 ;Reset XMS-move & DMA-input flags.
	les	si,[bx+RqPkt-@]	      ;Reload DOS request packet ptr.
	mov	es:[si.RPStat],RPDON  ;Set packet status to "done".
	mov	al,es:[si+RPSubU].lb  ;Get our CD/DVD drive number.
	mov	ah,20		      ;Get drive's "audio-start" ptr.
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
	je s	CDReqX		      ;"Device Close", ignore & exit.
	cmp	al,3		      ;CD/DVD "IOCTL Input" request?
	je s	Try3D		      ;Yes, do IOCTL Input dispatch.
	cmp	al,12		      ;CD/DVD "IOCTL Output" request?
	jb s	CDReqE		      ;No?  Handle as "unsupported"!
	ja s	CDReqX		      ;"Device Open", ignore & exit.
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
	call	VDUnlk		;"VDS unlock" user buffer if needed.
CDReqX:	pop	edx		;Reload EDS- and EAX-registers.
	pop	eax		 
	retf			;Return to CD/DVD exit logic.
CDReqE:	call	USR		;Error!  Handle as "unsupported".
	jmp s	CDReqX		;Go reload registers and exit.
	db	0		;(Unused alignment "filler").
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
	jc s	RqRL2		      ;"Lock" error:  Use "PIO mode".
	test	[bx+DMAAd-@].lb,001h  ;Is this drive using UltraDMA?
	jnz s	RqRL2		      ;No, go use "PIO mode" input.
	call	SetAdr		      ;Set 32-bit DMA buffer address.
@NoXM2:	adc	[bx+XMF-@].lw,00100h  ;Set XMS-move & DMA-input flags.
	call	CLXMov		      ;Move DMA command-list to XMS.
	jc s	RqRLE		;Failed?  Return "general error"!
RqRL2:	call	DoIO		;Do desired CD/DVD input request.
	jc s	RqRLX		;If any errors, exit below.
	or	[bx+XMF-@],bl	;Do we need an XMS buffer "move"?
	jz s	RqRL3		;No, just exit below.
	xor	ax,ax		;Move data from XMS to user buffer.
	call	XMSMov
	jc s	RqRLE		;Failed?  Return "general error".
RqRL3:	ret			;All is well -- exit.
RqRLE:	mov	al,GENERR	;XMS failure -- post "general error".
RqRLX:	jmp	ReqErr		;Set error code in packet & exit.
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
	nop			;(Unused alignment "filler").
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
DoBuf1:	mov	[bx+PktLn+1-@],al  ;Buffered -- set packet count.
DoBuf2:	mov	[bx+VDSLn-@],al	   ;Set 8-bit data-transfer length.
	mov	[bx+VDSOf-@].lw,(InBuf-@)  ;Use our buffer for I-O.
@InBuf1	equ	[$-2].lw
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
DoIO8:	xchg	ah,cs:[si]	    ;Load & set media-change flag.
	mov	cs:[si+12].lb,0FFh  ;Make last-session LBA invalid.
	dec	ah		    ;Media-change flag already set?
	jnz s	DoIO9		;Yes, set carry flag and exit.
	mov	al,15		;Return "Invalid Media Change".
DoIO9:	stc			;Set carry flag (error!).
	nop			;(Unused alignment "filler").
DoIO10:	pop	es		;Reload ES- and SI-registers.
	pop	si
	mov	di,(InBuf-@)	;For audio, point to our buffer.
@InBuf2	equ	[$-2].lw
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
	db	0		;(Unused alignment "filler").
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
;
; Subroutine to clear our ATAPI "packet" area for the next request.
;
ClrPkt:	mov	[bx+Packet-@],bx    ;Zero 1st 10 ATAPI packet bytes.
	mov	[bx+Packet+2-@],bx  ;(Last 2 are unused "pad" bytes).
	mov	[bx+Packet+4-@],bx
	mov	[bx+Packet+6-@],bx
	mov	[bx+Packet+8-@],bx
	ret			    ;Exit.
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
	db	0,0		;(Unused alignment "filler").
;
; CD/DVD Drive Parameter Tables.   These appear last in the CD/DVD
;   driver routines, so 4 more drives can be added easily when the
;   /U8 switch is given.    If not, the drive parameter tables for
;   CD/DVD unit 4 to 7 (below) are "dismissed" during driver Init.
;
UTable	dw	0FFFFh		;Unit 0 DMA address (set by Init).
	dw	0FFFFh		;	IDE address (set by Init).
	dw	0FFFFh		;	(Unused alignment "filler").
	db	0FFh		;	Device-select (set by Init).
	db	0FFh		;	Media-change flag.
	dd	0FFFFFFFFh	;	Current audio-start address.
	dd	0FFFFFFFFh	;	Current audio-end   address.
	dd	0FFFFFFFFh	;	Last-session starting LBA.
	db	60 dup (0FFh)	;Unit 1 to 3 Drive Parameter Tables.
UTblEnd	equ	$		;End of 1st 4 CD/DVD drive tables.
HMALEN2	equ	($-CDHMA)	;Normal length of CD-only HMA logic.
HMALEN3	equ	($+4-HDReq)	;Normal length of disk/CD HMA logic.
CD4to7	db	0,0,0,0		;Unit 4 to 7 Drive Parameter Tables
	db	76 dup (0FFh)	;  and "stack zeros" if /U8 omitted.
	db	0,0,0,0		;Unused "stack zeros", if /U8 given.
;
; Initialization Variables.
;
PCIDevX	dw	0		;Current PCI "device index".
PCISubC	dw	00100h		;PCI subclass and function code.
CtlrAdP	dw	(Ctl1Tbl-@)	;Disk controller address-table ptr.
UTblLmt	dw	UTblEnd		;CD/DVD parameter table limit.
CDVDSiz	dw	HMALEN2		;Size of "CD/DVD only" HMA logic.
HMASize	dw	HMALEN3		;Size of all HMA driver routines.
XMSRsv	dw	0		;Desired "reserved" XMS memory.
NCFlag	db	0		;"Handle no CD/DVD drives" flag.
NDFlag	db	0		;"Handle no hard-disks" flag.
U8Flag	db	0		;"Handle 8 CD/DVD drives" flag.
	db	0,0,0		;(Unused alignment "filler").
;
; CD/DVD I-O Address Table, used only during Init.    The 1st word
;   of a set is its DMA controller base address, -1 says "unused".
;   A zero in the DVD1 table indicates an old system having no DMA
;   controller whose CD/DVD drives must use "PIO mode" only.   The
;   2nd word is each controller's "legacy" base I-O address.   For
;   "native PCI mode", all base I-O addresses are set during Init.
;   The "extra" table lets old systems with no SATA/IDE controller
;   handle drives using the first "legacy" addresses.   After Init
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
	dd	18 dup (0FFFFh)	;IDE3 to IDE10 & "extra" parameters.
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
;
; Initialization Messages.
;
EDDBuff equ	$
TTLMsg	db	CR,LF,'UIDEJR, 10-16-2011.  CD/DVD name is '
TTLName	db	'         ',CR,LF,0
IBMsg	db	'No V2.0C+ PCI, hard-disks ignored!',CR,LF,'$'
NIMsg	db	'/N3 illegal$'
DNMsg	db	' d'
CDName	db	'isk is '
DName	equ	$
DNEnd	equ	DName+40
BCMsg	db	'BAD$'
PCMsg	db	'IDE0'
PCMsg1	db	' Controller at I-O address '
PCMsg2	db	'    h, Chip I.D. '
PCMsg3	db	'        h.',CR,LF,'$'
EDDMsg	db	'EDD BIOS$'
CHSMsg	db	'CHS'
UnitMsg	db	' data ERROR, Disk '
UnitNo	db	'  h.',CR,LF,'$'
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
PRMsg	db	'No 80386+ CPU'
Suffix	db	'; UIDEJR not loaded!',CR,LF,'$'
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
	lds	bx,cs:RqPkt	;Point to DOS "Init" packet.
	cmp	[bx].RPOp,0	;Is this really an "Init" packet?
	jne s	I_Exit		;No?  Reload regs. and exit QUICK!
	mov	[bx].RPStat,RPDON+RPERR  ;Set "Init" packet defaults.
	and	[bx].RPLen,0
	mov	[bx].RPSeg,cs
	push	cs		;NOW point DS-reg. to this driver!
	pop	ds
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
I_Junk:	mov	dx,(PRMsg-@)	;Point to "No 80386" error message.
I_Quit:	call	I_Msg		;Display "No 80386" and msg. suffix.
I_Exit:	pop	dx		;Reload 16-bit registers we used.
	pop	bx
	pop	ax
	pop	es		;Reload CPU segment registers.
	pop	ds
	popf			;Reload CPU flags and exit.
	retf
I_VErr:	mov	VEMsg.dwd,eax	;Set prefix in "VDS init error" msg.
I_VEr1:	mov	dx,(VEMsg-@)	;Point to "VDS init error" message.
I_Err:	push	dx		;Init ERROR!  Save message pointer.
	shr	VDSOf.lb,1	;Was driver "locked" by VDS?
	jnc s	I_XDis		;No, see if we reserved XMS memory.
	mov	ax,08104h	;"Unlock" this driver from memory.
	xor	dx,dx
	call	I_VDS
I_XDis:	mov	dx,CLXDH	;Load our XMS memory "handle".
	or	dx,dx		;Did we reserve any XMS memory?
	jz s	I_LDMP		;No, reload pointer & display msg.
	mov	ah,00Dh		;Unlock and "free" our XMS memory.
	push	dx
	call	I_XMS
	mov	ah,00Ah
	pop	dx
	call	I_XMS
I_LDMP:	pop	dx		;Reload error message pointer.
	call	I_Msg		;Display desired error message.
	popad			;Reload all 32-bit CPU registers.
I_Suff:	mov	dx,(Suffix-@)	;Display message suffix and exit!
	jmp s	I_Quit
I_Sv32:	pushad			;Save all 32-bit CPU registers.
	les	si,es:[bx].RPCL	;Get command-line data pointer.
	xor	bx,bx		;Zero BX-reg. for relative logic.
I_NxtC:	mov	al,es:[si]	;Get next command-line byte.
	inc	si		;Bump pointer past first byte.
	cmp	al,0		;Is byte the command-line terminator?
	je s	I_TrmJ		;Yes, go check for valid device name.
	cmp	al,LF		;Is byte an ASCII line-feed?
	je s	I_TrmJ		;Yes, go check for valid device name.
	cmp	al,CR		;Is byte an ASCII carriage-return?
I_TrmJ:	je	I_Term		;Yes, go check for valid device name.
	cmp	al,'-'		;Is byte a dash?
	je s	I_NxtS		;Yes, see what next "switch" byte is.
	cmp	al,'/'		;Is byte a slash?
	jne s	I_NxtC		;No, check next command-line byte.
I_NxtS:	mov	ax,es:[si]	;Get next 2 command-line bytes.
	and	al,0DFh		;Mask out 1st byte's lower-case bit.
	cmp	al,'A'		;Is this byte an "A" or "a"?
	jne s	I_ChkH		   ;No, see if byte is "H" or "h".
	mov	al,(ASDATA-0100h)  ;Reverse all "Legacy IDE" addrs.
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
	inc	si		;Point to next command-line byte.
I_ChkH:	cmp	al,'H'		;Is switch byte an "H" or "h"?
	jne s	I_CkN1		;No, see if next 2 bytes are "N1".
	mov	HFlag,al	;Set "use HMA space" flag.
	inc	si		;Point to next command-line byte.
I_CkN1:	cmp	ax,"1N"		;Are these 2 switch bytes "N1"?
	jne s	I_CkN2		;No, see if switch bytes are "N2".
	mov	NDFlag,al	;Set our "No hard-disks" flag.
	mov	BiosHD,bl	  ;Disable all hard-disk handling.
	mov	VDSLn.lw,(CDLM-@) ;Discard all hard-disk routines.
	mov	HMASize,HMALEN2
	inc	si		;Skip these 2 command-line bytes.
	inc	si
I_CkN2:	cmp	ax,"2N"		;Are these 2 switch bytes "N2"?
	jne s	I_CkN3		;No, see if switch bytes are "N3".
	mov	NCFlag,al	;Set our "No CD/DVD drives" flag.
	mov	DvdTB,ax	  ;Disable all CD/DVD handling.
	mov	VDSLn.lw,(HDLM-@) ;Discard all CD/DVD routines.
	mov	HMASize,HMALEN1
	inc	si		;Skip these 2 command-line bytes.
	inc	si
I_CkN3:	cmp	ax,"3N"		;Are these 2 switch bytes "N3"?
	jne s	I_ChkQ		;No, see if byte is "Q" or "q".
	mov	XMF,al		;Set "No XMS memory" flag.
	inc	si		;Skip 1st command-line byte.
I_ChkQ:	cmp	al,'Q'		;Is switch byte a "Q" or "q"?
	jne s	I_ChkR		;No, see if byte is "R" or "r".
	mov	@DRQ.lb,075h	;Enable "data request" tests above.
	inc	si		;Point to next command-line byte.
I_ChkR:	cmp	al,'R'		;Is switch byte an "R" or "r"?
	jne s	I_CkU8		;No, see if 2 bytes are "U8" or "u8".
	inc	si		;Point to next command-line byte.
	mov	ax,es:[si]	;Get next 2 command-line bytes.
	mov	cx,15296	;Get 15-MB XMS memory size.
	cmp	ax,"51"		;Does user want 15-MB XMS reserved?
	je s	I_CkRA		;Yes, set memory size to reserve.
	mov	ch,(64448/256)	;Get 63-MB XMS memory size.
	cmp	ax,"36"		;Does user want 63-MB XMS reserved?
	jne s	I_NxtJ		;No, continue scan for a terminator.
I_CkRA:	mov	XMSRsv,cx	;Set desired XMS memory to reserve.
	inc	si		;Skip these 2 command-line bytes.
	inc	si
I_CkU8:	cmp	ax,"8U"		;Are next 2 bytes "U8" or "u8"?
	jne s	I_ChkU		;No, see if 1st byte is "U" or "u".
	mov	U8Flag,al	;Set "Handle 8 CD/DVD drives" flag.
	inc	si		;Skip these 2 command-line bytes.
	inc	si
I_ChkU:	cmp	al,'U'		;Is this byte a "U" or "u"?
	jne s	I_ChkD		;No, see if 2 bytes are "D:" or "d:".
	inc	si		;Bump pointer past "U" or "u".
	mov	al,ah		;Get 2nd command-line byte.
	and	al,0DFh		;Mask out 1st lower-case bit (020h).
	cmp	al,'X'		;Is 2nd byte an "X" or "x"?
	jne s	I_NxtJ		;No, continue scan for a terminator.
	mov	NoDMA,al	;Set CD/DVD "No UltraDMA" flag.
	inc	si		;Bump pointer past "X" or "x".
I_NxtJ:	jmp	I_NxtC		;Go check next command-line byte.
I_ChkD:	cmp	ax,":D"		;Are next 2 bytes "D:" or "d:"?
	jne s	I_NxtJ		;No, continue scan for a terminator.
	inc	si		;Bump pointer past "device" switch.
	inc	si
	mov	bl,8		;Set driver-name byte count of 8.
	mov	cx,bx
	mov	di,(DvrNam-@)	;Blank out previous driver name.
I_Nam0:	dec	bx
	mov	[bx+DvrNam-@].lb,' '
	jnz s	I_Nam0
I_Nam1:	mov	al,es:[si]	;Get next device-name byte.
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
	jb s	I_Nam2
	cmp	al,'z'
	ja s	I_Nam2
	and	al,0DFh
I_Nam2:	cmp	al,'!'		;Is this byte an exclamation point?
	je s	I_Nam3		;Yes, store it in device name.
	cmp	al,'#'		;Is byte below a pound-sign?
	jb s	I_InvN		;Yes, Invalid!  Zero first byte.
	cmp	al,')'		;Is byte a right-parenthesis or less?
	jbe s	I_Nam3		;Yes, store it in device name.
	cmp	al,'-'		;Is byte a dash?
	je s	I_Nam3		;Yes, store it in device name.
	cmp	al,'0'		;Is byte below a zero?
	jb s	I_InvN		;Yes, invalid!  Zero first byte.
	cmp	al,'9'		;Is byte a nine or less?
	jbe s	I_Nam3		;Yes, store it in device name.
	cmp	al,'@'		;Is byte below an "at sign"?
	jb s	I_InvN		;Yes, invalid!  Zero first byte.
	cmp	al,'Z'		;Is byte a "Z" or less?
	jbe s	I_Nam3		;Yes, store it in device name.
	cmp	al,'^'		;Is byte below a carat?
	jb s	I_InvN		;Yes, invalid!  Zero first byte.
	cmp	al,'~'		;Is byte above a tilde?
	ja s	I_InvN		;Yes, invalid!  Zero first byte.
	cmp	al,'|'		;Is byte an "or" symbol?
	je s	I_InvN		;Yes, invalid!  Zero first byte.
I_Nam3:	mov	[di],al		;Store next byte in device name.
	inc	si		;Bump command-line pointer.
	inc	di		;Bump device-name pointer.
	loop	I_Nam1		;If more name bytes to go, loop back.
	jmp s	I_NxtJ		 ;Go get next command byte.
I_InvN:	mov	[bx+DvrNam-@],bl ;Invalid name!  Zero first byte.
	jmp s	I_NxtJ		 ;Go get next command byte.
I_Term:	mov	ah,'.'		;Set a period for "stores" below.
	mov	cx,8		;Set driver-name byte count of 8.
	mov	di,(DvrNam-@)	;Point to our driver name.
	cmp	[di].lb,' '	;Is driver "name" valid?
	ja s	I_SetN		   ;Yes, set name in "title" message.
	mov	[di+4].dwd,"   1"  ;If not, set "UDVD1" default name.
	mov	[di].dwd,"DVDU"
I_SetN:	mov	al,[di]		;Get next driver "name" byte.
	cmp	al,' '		;End of driver "name"?
	je s	I_8CDs			 ;Yes, check for 8 CDs/DVDs.
	mov	[di+(TTLName-DvrNam)],ax ;Store title byte & period.
	inc	di			 ;Bump driver "name" pointer.
	loop	I_SetN		;If more name bytes to go, loop back.
I_8CDs:	shr	U8Flag,1	;Has user requested 8 CD/DVD drives?
	jz s	I_InqX		;No, inquire about an XMS manager.
	cmp	HMASize,HMALEN1	;Does user also want "disks only"?
	je s	I_InqX		;Yes, ignore his FOOLISH /U8 switch!
	mov	ax,80		;Increase size of "CD/DVD only" or
	add	UTblLmt,ax	;  "combined" drivers by 80 bytes.
	add	CDVDSiz,ax
	add	HMASize,ax
	mov	CD4to7.dwd,-1	;Get rid of leading "stack zeros"
I_InqX:	mov	ax,04300h	;Inquire if we have an XMS manager.
	call	I_In2F
	cmp	al,080h		;Is an XMS manager installed?
	je s	I_SavX		;Yes, go save its "entry" address.
	mov	al,090h		;Set "No XMS memory" flag.
	mov	XMF,al
	mov	A20Req.lb,al	;Disable "A20" enable/disable calls.
	call	I_DisX		;Disable all our use of XMS memory.
	jmp s	I_NoHM		;Go ensure NO use of HMA space.
I_SavX:	mov	ax,04310h	;Save XMS manager "entry" address.
	call	I_In2F
	push	es
	push	bx
	pop	@XMgr1
	shl	HFlag,1		;Will we be loading in the HMA?
	jz s	I_NoHM		;No, go set our normal-memory size.
	mov	ax,04A01h	;Get "free" HMA memory size.
	call	I_In2F
	cmp	bx,HMASize	;Enough "free" HMA for our logic?
	jae s	I_TTL		;Yes, go display driver "title" msg.
I_NoHM:	mov	HFlag,0		;Ensure NO use of HMA space!
	mov	ax,HMASize	;Set "normal memory" driver size.
	add	VDSLn.lw,ax
I_TTL:	mov	si,(TTLMsg-@)	;Point to driver "title" message.
I_TTL1:	lodsb			;Get next byte of message & "swap"
	xchg	ax,dx		;  it into DL-reg. for an Int 21h.
	push	si		;Display next message byte.   If a
	mov	ah,002h		;  dollar-sign is part of a CD/DVD
	call	I_In21		;  name, this special routine will
	pop	si		;  display it properly!
	cmp	[si].lb,0	;Are we at the terminating "null"?
	jnz s	I_TTL1		;No, loop back & display next byte.
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
	mov	ax,cs		;Set driver VDS segment address.
	mov	VDSSg.lw,ax
	mov	@CDMain.hw,ax	;Set "fixed" driver segment values.
	mov	@HDMain.hw,ax
	mov	CLXSA.hw,ax
	mov	@DSReg,ax
	shl	eax,4		;Set 20-bit VDS driver address.
	mov	IOAdr,eax
	cli			      ;Avoid interrupts in VDS tests.
	test	es:[VDSFLAG].lb,020h  ;Are "VDS services" active?
	jz s	I_REnI		      ;No, re-enable CPU interrupts.
	mov	ax,08103h	;"Lock" this driver into memory.
	mov	dx,0000Ch
	call	I_VDS
	jc	I_VEr1		;If "lock" error, display msg. & exit!
	inc	VDSOf.lb	;Set initialization VDS "lock" flag.
I_REnI:	sti			;Re-enable CPU interrupts.
	mov	eax,IOAdr	;Get final driver 32-bit address.
	add	PRDAd,eax	;Set "No XMS memory" PRD address.
	xor	edi,edi		;Get PCI BIOS "I.D." code.
	mov	al,001h
	call	I_In1A
	cmp	edx," ICP"	;Is PCI BIOS V2.0C or newer?
	je s	I_ScnC		;Yes, scan for all IDE controllers.
	mov	dx,(IBMsg-@)	;Display "No V2.0C+ PCI" message.
	call	I_Msg
	jmp s	I_NoXM		;Go see if we need an XMS manager.
I_ScnC:	mov	al,PCI_IF	;Get next "interface bit" value.
	and	ax,00003h
	or	ax,PCISubC	;"Or" in subclass & current function.
	call	I_PCIC		;Test for specific PCI class/subclass.
	rol	PCI_IF,4	;Swap both "interface bit" values.
	mov	al,PCI_IF	;Load next "interface bit" value.
	or	al,al		;Both "interface" values tested?
	jns s	I_ScnC		;No, loop back and test 2nd one.
	add	PCISubC.lb,004h	;More PCI function codes to try?
	jnc s	I_ScnC		;Yes, loop back & try next function.
	cmp	DvdTA,(DVD1Tbl-@)  ;Any CD/DVD controllers found?
	ja s	I_Scn1		;Yes, see about "Native PCI" checks.
	add	DvdTA,DVDTSIZ	;Save "legacy" addresses for oldies!
I_Scn1:	test	al,001h		;Have we tested "Native PCI" ctlrs.?
	mov	PCI_IF,093h	;(Set "Native PCI" interface bits).
	jz s	I_ScnC		;No, loop back and test them, also.
I_NoXM:	shl	XMF,1		;User /N3 given, or no XMS manager?
	jz s	I_XMgr		;No, go set up needed XMS memory.
	mov	dx,(NIMsg-@)	;Point to "/N3 illegal" message.
	cmp	PRDAd.hw,00Ah	;Has user loaded us into upper-memory?
	jae s	I_ErrJ		;Yes, display msg. to the FOOL & exit!
	call	I_DisX		;Disable all our use of XMS memory.
	jmp	I_HDCt		;Go check hard-disk count.
I_XMgr:	mov	eax,@XMgr1	;Set XMS "entry" address in "XMSMov".
	mov	@XMgr2,eax
	mov	dx,XMSRsv	;Get "reserved" XMS memory size.
	or	dx,dx		;Does user want any "reserved" XMS?
	jz s	I_XGet		;No, get driver's actual XMS memory.
	mov	ah,009h		;Get 15-MB or 63-MB "reserved" XMS
	call	I_XMS		;  memory, which we "release" below.
	jnz s	I_XErr		;If error, display message and exit!
	mov	CLXDH,dx	;Save reserved-XMS "handle" number.
I_XGet:	mov	ah,009h		;Request 128K of XMS memory.
	mov	dx,128
	call	I_XMS
	jz s	I_XFre		;If no errors, "free" reserved XMS.
I_XErr:	mov	dx,(VEMsg-@)	;Set up "XMS init error" message.
	mov	VEMsg.lw,"MX"
I_ErrJ:	jmp	I_Err		;Go display XMS error message & exit!
I_XFre:	mov	@XMHdl,dx	;Save XMS buffer handle number.
	xchg	CLXDH,dx
	or	dx,dx		;Any XMS reserved by /R15 or /R63?
	jz s	I_XLok		;No, go "lock" our XMS memory.
	mov	ah,00Ah		;"Free" our reserved XMS memory.
	call	I_XMS
	jnz s	I_XErr		;If error, display message and exit!
I_XLok:	mov	ah,00Ch		;"Lock" our driver's XMS memory.
	mov	dx,CLXDH
	call	I_XMS
	jnz s	I_XErr          ;If error, display message and exit!
	shl	edx,16		;Get unaligned 32-bit buffer address.
	or	dx,bx
	mov	esi,edx		;Initialize command-list XMS offset.
	mov	eax,edx		;Copy 32-bit address to EAX-reg.
	jz s	I_XBAd		;Any low-order XMS buffer "offset"?
	mov	ax,0FFFFh	;Yes, align address to an even 64K.
	inc	eax
I_XBAd:	mov	@XBAddr,eax	;Save aligned "main buffer" address.
	mov	cx,ax		;Get buffer "offset" in XMS memory.
	sub	cx,dx
	mov	@XMOffs.lw,cx	;Set offset in "XMSMov" subroutine.
	mov	edx,000010000h	;Put command-list after XMS buffer.
	jcxz	I_PRDA		;Is buffer already on a 64K boundary?
	or	edx,-32		;No, put command-list before buffer.
I_PRDA:	add	eax,edx		;Set our 32-bit PRD address.
	mov	PRDAd,eax
	sub	eax,esi		;Set final command-list XMS offset.
	mov	CLXDA,eax
I_HDCt:	cmp	BiosHD,0	;Any BIOS hard-disks to use?
	je	I_SCD		;No, scan for CD/DVD units to use.
I_Next:	mov	ah,HDUnit	;Set unit number in error message.
	mov	cx,2
	mov	si,(UnitNo-@)
	call	I_HexA
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
	or	cl,cl		;Valid CHS values for this unit?
	jnz s	I_EDDX		;Yes, get disk's EDD "extensions".
	mov	dx,(CHSMsg-@)	;Display "CHS data error" message.
	call	I_Msg
I_EDDX:	mov	ah,041h		;Get EDD "extensions" for this disk.
	mov	bx,055AAh
	call	I_In13
	jc s	I_MorJ		;If none, ignore disk & test for more.
	cmp	bx,0AA55h	;Did BIOS "reverse" our entry code?
	jne s	I_MorJ		;No, ignore this disk & test for more.
	test	cl,004h		;Does this disk have "EDD" extensions?
	jz s	I_MorJ		;No, ignore this disk & test for more.
	mov	si,(EDDBuff-@)	;Point to "EDD" input buffer.
	mov	[si].lw,30	;Set 30-byte buffer size.
	or	[si+26].dwd,-1	;Init NO "DPTE" data!  A bad BIOS may
				;  NOT post this dword for USB sticks
				;  etc.!  Many Thanks to Daniel Nice!
	mov	ah,048h		;Get this disk's "EDD" parameters.
	call	I_In13
	jc s	I_ErED		;Error?  Display msg. & ignore!
	cmp	[si].lw,30	;Did we get at least 30 bytes?
	jb s	I_MorJ		;No, ignore this disk & test for more.
	cmp	[si+26].dwd,-1	;Valid "DPTE" pointer?
	je s	I_MorJ		;No, ignore this disk & test for more.
	les	si,[si+26]	;Get this disk's "DPTE" pointer.
	mov	bx,15		;Calculate "DPTE" checksum.
	xor	cx,cx
I_CkSm:	add	cl,es:[bx+si]
	dec	bx
	jns s	I_CkSm
	jcxz	I_EDOK		;If checksum O.K., use parameters.
I_ErED:	mov	dx,(EDDMsg-@)	;Display "EDD BIOS ERROR" message.
	call	I_Msg
	mov	dx,(UnitMsg-@)
	call	I_Msg
I_MorJ:	jmp	I_More		;Ignore this disk and test for more.
I_EDOK:	mov	bx,00010h	;Initialize IDE unit number index.
	and	bl,es:[si+4]
	shr	bl,4
	mov	ax,es:[si]	;Get disk's IDE base address.
	mov	CtlrNo.lb,'0'	;Reset display to "Ctlr. 0".
	mov	si,(Ctl1Tbl-@)	;Point to IDE address table.
I_ITbl:	cmp	[si].lw,-1	;Is this IDE table active?
	je s	I_More		;No, ignore this disk & test for more.
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
	jmp s	I_More		;Ignore this disk and test for more.
I_ChMS:	push	bx		;Save disk's controller/unit numbers.
	mov	IdeDA,ax	;Save disk's base IDE address.
	add	ax,CDSEL	;Point to IDE device-select register.
	xchg	ax,dx
	mov	al,bl		;Get drive-select command byte.
	shl	al,4
	or	al,LBABITS
	out	dx,al		;Select master or slave disk.
	push	ax		;Save disk's device-select values.
	push	bx
	mov	dx,(CtlMsg-@)	;Display IDE controller number.
	call	I_Msg
	pop	ax		;Reload disk's device-select value.
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
	pop	ax		;Reload disk controller/unit number.
	jc s	I_UDEr		;If any errors, display message.
	mov	bx,@LastU	;Set controller/unit in our tables.
	mov	[bx+CtlUn-@],al
	inc	@LastU		;Bump our unit-table index.
	jmp s	I_More		;Go test for any more hard disks.
I_UDEr:	call	I_Msg		;NOT UltraDMA -- Display error msg.
I_More:	inc	HDUnit		;Bump BIOS unit number.
	cmp	@LastU,MAXBIOS	;More entries in our units table?
	jae s	I_SCD		;No, check for any CD/DVD drives.
	dec	BiosHD		;More BIOS disks to check?
	jnz	I_Next		;Yes, loop back and do next one.
I_SCD:	xor	bx,bx		;Zero BX-reg. for CD/DVD logic.
	mov	DNMsg.lw," ,"	;Change start of disk "name" msg.
	mov	CtlrNo.lb,'0'-1	      ;Reset display controller no.
I_SCD1:	test	[bx+USNdx-@].lb,001h  ;Is this a new I-O channel?
	jnz s	I_SCD2		      ;No, test for new controller.
	add	[bx+DvdTB-@].lw,4     ;Bump to next I-O addresses.
I_SCD2:	cmp	[bx+USNdx-@].lb,004h  ;Is this a new controller?
	jb s	I_SCD3		      ;No, get unit's I-O addresses.
	inc	CtlrNo.lb	   ;Bump display controller number.
	mov	[bx+USNdx-@],bl	   ;Reset unit-select index.
I_SCD3:	mov	si,[bx+DvdTB-@]	   ;Load I-O parameter pointer.
	cmp	si,(DVDTEnd-@)	   ;Any more IDE units to check?
	jae s	I_AnyD		   ;No, check for any drives to use.
	mov	al,[bx+USNdx-@]	   ;Set indicated unit-select byte.
	shl	al,4
	or	al,MSEL
	mov	[bx+USelB-@],al
	inc	[bx+USNdx-@].lb	   ;Bump unit-select index.
	mov	eax,[si]	   ;Get this unit's I-O addresses.
	cmp	ax,-1		   ;Any more controllers to check?
	je s	I_AnyD		   ;No, check for any drives to use.
	mov	[bx+DMAAd-@],eax   ;Set this unit's I-O addresses.
	cmp	[bx+NoDMA-@],bl	   ;Is all UltraDMA disabled?
	jne s	I_SCD4		   ;Yes, default to no DMA address.
	or	ax,ax		   ;"Legacy IDE" with no DMA ctlr.?
	jnz s	I_SCD5		   ;No, go validate this unit.
I_SCD4:	or	[bx+DMAAd-@].lw,-1 ;Invalidate UltraDMA address.
I_SCD5:	call	I_VCD		   ;Validate unit as ATAPI CD/DVD.
	jc s	I_SCD1		   ;If invalid, merely ignore it.
	mov	si,[bx+UTblP-@]	   ;Update unit-table parameters.
	mov	eax,[bx+DMAAd-@]
	mov	[si],eax
	mov	al,[bx+USelB-@]
	mov	[si+6],al
	add	si,20		   ;Update unit-table pointer.
	mov	[bx+UTblP-@],si
	inc	[bx+CDUnits-@].lb  ;Bump number of active units.
	inc	CDMsgNo		   ;Bump display unit number.
	cmp	si,UTblLmt	;Can we install another drive?
	jb	I_SCD1		;Yes, loop back & check for more.
I_AnyD:	mov	dx,(NDMsg-@)	;Point to "Nothing to use" message.
	mov	al,CDUnits	;Get number of CD/DVD drives.
	or	al,al		;Do we have any CD/DVD drives to use?
	jnz s	I_20LE		;Yes, see if we will load in the HMA.
	or	al,@LastU.lb	   ;Any BIOS disk or diskette to use?
	mov	eax,[Suffix+2].dwd ;(Post driver's "No CD/DVD" name).
	mov	DvrNam.dwd,eax
	mov	[DvrNam+4].dwd," $RJ"
	jz	I_Err		;None?  Display error msg. and exit!
I_20LE:	shl	HFlag,1		;Will we be loading in the HMA?
	jz	I_MMv1		;No, check for memory movedowns.
	mov	ah,005h		;Issue "A20 local-enable" request.
	call	I_XMS
	mov	eax," 02A"	;Get "A20" error-message prefix.
	jnz s	I_HMAX		;If any "A20" error, bail out QUICK!!
	mov	ax,04A02h	;Request needed memory in the HMA.
	mov	bx,HMASize
	call	I_In2F
	mov	ax,es		;Get returned HMA segment address.
	cmp	ax,-1		;Is segment = 0FFFFh?
	jne s	I_HMA1		;No, VERY unusual but not an error.
	cmp	ax,di		;Is offset also = 0FFFFh?
	jne s	I_HMA1		;No, adjust addresses for HMA usage.
	call	I_A20D		;BAAAD News!  Do "A20 local-disable".
	mov	eax," AMH"	;Get "HMA" error-message prefix.
I_HMAX:	jmp	I_VErr		  ;Go display error message & exit!
I_HMA1:	lea	cx,[di-(HDReq-@)] ;Get starting HMA logic offset.
	add	@CDMain.lw,cx	  ;Adjust CD/DVD entry routine ptr.
	mov	@CDMain.hw,ax
	add	@HDMain.lw,cx	;Adjust hard-disk entry routine ptr.
	mov	@HDMain.hw,ax
	add	@CtlUn,cx	;Adjust disk "CHS tables" offsets.
	add	@CHSec,cx
	add	@CHSHd,cx
	add	@CtlTbl,cx	;Adjust disk controller-table ptr.
	add	CLXPtr,cx	;Adjust our XMS block pointers.
	add	XMSPtr,cx
	add	@UTable,cx	;Adjust CD/DVD table pointers.
	add	@DspTB2,cx
	add	@DspTB3,cx
	add	@DspTB4,cx
	add	@DspOfs,cx	;Adjust CD/DVD dispatch offset.
	shl	NDFlag,1	;Are we running CD/DVD drives only?
	jz s	I_HMA2		;No, move needed logic up to the HMA.
	push	es		;Save our starting HMA address.
	push	di
	mov	dx,(HDReq-@)	;Set move-down destination address.
	call	I_MvDn		;"Move down" all CD/DVD routines.
	pop	di		;Reload our starting HMA address.
	pop	es
I_HMA2:	mov	cx,HMASize	;Move all needed logic up to the HMA.
	shr	cx,1
	mov	si,(HDReq-@)
	rep	movsw
	call	I_A20D		;Issue "A20 local-disable" request.
	jmp s	I_Done		;Go set DOS "Init" packet results.
I_MMv1:	shl	NDFlag,1	;Are we running CD/DVD drives only?
	jz s	I_MMv2		;No, check for hard-disks only.
	mov	dx,(CDLM-@)	;Set move-down destination address.
	call	I_MvDn		;"Move down" all CD/DVD routines.
	jmp s	I_Done		;Go set DOS "Init" packet results.
I_MMv2:	shl	NCFlag,1	;Are we running hard-disks only?
	jz s	I_Done		;No, set DOS "Init" packet results.
	push	cs		;Point ES-reg. to this driver.
	pop	es
	mov	ax,(HDReq-Strat) ;Get disk move-down "offset".
	sub	@HDMain.lw,ax	 ;Adjust disk entry routine ptr.
	sub	CLXPtr,ax	 ;Adjust our XMS block pointers.
	sub	XMSPtr,ax
	sub	@CtlUn,ax	;Adjust disk "CHS tables" offsets.
	sub	@CHSec,ax
	sub	@CHSHd,ax
	sub	@CtlTbl,ax	;Adjust disk controller-table ptr.
	mov	cx,(HMALEN1/2)	;Move down main hard-disk routines.
	mov	si,(HDReq-@)
	mov	di,(Strat-@)
	rep	movsw
I_Done:	popad			;Done!  Reload all 32-bit CPU regs.
	les	bx,RqPkt	;Set results in DOS "init" packet.
	mov	ax,VDSLn.lw
	mov	es:[bx].RPLen,ax
	mov	es:[bx].RPStat,RPDON
	shr	NDFlag,1	;Has user requested no hard-disks?
	jnz s	I_End		;Yes, no stack to clear -- go exit.
	mov	@Stack,ax	;Set starting hard-disk stack addr.
	xchg	ax,bx		;Clear driver's stack (helps debug).
	mov	ax,STACK
I_ClrS:	dec	bx
	mov	[bx].lb,0
	dec	ax
	jnz s	I_ClrS
	cmp	ax,@LastU	;Do we have any hard-disks to use?
	je s	I_End		;No, reload all registers and exit.
	mov	ax,02513h	;"Hook" this driver into Int 13h.
	mov	dx,(Entry-@)
	call	I_In21
I_End:	pop	dx		;Reload 16-bit registers we used.
	pop	bx
	pop	ax
	pop	es		;Reload CPU segment registers.
	pop	ds
	popf			;Reload CPU flags and exit.
	retf
;
; Init subroutine to test for and set up each PCI controller.
;
I_PCIX:	ret			     ;(Local exit, used below).
I_PCIC:	mov	IdeDA,ax	     ;Save subclass & function codes.
	and	PCIDevX,0	     ;Reset PCI device index.
I_PCI1:	cmp	CtlrAdP,(CtlTEnd-@)  ;More space in address tables?
 	jae s	I_PCIX		     ;No, go exit above.
	mov	ax,IdeDA	;Test PCI class 1, subclass/function.
	mov	ecx,000010003h	;(Returns bus/device/function in BX).
	xchg	ax,cx
	mov	si,PCIDevX
	call	I_In1A
	jc s	I_PCIX		;Controller not found -- exit above.
	inc	PCIDevX		;Bump device index for another test.
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
I_PCI2:	mov	si,CtlrAdP	;Get current I-O address table ptr.
	mov	ax,DMAAd	;Set controller DMA base address.
	mov	[si],ax
	test	PCI_IF,001h	;Is this a "Native PCI" controller?
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
	mov	di,DvdTA	;Point to CD/DVD addresses table.
	push	cs
	pop	es
	lodsd			;Set CD/DVD primary addresses.
	stosd
	add	ax,8		;Set CD/DVD secondary addresses.
	stosw
	movsw
	mov	DvdTA,di	;Bump CD/DVD table pointer.
	mov	dx,(PCMsg-@)	;Display all controller data.
	call	I_Msg
	inc	[PCMsg+3].lb	;Bump controller number in message.
	add	CtlrAdP,CTLTSIZ	;Bump controller address-table ptr.
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
	mov	al,[bx+USelB-@]	;Select IDE "master" or "slave" unit.
	out	dx,al
	call	I_CkTO		;Await controller-ready.
	jc s	I_VC7		;If timeout, go exit below.
	mov	al,0A1h		;Issue "Identify Packet Device" cmd.
	out	dx,al
	call	I_CkTO		;Await controller-ready.
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
	mov	[bx+UFlag-@],al	;Save UltraDMA "valid" flags.
	mov	cl,35		;Skip I.D. words 54-87 (unimportant)
I_VC4:	in	ax,dx		;  and read I.D. word 88 into AX-reg.
	loop	I_VC4
	mov	[bx+UMode-@],ah	;Save posted UltraDMA "mode" value.
	mov	cl,167		;Skip all remaining I.D. data.
I_VC5:	in	ax,dx
	loop	I_VC5
	cmp	si,08500h	;Do device flags say "ATAPI CD/DVD"?
	je s	I_VC8		;Yes, see about UltraDMA use.
I_VC6:	stc			      ;Set carry flag on (error!).
I_VC7:	ret			      ;Exit.
I_VC8:	test	[bx+UFlag-@].lb,004h  ;Valid UltraDMA "mode" bits?
	jz s	I_VC9		      ;No, disable drive UltraDMA.
	cmp	[bx+UMode-@],bl	      ;Can drive do mode 0 minimum?
	jne s	I_VC10		      ;Yes, display "Unit n:" msg.
I_VC9:	or	[bx+DMAAd-@].lb,1     ;Disable this drive's UltraDMA.
I_VC10:	mov	dx,(CDMsg-@)	      ;Display "CDn:  " message.
	call	I_Msg
	mov	dx,(PriMsg-@)	      ;Point to "Primary" message.
	test	[bx+IdeDA-@].lb,080h  ;Primary-channel drive?
	jnz s	I_VC11		      ;Yes, display "Primary" msg.
	mov	dx,(SecMsg-@)	      ;Point to "Secondary" message.
I_VC11:	call	I_Msg		      ;Display CD/DVD's IDE channel.
	mov	dx,(MstMsg-@)	      ;Point to "Master" message.
	cmp	[bx+USelB-@].lb,SSEL  ;Is this drive a "slave"?
	jnz s	I_VC12		      ;No, display "Master".
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
I_VC15:	mov	cx,[bx+UMode-@]	;Initialize UltraDMA "mode" scan.
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
;
; Subroutine to test for CD/DVD I-O timeouts.   This is a "copy" of
;   the "ChkTO" subroutine above, located here so there are not any
;   BACKWARD calls into the run-time logic during driver init!
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
;
; Subroutine to disable the driver's use of XMS memory.
;
I_DisX:	db	066h,0B8h	   ;Disable hard-disk buffered I-O.
	jmp	$+(Pass-@NoXM1)
	db	0
	mov	@NoXM1.dwd,eax	
	mov	al,090h		   ;Disable "XMSMov" subroutine.
	mov	XMSReq.lb,al
	mov	@NoXM2.lb,al	   ;Disable CD/DVD buffered input.
	db	066h,0B8h	   ;("nop"/"cmc"/"adc [bx+DMF-@],bl"
	cmc			   ;  commands at "@NoXM2" -- DMA if
	adc	[bx+DMF-@],bl	   ;  buffer O.K., PIO mode if not).
	mov	[@NoXM2+1].dwd,eax
	ret			   ;Exit.
;
; Init subroutine to "move down" CD/DVD routines if /N2 is given. 
;
I_MvDn:	push	cs		;Point ES-reg. to this driver.
	pop	es
	mov	ax,(CDHMA-@)	;Get main CD/DVD move-down "offset".
	sub	ax,dx
	sub	@CDMain.lw,ax	;Adjust CD/DVD entry routine ptr.
	sub	CLXPtr,ax	;Adjust our XMS block pointers.
	sub	XMSPtr,ax
	sub	@UTable,ax	;Adjust CD/DVD table pointers.
	sub	@DspTB2,ax
	sub	@DspTB3,ax
	sub	@DspTB4,ax
	sub	@DspOfs,ax	 ;Adjust CD/DVD dispatch offset.
	mov	ax,(A20Dis-CDBM) ;Adjust CD/DVD "entry" addresses.
	sub	StratP,ax
	sub	DevIntP,ax
	sub	@InBuf1,ax	;Adjust CD/DVD input buffer addrs.
	sub	@InBuf2,ax
	mov	cx,(CDLSiz/2)	;Move down CD/DVD "entry" logic.
	mov	si,(A20Dis-@)
	mov	di,(CDBM-@)
	rep	movsw
	mov	cx,CDVDSiz	;Move down main CD/DVD routines.
	shr	cx,1
	mov	si,(CDHMA-@)
	mov	di,dx
	rep	movsw
	ret			;Exit.
;
; Subroutines to issue initialization "external" calls.
;
I_A20D:	mov	ah,006h		;"A20 local-disable" -- get XMS code.
I_XMS:	call	@XMgr1		;XMS -- issue desired request.
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
