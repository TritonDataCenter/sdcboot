	page	59,132
	title	XMGR -- DOS XMS Memory Manager.
;
; This is a DOS driver that works as a V3.0 XMS manager.   It replaces
; MS-DOS HIMEM.SYS and supports the V3.70+ UMBPCI upper-memory driver.
;
; General Program Equations.
;
	.386p			;Allow use of 80386 instructions.
s	equ	<short>		;Default conditional jumps to "short".
MAXBIOS	equ	32		;Maximum BIOS hard-disks supported.
XMSVER	equ	00300h		;Supported driver XMS version number.
DVRVER	equ	0030Ah		;"Current" driver XMS version number.
DEFHDLS	equ	48		;Default XMS "Handles" count.
HDWRFL	equ	00410h		;BIOS hardware-installed flag.
HDISKS	equ	00475h		;BIOS hard-disk count address.
VDSFLAG equ	0047Bh		;BIOS "Virtual DMA" flag address.
P92PORT	equ	092h		;"A20" I-O port for PS/2, etc.
P92FLAG	equ	002h		;"A20" I-O port test flag.
CR	equ	00Dh		;ASCII carriage-return.
LF	equ	00Ah		;ASCII line-feed.
;
; XMS Error Codes.
;
ERR_UNSUP	equ	080h
ERR_A20		equ	082h

ERR_HMAINUSE	equ	091h

ERR_NOMEMORY	equ	0A0h
ERR_NOHANDLE	equ	0A1h
ERR_BADHNDL	equ	0A2h
ERR_BADSH	equ	0A3h
ERR_BADSO	equ	0A4h
ERR_BADDH	equ	0A5h
ERR_BADDO	equ	0A6h
ERR_BADLEN	equ	0A7h
ERR_PARITY	equ	0A9h
ERR_LOCKED	equ	0ABh
ERR_LOCKOVFL	equ	0ACh
ERR_NOUMB	equ	0B1h
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
	dw	?		;(Unused by us).
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
	db	2 dup (?)	;(Unused by us).
RPOp	db	?		;Opcode.
RPStat	dw	?		;Status word.
	db	9 dup (?)	;(Unused by us).
RPLen	dw	?		;Resident driver 32-bit length.
RPSeg	dw	?		;(Segment must ALWAYS be set!).
RPCL	dd	?		;Command-line data pointer.
RP	ends
RPERR	equ	08003h		;Packet "error" flags.
RPDON	equ	00100h		;Packet "done" flag.
;
; XMS "Handles" Table Entry Layout.
;
HDL	struc
HFlag	db	?		;"Flag" byte.
HLock	db	?		;"Lock" count.
HBase	dd	?		;"Base address" in KBytes.
HSize	dd	?		;"Block length" in KBytes.
HDL	ends
HDLSIZE	equ	size HDL	;Size of a "Handles" Table entry.
;
; Extended Memory Move Block Layout.
;
MX	struc
MXLen	dd	?		;Length of block to move.
MXSrcH	dw	?		;Source handle number.
MXSrcA	dd	?		;Source seg:offs address
				;  or 32-bit block offset.
MXDstH	dw	?		;Destination handle number.
MXDstA	dd	?		;Destination seg:offs address
				;  or 32-bit block offset.
MX	ends
;
; Segment Definitions.
;
CODE	segment	public use16 'CODE'
	assume	cs:code,ds:code,es:code
;
; DOS Device-Driver "Header".
;
@	dd	0FFFFFFFFh	;Link to next device driver.
	dw	0A000h		;Device "attribute" flags.
StratP	dw	(Strat-@)	;"Strategy" routine pointer.
	dw	(DevInt-@)	;"Device Interrupt" routine pointer.
DvrNam	db	'XMGR2$',0,0	;Driver name.
;
; "Stub" Driver Variables, Part 1.
;
HMAUsed	db	0		;HMA being used flag (1 if so).
A20Flag	db	0		;"A20" changable flag (0 if not).
GEFlag	db	0		;"A20" global-enable flag.
;
; "Dummy" request-packet handler, for all but the "init" packet.
;
Dummy:	mov	es:[bx].RPStat,00100h  ;Just post "done" & exit.
DummyX:	retf
;
; "Stub" XMS Entry Routine.
;
	dd	0		;(Unused alignment "filler").
XMSEnt:	jmp s	XMSGo		;For "hooks".   The entry jump must
	nop			;  be short, to denote the end of a
	nop			;  "hook" chain.   The nop commands
	nop			;  let a far jump be patched in.
XMSGo:	jmp	cs:XMSRtnP	;Jump to main XMS processing routine.
LECount	dw	0		;"A20" local-enable count.
;
; "Stub" Int 2Fh Entry Routine.
;
I2FEnt:	pushf			;Save current CPU flags.
	stc		 	;Set carry flag for Int 2Fh.
	jmp s	IntGo		;Go to main interrupt entry routine.
;
; "Stub" Int 15h Entry Routine.
;
I15Ent:	pushf			;Save current CPU flags.
	clc			;Clear carry flag for Int 15h.
IntGo:	jmp	cs:IntRtnP	;Go to main interrupt entry routine.
;
; "Stub" Driver Variables, Part 2.
;
DvrSw	db	003h		;Driver /T switches and port flag.
XMSRtnP	dd	(XMSRtn-@)	;Current XMS-entry routine pointer.
IntRtnP	dd	(IntRtn-@)	;Current interrupt routine pointer.
StubEnd	equ	$		;End of low-memory driver "stub".
;
; Resident Driver Variables.
;
MSHT	db	001h		;040h:  "Handles Table" flag byte.
MSHTSiz	db	HDLSIZE		;041h:  "Handles Table" entry size. 
MSHTCt	dw	DEFHDLS		;042h:  "Handles Table" entry count.
MSHTPtr	dd	(HTbl-@)	;044h:  "Handles Table" pointer.
HKAddr	dd	0		;048h:  "Highest-known address".
;
; XMS Error-Code Table, to be used on BIOS Int 15h "move" errors.
;
I15Errs	db	0,ERR_PARITY,ERR_BADLEN,ERR_A20  ;BIOS move codes.
;
; Int 15h Descriptor Table, used for BIOS "protected-mode" moves.
;
MoveDT	dd	0,0		      ;"Null" descriptor.
	dd	0,0		      ;GDT descriptor.
SrcDsc	dd	00000FFFFh,000009300h ;Source segment descriptor.
DstDsc	dd	00000FFFFh,000009300h ;Dest.  segment descriptor.
	dd	0,0		      ;(Used by BIOS).
	dd	0,0		      ;(Used by BIOS).
;
; Global Descriptor Table, used during our own "real-mode" moves.
;
OurGDT	dd	0,0		      ;"Null" segment to start.
GDT_CS	dd	00000FFFFh,000009F00h ;Conforming code segment.
GDT_DS	dd	00000FFFFh,000CF9300h ;4-GB data segment in pages.
GDTLen	equ	($-OurGDT)
;
; Global Descriptor Table Pointer, used by our "lgdt" command.
;
GDTP	dw	GDTLen		;GDT length (always 24 bytes).
GDTPAdr	dd	0		;GDT 32-bit address (Set By Init).
;
; XMS Function Dispatch Table.
;
Dspatch	dw	(ScanM-@)	;Current "Scan Memory" entry point.
	dw	ReqHMA		;01h -- Request HMA.
	dw	RelHMA		;02h -- Release HMA.
	dw	A20_GE		;03h -- "A20" Global Enable.
	dw	A20_GD		;04h -- "A20" Global Disable.
	dw	A20_LE		;05h -- "A20"  Local Enable.
	dw	A20_LD		;06h -- "A20"  Local Disable.
	dw	ChkA20		;07h -- "A20" Test if Enabled.
	dw	QueryM		;08h -- XMS Memory Query.
	dw	AllocM		;09h -- XMS Memory Allocate.
	dw	FreeM		;0Ah -- XMS Memory Free.
	dw	MoveM		;0Bh -- XMS Memory Move.
	dw	LockM		;0Ch -- XMS Memory Lock.
	dw	UnlkM		;0Dh -- XMS Memory Unlock.
	dw	HInfo		;0Eh -- XMS "Handle" Info.
	dw	ReallM		;0Fh -- XMS Memory Reallocate.
	dw	ReqUMB		;10h -- Request Upper Memory Block.
DSLIMIT	equ	($-Dspatch-2)
;
; XMS Processing Routine.   Dispatches to the appropriate logic below
;   based on the function code in the AH-reg.    NOTE that interrupts
;   are disabled and the AX-reg. is zero after leaving this routine.
;
XMSRtn:	or	ah,ah		;Is request for Version-Number data?
	jz s	GetVer		;Yes, execute it unconditionally.
	push	ebp		;Save all needed CPU registers.
	push	eax
	push	ecx
	push	esi
	push	edi
	push	ds		;Save DS- and ES-registers.
	push	es
	pushf			;Save flags LAST so they reload FIRST!
	push	ds		;Save DS-register in ES-register.
	pop	es
	push	cs		;Set our driver's DS-register.
	pop	ds
	mov	ch,ah		;Copy request code to CH-reg.
	sar	ch,1		;Shift out low-order bit.
	cmp	ch,0C7h		;V3.0 "Extended Info" or "Reallocate"?
	je s	XCMask		;Yes, convert to V2.0 function code.
	cmp	ch,0C4h		;V3.0 "Query" or "Allocate" request?
	jne s	XCSetO		;No, preset dispatch-table offset.
XCMask:	and	ah,07Fh		;Convert V3.0 to V2.0 request code.
XCSetO:	movzx	di,ah		;Preset our dispatch-table offset.
	shl	di,1
	xor	eax,eax		;Preset "failure" status in EAX-reg.
	cmp	di,DSLIMIT	;Valid XMS function code?
@ReqLmt	equ	[$-1].lb
	jae s	XCBadF		;No?  Return "unsupported" and exit.
	jmp	Dspatch
XCClrT:	xchg	ax,bp		;"ScanM" end -- reset "Handles" table.
	stosw
	xchg	ax,bp
	stosd
	stosd
	loop	XCClrT
	pop	es		;Reload CPU regs. used during clear.
	pop	di
	pop	cx
XCCall:	call	Dspatch[di]	;Dispatch to indicated XMS routine.
XCExit:	popf			;Reload flags to permit interrupts.
	pop	es		;Reload ES- and DS-registers.
	pop	ds
	pop	edi		;Reload all 32-bit CPU registers.
	pop	esi		;  Note that only the UPPER-half
	pop	ecx		;  of the EAX-reg. gets reloaded!
	xchg	ax,bp
	pop	eax
	xchg	ax,bp
	pop	ebp
	retf			;Exit.
XCBadF:	mov	bl,ERR_UNSUP	;Unsupported function!  Post error.
	jmp s	XCExit		;Exit above and return "failure".
;
; XMS Function 00h:  Get Version Info.
;
GetVer:	mov	ax,XMSVER	;Return XMS version in AX-reg.
	mov	bx,DVRVER	;Return our version in BX-reg.
	mov	dx,00001h	;We ALWAYS have an HMA to use!
	retf			;Exit.
;
; XMS Function 01h:  Request the HMA.
;
ReqHMA:	cmp	al,HMAUsed	;Is the HMA currently free?
	jne s	ReqHMU		;No, post "HMA in use" and exit.
	inc	ax		;Post "success" status.
	mov	HMAUsed,al	;Flag the HMA as being in-use.
	ret			;Exit.
ReqHMU:	mov	bl,ERR_HMAINUSE	;Post "HMA in use" error code.
	ret			;Return "failure" status & exit.
;
; XMS Function 02h:  Release the HMA.
;
RelHMA:	mov	HMAUsed,al	;Ignore errors & flag HMA as free.
RelHMX:	inc	ax		;Post "success" status and exit.
	ret
;
; XMS Function 03h:  "A20" Global Enable.
;
A20_GE:	cmp	al,GEFlag	;Is "A20" already globally enabled?
	jne s	RelHMX		;Yes, return "success" and exit.
	call	A20_LE		;Issue "A20" Local Enable.
	jc s	A20_GX		;If error, return failure and exit.
	mov	GEFlag,al	;Post "A20" as globally enabled.
	ret			;Return "success" and exit.
;
; XMS Function 04h:  "A20" Global Disable.
;
A20_GD:	cmp	al,GEFlag	;Is "A20" already globally disabled?
	je s	RelHMX		;Yes, return "success" and exit.
	call	A20_LD		;Issue "A20" Local Disable.
	jc s	A20_GX		;If error, return failure and exit.
	shr	GEFlag,1	;Post "A20" as globally disabled.
A20_GX:	ret			;Exit.
;
; XMS Function 05h:  "A20" Local Enable.
;
A20_LE:	cli			;Disable CPU interrupts.
	xor	ax,ax		;Set "failure" status in AX-reg.
	cmp	al,A20Flag	;Can "A20" line be changed?
	je s	A20_OK		;No, return "success" and exit.
	cmp	LECount,-1	;Is "A20" local-enable count max.?
	je s	A20_ER		;Yes?  Return failure and exit.
	inc	ax		;Get "A20" enable code in AX-reg.
	cmp	ax,LECount	;Is "A20" local-enable count zero?
	jbe s	A20_In		;No, increment it and return success.
	call	A20Hdl		;Ask "A20" handler for an enable.
	or	ax,ax		;Was "A20" line enabled O.K.?
	jz s	A20_ER		;No?  Return failure and exit.
A20_In:	inc	LECount		;Increment "A20" local-enable ctr.
	jmp s	A20_OK		;Go post "success" and exit.
;
; XMS Function 06h:  "A20" Local Disable.
;
A20_LD:	cli			;Disable CPU interrupts.
	xor	ax,ax		;Set "failure" status in AX-reg.
	cmp	al,A20Flag	;Can "A20" line be changed?
	je s	A20_OK		;No, return "success" and exit.
	cmp	LECount,1	;What is our local-enable counter?
	jb s	A20_OK		;Zero, return "success" and exit.
	ja s	A20_Dc		;Over one, just decrement it.
	call	A20Hdl		;Ask "A20" handler for a disable.
	or	ax,ax		;Was "A20" line disabled O.K.?
	jz s	A20_ER		;No?  Return failure and exit.
	dec	ax		;Clear AX-reg. for "fall-through".
A20_Dc:	dec	LECount		;Decrement "A20" local-enable ctr.
A20_OK:	or	al,001h		;Post "success" and clear carry.
	ret			;Exit.
A20_ER:	stc			;Error!  Set carry for global calls.
	mov	bl,ERR_A20	;Return "A20 error" code and exit.
	ret
;
; XMS Function 07h:  Check "A20" Status.
;
ChkA20:	call	TstA20		;"Let George do it!", below.
	xor	bl,bl		;Reset returned error code & exit.
	ret
;
; XMS Function 08h and 88h:  Query Extended Memory.
;
QueryM:	xor	esi,esi		;Reset free-memory bucket.
	lds	di,MSHTPtr	;Set up "Handles" table scan.
	mov	bl,DEFHDLS
@HdlCt1	equ	[$-1].lb	;("Handles" count, set by Init).
QMNext:	cmp	[di].HFlag,1	;Is this a block of free memory?
	jne s	QMMore		;No, skip to next handle.
	add	esi,[di].HSize	;Update free-memory bucket.
	cmp	eax,[di].HSize	;Is this the largest block yet?
	jae s	QMMore		;No, skip to next handle.
	mov	eax,[di].HSize	;Update maximum-size bucket.
QMMore:	add	di,HDLSIZE	;Increment "Handles" table pointer.
	dec	bl		;More table "Handles" to check?
	jnz s	QMNext		;Yes, loop back and check next one.
	or	esi,esi		;Did we find any free memory?
	jnz s	QMAnyM		;Yes, check for "Query Any" request.
	mov	bl,ERR_NOMEMORY	;Return "no memory" error code.
QMAnyM:	push	esi		;Save total free memory count.
	or	ch,ch		;V3.0 "Query Any" request?
	js s	QMHKAd		;Yes, set "Highest-known address".
	push	eax		;Get upper maximum-block size.
	pop	cx
	pop	cx		;Over 64-MB maximum block size?
	jcxz	QMSetF		;No, set total free memory size.
	mov	ax,0FFFFh	;Set 64-MB maximum in AX-register.
QMSetF:	pop	dx		;Set total free memory in DX-reg.
	pop	cx		;Over 64-MB total free memory?
	jcxz	QMExit		;No, go exit.
	mov	dx,0FFFFh	;Set 64-MB maximum in DX-register.
QMExit:	ret			;Exit.
QMHKAd:	mov	ecx,cs:HKAddr	;Set our "highest-known address".
	pop	edx		;Set total free memory in EDX-reg.
	mov	bp,sp		;Point to our current stack.
	mov	[bp+20],eax	;Save EAX-reg. on stack for exit.
	jmp	SavECX		;Go save ECX-reg. also, and exit.
;
; XMS Functions 09h and 89h:  Allocate Extended Memory.
;
AllocM:	push	edx		;Save EDX-register.
	or	ch,ch		;V3.0 "Allocate Any" request?
	js s	Alloc1		;Yes, convert size to bytes.
	movzx	edx,dx		;Reset high-order allocaton.
Alloc1:	test	edx,0FFC00000h	;Is this request for over 4-GB?
	jnz s	AMLenE		;Yes?  Return "Bad Length" & exit.
	mov	si,ax		;Reset free-memory "Handle" ptr.
	mov	bp,ax		;Reset unused "Handle" pointer.
	lds	di,MSHTPtr	;Set up "Handles" table scan.
	mov	cx,DEFHDLS
@HdlCt2	equ	[$-2].lw	;("Handles" count, set by Init).
AMNext:	cmp	ax,si		;Need a free-memory "Handle"?
	jne s	AMUnus		;No, see about an unused one.
	cmp	[di].HFlag,1	;Is this a block of free memory?
	jne s	AMUnus		;No, see if it is unused.
	cmp	edx,[di].HSize	;Enough memory for user request?
	ja s	AMMore		;No, ignore it and skip to next.
	mov	si,di		;Save our free-memory "Handle".
	cmp	ax,bp		;Got an unused "Handle" too?
	je s	AMMore		;No, keep looking through table.
	jmp s	AMBoth		;Go do normal memory allocation.
AMUnus:	cmp	ax,bp		;Do we need an unused "Handle"?
	jne s	AMMore		;No, skip to next one.
	cmp	[di].HFlag,4	;Is this "Handle" totally unused?
	jne s	AMMore		;No, skip to next one.
	mov	bp,di		;Save our unused "Handle".
	cmp	ax,si		;Got a free-memory "Handle" too?
	jne s	AMBoth		;Yes, do normal memory allocation.
AMMore:	add	di,HDLSIZE	;Skip to next "Handle".
	loop	AMNext		;If more "Handles" left, loop back.
	cmp	ax,si		;Table end -- free memory found?
	jne s	AMAloc		;Yes, give user the whole block.
	cmp	ax,bp		;No memory OR unused "Handles"?
	je s	AMHdlE		;Yes, return no-handles and exit.
	cmp	eax,edx		;Does user want a "null" handle?
	jne s	AMNone		;No, return no-memory and exit.
AMNull:	mov	si,bp		;Set unused "Handle" in free bkt.
	jmp s	AMAloc		;Go set in-use & return "Handle" no.
AMBoth:	mov	ecx,edx		;Got free memory & unused "Handle" --
	jecxz	AMNull		;If only a null wanted, go set it up.
	mov	di,bp		;Get unused "Handle" in DI-reg.
	add	ecx,[si].HBase	;Calculate unused "Handle" base.
	mov	ebp,edx		;Swap free- & user-request sizes and
	xchg    [si].HSize,ebp	;  set request size in free "Handle".
	sub	ebp,edx		;Any "leftover" free memory?
	jz s	AMAloc		;No, flag old free "Handle" in-use.
	mov	[di].HFlag,1	;Flag unused "Handle" free memory.
	mov	[di].HBase,ecx	;Set new free "Handle" base.
	mov	[di].HSize,ebp	;Set new free "Handle" size.
AMAloc:	mov	[si].HFlag,2	;Flag old free "Handle" in-use.
	pop	edx		;Reload EDX-register.
	mov	dx,si		;Return "Handle" number in DX-reg.
AMDone:	inc	ax		;Return "success" and exit.
	ret
AMLenE:	mov	bl,ERR_BADLEN	;Bad request length -- set code.
	jmp s	AMErr		;Go reload EDX-reg. and exit.
AMHdlE:	mov	bl,ERR_NOHANDLE	;Get "no handles" error code.
	jmp s	AMErr		;Go reload EDX-reg. and exit.
AMNone:	mov	bl,ERR_NOMEMORY	;No available memory -- set code.
AMErr:	pop	edx		;Reload EDX-register.
	xor	dx,dx		;Zero user "Handle" number.
	ret			;Return "failure" and exit.
;
; XMS Function 0Ah:  Free Extended Memory.
;
FreeM:	call	ValHdl		;Validate user "Handle" number.
	cmp	al,[si].HLock	;Is "Handle" currently "locked"?
	jne s	FMLokE		;Yes?  Must NOT free its memory!
FreeRA:	xor	eax,eax		;Zero EAX-reg. for "Reallocate" calls.
	mov	[si].HFlag.lw,4	;Flag this "Handle" totally unused.
	cmp	eax,[si].HBase	;Is this a "Null Handle"?
	je s	AMDone		;Yes, return "success" and exit.
	mov	[si].HFlag,1	;Change "Handle" from used to free.
FMScan:	mov	di,offset (HTbl-HDLSIZE)  ;Set up "Handles" scan.
@HTbl1	equ	[$-2].lw
	cli			;Disable CPU interrupts for each scan!
	cmp	[si].HFlag,1	;Still free-memory after an interrupt?
	jne s	AMDone		;No, exit NOW to prevent NASTY errors!
	mov	ebp,[si].HBase	;Get freed "Handle" end address.
	add	ebp,[si].HSize
FMNext:	add	di,HDLSIZE	;Skip to next "Handle" in table.
	cmp	di,08000h	;End of table with no more merges?
@HTEnd1	equ	[$-2].lw	;(End-of-table address, Init set).
	jae s	AMDone		;Yes, Free-Memory request is done!
	cmp	[di].HFlag,1	;Is this a block of free memory?
	jne s	FMNext		;No, skip to next "Handle".
	mov	ecx,[di].HBase	;Get table "Handle" base address.
	cmp	ebp,ecx		;Table "Handle" just above freed one?
	je s	FMAbov		;Yes, merge memory in freed "Handle".
	add	ecx,[di].HSize	;Get table "Handle" end address.
	cmp	ecx,[si].HBase	;Table "Handle" just below freed one?
	jne s	FMNext		;No, skip to next "Handle" in table.
	xchg    si,di		;Merge memory into table "Handle".
FMAbov:	mov	[di].HFlag,4	;Flag upper "Handle" totally unused.
	xor	ecx,ecx		;Reset upper "Handle" base to zeros.
	mov	[di].HBase,ecx
	xchg	ecx,[di].HSize	;Load and reset upper "Handle" size.
	sti			;Permit interrupts before next scan.
	add	[si].HSize,ecx	;Merge all memory into low "Handle".
	jmp s	FMScan		;Go scan again to merge more memory.
FMLokE:	mov	bl,ERR_LOCKED	;Locked "Handle"!  Get error code.
	ret			;Return "failure" and exit.
;
; XMS Function 0Bh:  Move Extended Memory (really moves ANY memory).
;
MoveM:	mov	ecx,es:[si].MXLen ;Get move byte count.
	jecxz	MMDone		  ;If zero, just exit immediately.
	push	bx		;Save "critical" BX-reg.
	mov	al,ERR_BADLEN	;Set "bad length" error code.
	test	cl,1		;"Odd" byte count?
	jnz s	MMovNS		;Yes, exit immediately.
	lea	bx,[si].MXSrcH	;Get 32-bit move source address.
	call	GetLin
	jc s	MMovSE		;If O.K., get desination address.
	push	bx		;Save handle for unlock at exit.
	xchg	esi,edi		;Save source address in ESI-reg.
	lea	bx,[di].MXDstH	;Get 32-bit move destination address.
	call	GetLin
	jc s	MMovND		;If error, unlock source hdl. & exit.
	push	bx		;Save handle for unlock at exit.
	call	MvData		;Move data from source to destination.
	xchg	ax,bx		;"Swap" any error code into AX-reg.
	pop	si		;Unlock destination handle, if used.
	or	si,si
	jz s	MMovND
	dec	[si].HLock
MMovND:	pop	si		;Unlock source handle, if used.
	or	si,si
	jz s	MMovNS
	dec	[si].HLock
MMovNS:	pop	bx		;Reload "critical" BX-reg.
	and	ax,000FFh	;Any error code to return?
	jnz s	MMErr		;Yes, set code in BL-reg. & exit.
MMDone:	inc	ax		;Post "success" status in AX-reg.
	ret			;Exit.
MMErr:	mov	bl,al		;Return error code in BL-reg.
	xor	ax,ax		;Post "failure" status and exit.
	ret
MMovSE:	cmp	al,ERR_BADLEN	;Source error!  Bad length or locks?
	jae s	MMovNS		;Yes, return error code and exit.
	sub	al,2		;Convert error code to source value.
	jmp s	MMovNS		;Go return error code and exit.
;
; XMS Function 0Ch:  Lock Extended Memory.
;
LockM:	call	ValHdl		;Entry -- validate user "Handle".
	inc	[si].HLock	;Is "lock" count at maximum?
	jz s	LKOvfl		;Yes?  Return "failure" and exit.
	mov	ebp,[si].HBase	;Set 32-bit memory address in DX:BX.
	shl	ebp,10
	push	ebp
	pop	bx
	pop	dx
	inc	ax		;Return "success" status and exit.
	ret
LKOvfl:	dec	[si].HLock	;Lock overflow!  Decrement counter.
	mov	bl,ERR_LOCKOVFL	;Post "lock overflow" error code.
	mov	dx,ax		;Reset handle and return failure.
	ret
;
; XMS Function 0Dh:  Unlock Extended Memory.
;
UnlkM:	call	ValHdl		;Validate user "Handle" number.
	inc	si		;Point to "Handle" lock counter.
	cmp	al,[si]		;Is handle "lock" count now zero?
	sbb	[si],al		;No, decrement handle "lock" count.
	inc	ax		;Return "success" status and exit.
	ret
;
; XMS Functions 0Eh and 8Eh:  Get "Handle" Information.
;
HInfo:	call	ValHdl		;Validate user "Handle" number.
	xor	cl,cl		;Reset unused "Handle" counter.
	mov	di,offset HTbl	;Set up "Handles" table scan.
@HTbl2	equ	[$-2].lw
	mov	bp,DEFHDLS
@HdlCt3	equ	[$-2].lw	;("Handles" count, set by Init).
HINext:	cmp	[di].HFlag,4	;Is this "Handle" totally unused?
	jne s	HIMore		;No, skip to next one.
	inc	cx		;Increment unused "Handle" counter.
HIMore:	add	di,HDLSIZE	;Increment "Handles" table pointer.
	dec	bp		;More table "Handles" to check?
	jnz s	HINext		;Yes, loop back and do next one.
	mov	bh,[si].HLock	;Get "Handle" lock count in BH-reg.
	mov	esi,[si].HSize	;Get "Handle" block size in ESI-reg.
	inc	ax		;Post "success" status.
	shr	ch,8		;V3.0 "Extended Info" request?
	jc s	HIExtI		;Yes, return 32-bit block size.
	mov	dx,si		;Return 16-bit block size in DX-reg.
	mov	bl,cl		;Set unused "Handles" in BL-reg.
	shr	esi,16		;Is "Handle" block size over 64-MB?
	jz s	HIBye		;No, go exit below.
	mov	dx,0FFFFh	;Set 64-MB maximum in DX-reg.
HIBye:	ret			;Exit.
HIExtI:	mov	edx,esi		;Return 32-bit block size in EDX-reg.
	mov	bp,sp		;Point to our current stack.
SavECX:	mov	[bp+16],ecx	;Save ECX-reg. on stack for exit.
	ret			;Exit.
;
; XMS Functions 0Fh and 8Fh:  Reallocate Extended Memory.
;
ReallM:	call	ValHdl		;Validate user "Handle" number.
	cmp	al,[si].HLock	;Is desired "Handle" locked?
	jne	FMLokE		;Yes?  Return "failure" and exit.
	push	ebx		;Save original request size.
	or	ch,ch		;V3.0 "Reallocate Any" request?
	js s	RAConv		;Yes, convert size to bytes.
	movzx	ebx,bx		;Reset high-order allocaton.
RAConv:	mov	cl,ERR_BADLEN	;For Fools, get Invalid-Length code.
	test	ebx,0FFC00000h	;Is this request for more than 4-GB?
	jnz s	RANewE		;Yes?  Return "Failure" to our Fool!
	mov	ecx,[si].HSize	;Get current "Handle" block size.
	sub	ecx,ebx		;What kind of reallocation is this?
	ja s	RALess		;Smaller -- get an unused "Handle".
	jb s	RANew		;Larger -- get a new area of memory.
RABail:	pop	ebx		;Same size -- reload EBX-register.
	inc	ax		;Just return "success" and exit.
	ret
RALess:	mov	di,offset (HTbl-HDLSIZE)  ;Set up "Handles" scan.
@HTbl3	equ	[$-2].lw
RALNxt:	add	di,HDLSIZE	;Skip to next "Handle" in table.
	cmp	di,08000h	;End of table with no more merges?
@HTEnd2	equ	[$-2].lw	;(End-of-table address, Init set).
	jae s	RABail		;Yes, keep current allocation as-is.
	cmp	[di].HFlag,4	;Is this "Handle" totally unused?
	jne s	RALNxt		;No, skip to next "Handle".
	mov	[di].HSize,ecx	;Set free-memory "Handle" size.
	sub	[si].HSize,ecx	;Cut our current "Handle" size.
	mov	ecx,[si].HBase	;Set free-memory "Handle" base.
	add	ecx,ebx
	mov	[di].HBase,ecx
	xchg	si,di		;Swap current and free "Handles".
	jmp s	RAFree		;Go merge free memory & other blocks.
RANew:	push	edx		;Save our current "Handle" number.
	mov	edx,ebx		;Set new "Handle" size in EDX-reg.
	mov	ch,080h
	call	AllocM		;Allocate a new area of XMS memory.
	mov	si,dx		;Save new "Handle" in SI-register.
	pop	edx		;Reload old "Handle" in EDX-reg.
	xor	cl,cl		;Swap any error code into CL-reg.
	xchg	bl,cl
	cmp	al,1		;Any errors allocating new memory?
RANewE:	jne s	RAErr		;Yes?  Return "failure" and exit.
	inc	[si].HLock	;"Lock" our new memory area.
	mov	edi,[si].HBase	;Get move destination address.
	shl	edi,10		;Convert destination addr. to bytes.
	xchg	si,dx		;Swap current/new "Handle" numbers.
	push	si		;Save our current "Handle" number.
	mov	ecx,[si].HSize	;Get our current "Handle" block size.
	cmp	ecx,ebx		;Reallocating to a larger size?
	jbe s	RASrcA		;Yes, move all data to new "Handle".
	mov	ecx,ebx		;Move residual data to new "Handle".
RASrcA:	shl	ecx,10		;Convert move length to bytes.
	inc	[si].HLock	;"Lock" our current memory area.
	mov	esi,[si].HBase	;Get move source address,
	shl	esi,10		;Convert source address to bytes.
	call	MvData		;Move previous data to new "Handle".
	pop	di		;Reload our current "Handle" number.
	or	bl,bl		;Any errors during data move?
	jz s	RASwap		;No, go swap "Handle" data.
	push	bx		;BAAAD News!  Save error code.
	push	di		;Save our current "Handle" number.
	mov	si,dx		;Get RID of our "new" memory area.
	call	FreeRA		;(We are "stuck" in the OLD one!).
	pop	dx		;Reload our "Handle" and error code.
	pop	cx
RAErr:	pop	ebx		;Reload original EBX-register.
	mov	bl,cl		;Set returned error code in BL-reg.
	xor	ax,ax		;Return "failure" status and exit.
	ret
RASwap:	mov	si,dx		;Set free-memory "Handle" in SI-reg.
	mov	eax,[si].HBase	;Swap "Handle" bases.
	xchg	eax,[di].HBase
	mov	[si].HBase,eax
	mov	eax,[si].HSize	;Swap "Handle" sizes.
	xchg	eax,[di].HSize
	mov	[si].HSize,eax
	mov	[di].HFlag.lw,2	;Flag current "Handle" used/unlocked. 
RAFree:	push	di		;Save our current "Handle" number.
	call	FreeRA		;Merge free-memory with other blocks.
	pop	dx		;Reload our current "Handle" number.
	pop	ebx		;Reload original EBX-register.
	ret			;Return "success" status and exit.
;
; Subroutine to move data.   At entry, the ECX-reg. has the 32-bit
;   byte count, the ESI-reg. has the 32-bit source address and the
;   EDI-reg. has the 32-bit destination address.    At exit, carry
;   is set for an error, and the AL-reg. has an error code.   Note
;   that for speed, most registers are NOT saved!
;
MvData:	sti			;Allow CPU interrupts during moves.
	push	cs		;Point DS-register to this driver.
	pop	ds
	jecxz	MvDone		;If no data to move, just exit.
	smsw	ax		;Get CPU "machine status word".
	shr	eax,1		;Are we running in protected mode?
	jc s	MvProt		;Yes, use protected-mode logic below.
	call	A20_LE		;Real mode -- do "A20 Local Enable".
	or	ax,ax		;Was "A20 Local Enable" successful?
	jz s	MvExit		;No??  Post return code and get out!
	push	dx		;Save DX-reg. for "selector" use.
	mov	ebp,ecx		;Set move byte count in EBP-reg.
	mov	ecx,2048	;Set initial move section count.
MVRNxt:	cmp	ecx,ebp		;At least 2048 data bytes left?
	jbe s	MvRGo		;Yes, disable interrupts.
	mov	ecx,ebp		;Use remaining byte count.
MvRGo:	cli			;Disable CPU interrupts.
	db	02Eh,00Fh	;"lgdt cs:GDTP", coded the hard-way
	db	001h,016h	;   to avoid any annoying V5.1 MASM
	dw	(GDTP-@)	;   warning messages about it!
	mov	eax,cr0		;Set CPU protected-mode control bit.
	or	al,001h
	mov	cr0,eax
	mov	dx,(GDT_DS-OurGDT)  ;Set DS- & ES-reg. "selectors".
	mov	ds,dx
	mov	es,dx
	cld			;Ensure FORWARD "string" commands.
	shr	cx,2		;Get count of 32-bit words to move.
	db	0F3h,067h	;Move all 32-bit words using "rep"
	movsd			;  and "address-override" prefixes.
	db	67h,090h	;("Override" & "nop" for 386 BUG!).
	mov	ch,(2048/256)	;Set next move section count.
	jnc s	MvROK		;"Odd" 16-bit word to move?
	db	067h		;Yes, move it also, using "address-
	movsw			;  override" prefix.
	db	67h,090h	;("Override" & "nop" for 386 BUG!).
MvROK:	dec	ax		;Clear protected-mode control bit.
	mov	cr0,eax
	sti			;Allow interrupts after next command.
	sub	ebp,ecx		;Any more data sections to move?
	ja s	MvRNxt		;Yes, go move next data section.
	pop	dx		;Reload DX- and DS-regs.
	push	cs
	pop	ds
	call	A20_LD		;Issue "A20" Local Disable.
	jc s	MvExit		;If error, post return code and exit.
MvDone:	xor	bx,bx		;Success!  Reset returned error code.
MvExit:	mov	ds,MSHTPtr.hw	;Point DS-reg. to our "Handles" table.
	cli			;Disable CPU interrupts and exit.
	ret
MvProt:	mov	eax,65536	;Protected mode -- get section count.
@MovSC	equ	[$-4].dwd	;(Move section count, 2K if using /Z).
MvPNxt:	push	ecx		;Save remaining move byte count.
	push	esi		;Save current move source address.
	cmp	ecx,eax		;At least one full data section left.
	jb s	MvPGo		;No, use remaining byte count.
	mov	ecx,eax		;Use full-size move section count.
MvPGo:	cld			;Ensure FORWARD "string" commands!
	push	edi		;Save current move destination addr.
	shr	ecx,1		;Convert byte count to word count.
	push	cs		;Point ES-register to this driver.
	pop	es
	push	edi		;Set up destination descriptor.
	mov	di,(DstDsc+2-@)
	pop	ax
	cli			;(Must disable CPU interrupts now).
	stosw
	pop	ax
	stosb
	mov	[di+2],ah
	push	esi		      ;Set up source descriptor ("sub"
	sub	di,(DstDsc-SrcDsc+3)  ;  will zero carry for Int 15h).
	pop	ax
	stosw
	pop	ax
	stosb
	mov	[di+2],ah
	mov	si,(MoveDT-@)	;Point to our move descriptor table.
	mov	ah,087h		;Have JEMM386/BIOS move next section.
	int	015h
	sti			;Re-enable CPU interrupts.
	push	cs		;Reload our DS-register.
	pop	ds
	movzx	si,al		;If move error, get XMS error code.
	mov	bl,[si+I15Errs-@]
	pop	edi		;Reload destination/source addresses.
	pop	esi
	pop	ecx		;Reload remaining move byte count.
	jc s	MvExit		;If error, exit immediately.
	mov	eax,@MovSC	;Reload move section count.
	add	esi,eax		;Update source and dest. addresses.
	add	edi,eax
	sub	ecx,eax		;Any more data sections to move?
	ja s	MvPNxt		;Yes, move next section of data.
	jmp s	MvDone		;Go reset error code and exit.
	db	0,0		;(Unused alignment "filler").
;
; Subroutine to convert a Move-Extended offset or seg:offs into a
;   "linear" 32-bit address.   The handle is "locked", if needed.
;   The "linear" address is returned in the EDI-register.
;
GetLin:	mov	edi,es:[bx+2]	;Get desired user offset.
	mov	bx,es:[bx]	;Get desired user "Handle".
	dec	bx		;Is this an absolute address?
	jz s	GLExit		;Yes, just exit below.
	inc	bx		;Is this a segment:offset address?
	jz s	GLSegO		;Yes, go validate it below.
	mov	ax,bx		;"Handle" -- check it for validity.
	call	ChkHdl
	mov	al,ERR_BADDH	;Get "Bad Handle" error code.
	jc s	GLErr		;Invalid?  Set error code and exit!
	cmp	[bx].HFlag,2	;Is this "Handle" currently in-use?
	jne s	GLErr		;No, return "Bad Handle" and exit.
	mov	ebp,[bx].HSize	;Get "Handle" block size.
	shl	ebp,10		;Is this a "Null Handle"?
	jz s	GLErr		;Yes, return "Bad Handle" and exit.
	mov	al,ERR_BADDO	;Get "Bad Offset" error code.
	sub	ebp,edi		;Is user offset too big?
	jb s	GLErr		;Yes, return "Bad Offset" and exit.
	cmp	ecx,ebp		;Move length > ("Handle" + offset)?
	ja s	GLBadL		;Yes, return "bad length" and exit.
	mov	al,ERR_LOCKOVFL	;Get "lock overflow" error code.
	cmp	[bx].HLock,0FFh	;Is "lock" count at maximum?
	je s	GLErr		;Yes, return "lock overflow" & exit.
	inc	[bx].HLock	;Increment "lock" count.
	mov	eax,[bx].HBase	;Get starting "linear" address.
	shl	eax,10
	add	edi,eax		;Add start address to user offset.
GLExit:	clc			;Clear carry flag and exit.
	ret
GLSegO:	movzx	eax,di		;Get 32-bit offset in EAX-reg.
	xor	di,di		;Get 32-bit segment in EDI-reg.
	shr	edi,12
	add	edi,eax		;Get "linear" address in EDI-reg.
	mov	eax,edi		;Get ending "move" address.
	add	eax,ecx
	cmp	eax,00010FFF0h	;Is ending address beyond the HMA?
	jbe s	GLExit		;No, clear carry flag and exit.
GLBadL:	mov	al,ERR_BADLEN	;Return "invalid length" code.
GLErr:	stc			;Set carry flag (error!) and exit.
	ret
;
; Subroutine to validate "Handle" numbers in the DX-register.   An
;   invalid "Handle" causes an immediate return to "XCExit" in the
;   dispatch routine.   The SI-register is lost.
;
ValHdl:	mov	ax,dx		;Check "Handle" for proper limits.
	call	ChkHdl
	jc s	ValHE		;Invalid?  Post "Bad Handle" & exit.
	mov	si,dx		;Move "Handle" number into SI-reg.
	cmp	[si].HFlag,2	;Is this "Handle" currently in-use?
	je s	ValHX		;No, "Handle" is valid -- go exit.
ValHE:	mov	bl,ERR_BADHNDL	;Invalid "Handle"!  Set error code.
	pop	cx		;"Pitch" our return address.
ValHX:	ret			;Exit.
	db	0		;(Unused alignment "filler").
;
; Subroutine to check that a "Handle" is within our table and on
;   a 10-byte boundary.   If not, the carry flag is set on exit.
;   The AX-register is zeroed, and all others are saved.
;
ChkHdl:	cmp	ax,08000h	;Is "Handle" above end of table?
@HTEnd3	equ	[$-2].lw	;(End-of-table address, Init set).
	jae s	ChkHE		;Yes, set carry flag and exit.
	sub	ax,offset HTbl	;Is "Handle" below start of table?
@HTbl4	equ	[$-2].lw
	jb s	ChkHE		;Yes, set carry flag and exit.
	div	cs:MSHTSiz	;Divide table offset by entry size.
	and	ax,0FF00h	;Is "Handle" on a 10-byte boundary?
	jnz s	ChkHE		  ;No, set carry flag and exit.
	mov	ds,cs:MSHTPtr.hw  ;Point DS-reg. to "Handles" table.
	ret			  ;Exit.
ChkHE:	xor	ax,ax		;Bad "Handle"!  Clear AX-register.
	stc			;Set carry flag (error!) and exit.
	ret
;
; Driver Int 15h Handler.   Our "A20" status must be saved, and the
;   "A20" line must be re-enabled (if needed) after a "block move".
;
I15Nxt:	popf			;Not for us -- reload prior CPU flags.
	db	0EAh		;Go to next Int 15h handler in chain.
PrevI15	dd	(I15Ent-@)	;(Previous Int 15h vector, Init set).
IntRtn:	jc s	I2FRtn		;If Int 2F request, use logic below.	
	cmp	ah,087h		;Entry -- "block move" request?
	jne s	I15Mem		;No, test for free-memory request.
	popf			;Save AX-reg. ahead of CPU flags.
	push	ax
	pushf
	cli			;Ensure CPU interrupts are disabled.
	push	cx		;Save registers used by "TstA20".
	push	si
	push	di
	push	ds
	push	es
	call	TstA20		;Get current "A20" status.
	pop	es		;Reload registers used by "TstA20".
	pop	ds
	pop	di
	pop	si
	pop	cx
	popf
	push	bp		;Reload AX-reg. & save "A20" status.
	mov	bp,sp
	xchg	ax,[bp+2]
	pop	bp
	pushf			;Call previous Int 15h handler.
	call	cs:PrevI15
	pushf			;Save returned CPU flags.
	push	bp		;Swap AX-reg. & saved "A20" status.
	mov	bp,sp
	xchg	ax,[bp+4]
	pop	bp
	cmp	al,1		;Should "A20" line be re-enabled?
	jne s	I15Fin		;No, reload flags & AX-reg. and exit.
	cli			;Re-enable our "A20" line.
	call	A20Hdl
I15Fin:	popf			;Reload CPU flags and AX-register.
	pop	ax
	jmp s	I15End		;Go set returned carry flag & exit.
I15Mem:	cmp	ah,088h		;Request for free extended memory?
	jne s	I15Nxt		;No, go to next Int 15h handler.
	xor	ax,ax		;Return "null" free-memory count.
I15NoC:	popf			;Discard CPU flags saved on entry.
	clc			;Reset carry flag on exit.
	nop			;(Unused alignment "filler").
I15End:	push	bp		;Point to our current stack.
	mov	bp,sp
	rcr	[bp+6].lb,1	;Post carry bit in exiting flags.
	rol	[bp+6].lb,1
	pop	bp		;Reload BP-register and exit.
	iret
	db	0		;(Unused alignment "filler").
;
; Interrupt 02Fh Handler, for XMS driver-loaded or entry-point calls.
;
I2FF09:	cmp	ax,04309h	;Microsoft "Handles Table" request?
	jne s	I2FF10		;No, test for "Entry Point" request.
	mov	al,043h		;Return "Handles Table" signature.
	mov	bx,(MSHT-@)	;Return ES:BX "Handles Table" pointer.
	push	cs
	pop	es
	jmp s	I15NoC		;Go discard CPU flags & exit.
I2FNxt: popf			;Not for us -- reload prior CPU flags.
	db	0EAh		;Go to next Int 2Fh handler in chain.
PrevI2F	dd	0		;(Previous Int 2Fh vector, Init set).
I2FRtn:	cmp	ax,04300h	;"XMS Driver Loaded" request?
	jne s	I2FF09		;No, test for "Handles Table" request.
	mov	al,080h		;Return "Driver Loaded" signature.
	jmp s	I15NoC		;Go discard CPU flags & exit.
I2FF10:	cmp	ax,04310h	;"XMS Entry Point" request?
	jne s	I2FNxt		;No, go to next Int 2Fh handler.
	mov	bx,(XMSEnt-@)	;Return ES:BX "XMS Entry-Point".
	push	08000h
@I2FSeg	equ	[$-2].lw	;("Stub" segment address, Init set).
	pop	es
	jmp s	I15NoC		;Go discard CPU flags & exit.
	db	0		;(Unused alignment "filler").
;
; Subroutine to test if the "A20" line is on.
;
TstA20:	mov	ax,0FFFFh	;Set DS:SI-regs. to FFFF:0090h.
	mov	ds,ax
	mov	si,00090h
	inc	ax		;Set ES:DI-regs. to 0000:0080h.
	mov	es,ax
	mov	di,00080h
	cld			;Compare 4 words at FFFF:0090h
	mov	cx,4		;   v.s. 4 words at 0000:0080h.
	repe	cmpsw
	je s	TsA20X		;Are the two areas the same?
	inc	ax		;No, "A20" is on -- post "success".
TsA20X:	ret			;Exit.
;
; Standard "A20" Handler.   Driver initialization overlays this
;   routine with the PS/2 handler ("P92Hdl" below) when needed.
;   All registers except the AX-reg. are saved.
;
A20Hdl:	push	cx		;Save CX-register for delay loops.
	mov	ah,0DFh		;Set enable byte for below.
	cmp	al,1		;Is this an enable request?
	je s	A20Go		;Yes, go do it.
	mov	ah,0DDh		;Set disable byte for below.
A20Go:	call	Sync42		;Ensure keyboard controller is ready.
	jnz s	A20Err		;If not, post failure and exit.
	mov	al,0D1h		;Send 0D1h byte.
	out	064h,al
	call	Sync42		;Wait for keyboard controller ready.
	jnz s	A20Err		;If error, post failure and exit.
	mov	al,ah		;Send enable or disable byte.
	out	060h,al
	call	Sync42		;Wait for keyboard controller ready.
	jnz s	A20Err		;If error, post failure and exit.
	mov	al,0FFh		;Send 0FFh (pulse output port NULL).
	out	064h,al
	call	Sync42		;Wait for keyboard controller ready.
	jz s	A20OK		;If no error, post success & exit.
	cmp	ah,0DDh		;Was this a disable request?
	je s	A20OK		;Yes, post success anyway and exit.
A20Err:	xor	ax,ax		;ERROR!  Return failure code.
	pop	cx		;Reload CX-register and exit.
	ret
A20OK:	mov	ax,1		;O.K. -- Return success code.
	pop	cx		;Reload CX-register and exit.
	ret
Sync42:	xor	cx,cx		;8042 "sync" -- reset loop counter.
SyLoop:	in	al,064h		;Get 8042 status
	and	al,002h		;Mask off "A20" flag.
	loopnz  SyLoop		;If not set, loop back if we can.
	ret			;Exit -- AX-reg. is zero if O.K.
;
; XMS Function 10h -- Request Upper Memory Block.
;
ReqUMB:	xor	bp,bp		        ;Clear largest-block bucket.
	mov	si,offset (UMBTbl-4-@)  ;Point to upper-memory table.
RqUMB1:	add	si,4		        ;Advance to next table entry.
	mov	cx,[si]		;Get next upper-memory segment.
	jcxz	RqUMB3		;If end, see if we found anything.
	inc	cx		;Has this entry already been used?
	jz s	RqUMB1		;Yes, advance to next entry.
	mov	cx,[si+2]	;Get memory block size in "pages".
	cmp	cx,bp		;Is this the largest block yet?
	jbe s	RqUMB2		;No, see if we can use it.
	mov	bp,cx		;Update largest-block bucket.
RqUMB2:	cmp	cx,dx		;Block size >= request size?
	jb s	RqUMB1		;No, advance to next entry.
	mov	bx,0FFFFh	;Swap memory segment & "used" flag.
	xchg	bx,[si]
	mov	dx,cx		;Set memory block size.
	inc	ax		;Return "success" and exit.
	ret
RqUMB3:	mov	dx,bp		;No useable block -- set largest.
	mov	bl,ERR_NOUMB	;Set "No UMB" return code.
	or	dx,dx		;Any free upper memory found?
	jz s	RqUMBX		;No, return "failure" & exit.
	dec	bx		;Set "smaller UMB" return code.
RqUMBX:	ret			;Return "failure" and exit.
	db	0		;(Unused alignment "filler").
;
; Upper-Memory Descriptor Table.
;
UMBTbl	dd	10 dup (0)	;10 4-byte table entries.
UMBLEN	equ	$-UMBTbl	;Length of this table.
UMBEnd	dw	0		;Table "end" marker.
;
; "I-O Catcher" Variables.
;
WSSeg	dw	0		    ;Segment of DOS "workspace" ptr.
DOSPtr	dd	0		    ;Current DOS "workspace" ptr.
CHSTbl	db	(MAXBIOS*2) dup (0) ;Disk CHS parameters (Init set).
IOBlock	dw	16		    ;"DAP" I-O block:  16-byte size.
	dw	0		    ;  I-O sector count  (set to 1).
IOBuf	dd	0		    ;  I-O buffer offset (always 0).
LBA	dd	0		    ;  Variable lower LBA address.
LBA2	dw	0,0		    ;  Variable upper LBA address.
;
; Diskette Entry Routine.
;
DsktIO:	pushf			;Diskette -- save CPU flags.
	pusha			;Save all 16-bit CPU registers.
	shr	dl,1		;Is request for BIOS unit 0 or 1?
	jnz s	D_Pass		;No, go "pass" request to BIOS.
	shr	ah,1		;Shift out write request bit.
	dec	ah		;Is this a CHS read or write?
	jnz s	D_Pass		;No, go "pass" request to BIOS.
	xchg	ax,cx		;Any I-O sectors to process?
	jcxz	D_Pass		;No, go "pass" request to BIOS.
	xchg	al,ch		;Save sector no. with sector count.
	mov	dx,es		;Get user I-O buffer segment.
	cmp	dh,0A0h		;Is I-O request for upper-memory?
	jb s	D_Pass		;No, go "pass" request to BIOS.
	push	ds		;Save CPU segment registers.
	push	es
	jmp	SetBuf		;Go stack working buffer pointer.
D_Pass:	popa			;"Pass" request -- reload all regs.
	popf			;Reload CPU flags saved at entry.
	db	0EAh		;Go to next routine in Int 40h chain.
@DsktV	dd	0		;(Prior Int 40h vector, set by Init).
;
; Hard Disk Entry Routine.
;
DiskIO:	pushf			 ;Hard disk -- save CPU flags.
	pusha			 ;Save all 16-bit CPU registers.
	shl	dl,1		 ;Is this a hard-disk unit number.
	jnc s	QuickX		 ;No, not our request -- exit quick!
	cmp	dl,(MAXBIOS*2)-2 ;Is hard-disk unit above our limit?
	ja s	QuickX		 ;Yes, not our request -- exit fast!
	movzx	di,dl		 ;Get disk's CHS parameters.
	mov	di,cs:[di+CHSTbl-@]
	test	di,00080h	 ;Is this disk disabled (no EDD)?
	js s	QuickX		 ;Yes, ignore it and exit quick!
	push	ds		 ;Save CPU segment registers.
	push	es
	shr	ah,1		;Shift out write bit.
	dec	ah		;CHS read or write request?
	jnz s	ChkLBA		;No, check for LBA request.
	xchg	ax,cx		;Swap sector count and CHS address.
	mov	si,0003Fh	;Set SI-reg. to starting sector.
	and	si,ax
	dec	si
	shr	al,6		;Set AX-reg. to starting cylinder.
	xchg	al,ah
	xchg	ax,bp		;Save starting cylinder number.
	mov	ax,di		;Get disk CHS sectors/head value.
	mul	dh		;Convert head to sectors.
	add	si,ax		;Add result to starting sector.
	xchg	ax,di		;Load both CHS parameter values.
	xor	di,di		;Reset upper LBA address bits.
	cmp	ax,di		;Are disks' CHS parameters valid?
	je s	Pass		;No, go "pass" request to BIOS.
	mul	ah		;Convert cylinder to sectors.
	mul	bp
	add	si,ax		;Add to head/sector value.
	adc	dx,di
	jmp s	ValSC		;Go validate I-O sector count.
ChkLBA:	sub	ah,020h		;Is this an LBA read or write?
	jnz s	Pass		;No, go "pass" request to BIOS.
	mov	cl,[si].DapSC	;Get "DAP" I-O sector count.
	les	dx,[si].DapLBA1	;Get "DAP" LBA bits 16-47.
	mov	di,es
	les	bx,[si].DapBuf	;Get "DAP" I-O buffer pointer.
	mov	si,[si].DapLBA	;Get "DAP" LBA bits 0-16.
ValSC:	jcxz	Pass		;If no sectors, go "pass" request.
	mov	ax,es		;Get I-O buffer segment.
	cmp	ah,0A0h		;Is I-O request for upper-memory?
	jae s	SetLBA		;Yes, go set LBA & other parameters.
Pass:	pop	es		;"Pass" request -- reload all regs.
	pop	ds
QuickX:	popa			;Reload all 16-bit CPU registers.
	popf			;Reload CPU flags saved at entry.
	db	0EAh		;Go to next handler or to the BIOS.
@HDiskV	dd	0		;(Prior hard-disk vector, Init set).
SetLBA:	push	dx		;Set our "DAP" 48-bit LBA.
	push	si
	pop	cs:LBA
	mov	cs:LBA2,di
	mov	al,0C0h		;Do all hard disk I-O in LBA mode.
;
; Common Diskette and Hard Disk Setup Logic.
;
SetBuf:	push	es		;Stack working I-O buffer pointer.
	push	bx
	mov	bp,sp		;Point BP-reg. to stack variables.
	mov	[bp+14],cx	;Set sector count & CHS sector no.
	or	[bp+23],al	;If hard disk, adjust request code.
ChkWSP:	push	cs		;Point DS-register to this driver.
	pop	ds
@NoWSP:	mov	si,(WSSeg-@)	      ;Point to our "workspace" seg.
	les	bx,[si+DOSPtr-WSSeg]  ;Point to DOS "workspace" ptr.
	mov	ax,es:[bx+2]	      ;Get DOS "workspace" segment.
	cmp	ax,[si]		      ;New "workspace" pointer set?
	je s	Next		      ;No, use existing buffer ptr.
	mov	[si],ax		      ;Save new DOS "workspace" seg.
	push	es:[bx].dwd	      ;Save new DOS "workspace" ptr.
	pop	[si+IOBuf-WSSeg].dwd
;
; Main Driver I-O Routine.
;
Next:	mov	ax,[bp+22]	;Reload our I-O request code.
	les	bx,IOBuf	;Set our I-O buffer pointer.
	test	ah,001h		;Is this a write request?
	jz s	Params		;No, set all I-O parameters.
	pop	si		;Set user buffer as source.
	pop	ds
	mov	di,bx		;Set our I-O buffer as destination.
	cld			;Move output data to our buffer.
	mov	cx,256
	rep	movsw
	push	ds		;Save updated user buffer pointer.
	push	si
Params:	mov	dx,[bp+18]	;Reload unit number & CHS head.
	mov	si,(IOBlock-@)	;Point to our "DAP" I-O block.
	push	cs
	pop	ds
	mov	[si].DapSC,1	;Set "DAP" sector count of 1.
	mov	di,(@HDiskV-@)	;Point to prior hard-disk vector.
	btr	ax,15		;Is this a hard-disk request?
	jc s	DoIO		;Yes, do hard-disk I-O below.
	mov	al,1		;Set CHS sector count of 1.
	mov	cl,[bp+15]	;Reload CHS sector & cylinder.
	mov	ch,[bp+21]
	mov	di,(@DsktV-@)	;Point to prior diskette vector.
DoIO:	push	[bp+24].lw	;Reload CPU flags saved at entry.
	popf
	pushf			;Have BIOS do next sector's I-O.
	call	[di].dwd
	mov	bp,sp		;Point to our stack variables.
	jc s	Exit		;If any errors, exit immediately!
	sti			;If needed, re-enable interrupts.
	xor	bx,bx		;Zero BX-reg. for our logic.
	test	[bp+23].lb,001h	;Is this a read request?
	jnz s	Update		    ;No, go update disk address.
	lds	si,cs:[bx+IOBuf-@]  ;Set our I-O buffer as source,
	pop	di		    ;Set user I-O buffer as dest.
	pop	es
	cld			;Move input data to user buffer.
	mov	cx,256
	rep	movsw
	push	es		;Save updated user buffer pointer.
	push	di
Update:	push	cs		;Point DS-register to this driver.
	pop	ds
	inc	[bp+15].lb	   ;Bump diskette sector number.
	add	[bx+LBA-@].dwd,1   ;Bump hard disk 48-bit LBA.
	adc	[bx+LBA2-@].lw,bx
	dec	[bp+14].lb	   ;Decrement I-O sector count.
	jnz s	Next		;If more sectors to go, loop back.
Exit:	mov	[bp+23],ah	;Set return code in exiting AH-reg.
	rcr	[bp+30].lb,1	;Set carry in exiting status flags.
	rol	[bp+30].lb,1
	pop	ax		;Discard working I-O buffer pointer.
	pop	ax
	pop	es		;Reload all 16-bit CPU registers.
	pop	ds
	popa
	popf			;Discard CPU flags saved at entry.
	iret			;Exit.
UMCSIZE	equ	$-ReqUMB	;"Dismissable" UMBPCI logic size.
;
; Start of XMS "Handles" Table.
;
	align	16
HTbl	dw	00001h		;1st "Handle" is always free memory.
	dd	0,0
;
; Scan-Memory Routine.   This logic is called only on the first non-
;   version user request by the XMS entry logic.    It asks the BIOS
;   about extended memory, "hooks" Int 15h, and returns to "XCClrT".
;   Read the XMS Specs for why memory CANNOT be reserved until AFTER
;   that first request!   This logic becomes our "Handles" table, to
;   minimize the resident driver size.
;
ScanM:	mov	ax,(XCExit-@)	;Avoid re-entry while we are here.
	mov	Dspatch,ax
	sti			;Re-enable CPU interrupts.
	push	cx		;Save all needed CPU registers.
	push	di		;(CX, DI, ES first, as they reload
	push	es 		;  last at "XCClrT" after clearing
	push	ebx		;  the "Handles" table.  We do not
	push	edx		;  need to save EAX, ESI, or EBP).
ScnM0:	db	34 dup (090h)	;Unused "filler", for more free-
				;  memory "Handles" with "E820h".
	xor	ebx,ebx		;Zero "E820h" continuation value.
	mov	edx,"SMAP"	;Load "SMAP" signature constant.
	mov	si,(HTbl-@)	;Initialize "Handles" table ptr.
@HTbl5	equ	[$-2].lw
ScnM1:	mov	eax,00000E820h	;Set BIOS "E820h" parameters.
	xor	ecx,ecx
	mov	cl,20
	mov	di,offset MList
@ScanA1	equ	[$-2].lw
	push	cs
	pop	es
	push	edx		;Save "SMAP" signature constant.
	push	si		;Save pointers and DS-register.
	push	di
	push	ds
	int	015h
@NoE820	equ	[$-2].lw	;("stc"/"nop" if no "E820h" logic).
	pop	ds		;Reload DS-register and pointers.
	pop	di
	pop	si
	pop	edx		;Reload "SMAP" signature constant.
	jc	ScnM9		;If error/end, check for any memory.
	cmp	eax,edx		;Did BIOS return "SMAP" in EAX-reg.?
	jne s	ScnM2		;No?  Handle same as error/end.
	sub	ecx,20		;Did BIOS return a list size of 20?
ScnM2:	jne s	ScnM9		;No?  Handle same as error/end.
	dec	[di+16].dwd	;Is this a block of "real" memory?
	jnz s	ScnM8		;No, ignore this list element.
	cmp	ecx,[di+4]	;Any "upper" address bits set?
	jne s	ScnM8		;Yes, ignore this list element.
	cmp	ecx,[di+12]	;Any "upper" length bits set?
	mov	eax,[di+8]	;(Get memory block size & address).
	mov	edi,[di]
	jne s	ScnM3		;Yes, set block size to maximum.
	mov	ebp,eax		;Get ending block address.
	add	ebp,edi
	cmc			;Does this block exceed 0FFFFFFFFh?
	jbe s	ScnM4		;No, test for 1-KB block alignment.
ScnM3:	xor	eax,eax		;Set block size to maximum.
	sub	eax,edi
ScnM4:	sub	cx,di		;Get negative lower block address.
	and	ch,003h		;Mask off lower 10 bits and "round"
	add	edi,ecx		;  block up to next 1-KB if needed.
	sub	eax,ecx		;Any memory left after "rounding"?
	jbe s	ScnM8		;No?  Ignore this list element.
	cmp	edi,000100000h	;Where does this memory block start?
	ja s	ScnM6		;Above 1-MB -- check for valid HMA.
	jb s	ScnM8		;Below 1-MB -- ignore list element.
	cmp	eax,000080000h	;At the HMA -- is it 512-KB or more?
	jb s	ScnM10		;No?  ERROR -- go try "E810h" logic.
	mov	ecx,000010000h	;Delete 64K from block for our HMA.
	add	edi,ecx
	sub	eax,ecx
	jmp s	ScnM7		;Go add block to "Handles" table.
ScnM6:	cmp	si,(HTbl-@)	;Have we already found a valid HMA?
@HTbl6	equ	[$-2].lw
	je s	ScnM10		;No?  ERROR -- go try "E810h" logic.
ScnM7:	shr	edi,10		;Convert address/length to KBytes.
	shr	eax,10
	call	SetHdl		;Set up next free-memory "Handle".
	cmp	eax,HKAddr	;Above our "Highest-known address"?
	jbe s	ScnM8		;No, test space for more "Handles".
	mov	HKAddr,eax	;Update our "Highest-known address".
ScnM8:	or	ebx,ebx		;Was this the last list element?
	jz s	ScnM9		;Yes, see if we found any memory.
	cmp	si,(HTbl+(7*HDLSIZE)-@)  ;Space for more "Handles"?
@HTbl7	equ	[$-2].lw
	jb	ScnM1		;Yes, check for more memory blocks.
ScnM9:	cmp	si,(HTbl-@)	;Did "E820h" logic find any memory?
@HTbl8	equ	[$-2].lw
	ja s	ScnM18		;Yes, go "hook" Int 15h vector.
ScnM10:	push	ds		;Save our DS-register.
	mov	ax,0E801h	;Ask BIOS about extended memory
	xor	bx,bx		;  using a 1994 "E801h" request.
	xor	cx,cx
	xor	dx,dx
	int	015h
@NoE801	equ	[$-2].lw	;("stc"/"nop" if no "E801h" logic).
	jc s	ScnM12		;If error, try an old 64-MB request.
	mov	bp,ax		;"Merge" AX- and BX-reg. values.
	or	bp,bx		;Any values in AX- and BX-regs.?
	jnz s	ScnM11		;Yes, check for memory "hole".
	xchg	ax,cx		;Use CX- and DX-regs. instead.
	mov	bx,dx
ScnM11:	movzx	eax,ax		;Get memory KBytes below 16-MB.
	movzx	ebx,bx		;Get memory KBytes above 16-MB.
	shl	ebx,6
	cmp	ax,03C00h	;Any memory "hole" below 16-MB?
	jb s	ScnM13		;Yes, check for at least 512K.
	ja s	ScnM12		;ERROR if > 15-MB!  Try old request.
	add	eax,ebx		;Merge memory into one linear span.
	xor	ebx,ebx
	jmp s	ScnM16		;Go set up 1st free-memory "Handle".
ScnM12:	xor	ax,ax		;Ask BIOS about extended memory with
	mov	ah,088h		;  an old 64-MB request.   It is our
	int	015h		;  "last chance" for extended memory!
	jc s	ScnM14		;If this request fails, we are KAPUT!
	movzx	eax,ax		;Get total XMS memory size.
	xor	ebx,ebx		;Clear "Above 16-MB" XMS size.
ScnM13:	cmp	ax,512		;At least 512K of XMS below 16-MB?
	jae s	ScnM16		;Yes, go set free-memory sizes.
ScnM14:	pop	ds		;BAAAD News -- NO extended memory!
	pop	edx		;Reload all CPU registers.
	pop	ebx
	pop	es
	pop	di
	pop	cx
	cli			;Disable CPU interrupts.
	mov	ax,(ScanM-@)	;Permit our boy to "try it again" --
@ScanA2	equ	[$-2].lw	;  perhaps he can "unload" something!
	mov	Dspatch,ax
	xor	ax,ax		;Return "failure" status and exit.
	mov	bl,ERR_NOMEMORY
ScnM15:	jmp	XCExit
ScnM16:	pop	ds		;Success!  Reload our DS-register.
	mov	si,(HTbl-@)	;Point to our "Handles" table.
@HTbl9	equ	[$-2].lw
	sub	eax,64		;Subtract 64K for our HMA.
	mov	edi,000000440h	;Set up first free-memory "Handle"
	call	SetHdl		;  with a starting address of 1088K.
	or	ebx,ebx		;Second free-memory area at 16-MB?
	jz s	ScnM17		;No, post "Highest-known address".
	xchg	eax,ebx		;Set up second free-memory "Handle"
	mov	di,04000h	;  with a starting address of 16-MB.
	call	SetHdl
ScnM17:	mov	HKAddr,eax	;Post our "Highest-known address".
ScnM18:	pop	edx		;Reload regs. not needed below.
	pop	ebx
	cli			;Disable CPU interrupts.
	mov	ax,(XCCall-@)	;Avoid entering this routine again.
	mov	Dspatch,ax
	push	0		;Point ES:DI-rgs. to Int 15h vector.
	pop	es
	mov	di,(00015h * 4)
	cld			;Save previous Int 15h vector
	mov	eax,es:[di]	;  and install our own instead.
	xchg	eax,PrevI15.dwd
	stosd
	mov	di,si		;Point ES:DI-regs. to next "Handle"
	les	cx,MSHTPtr	;  and get "Handles" base in CX-reg.
	mov	ax,di		;Get number of "Handles" used above.
	sub	ax,cx
	div	MSHTSiz
	mov	cx,DEFHDLS	;Get number of "Handles" to reset.
@HdlCt4	equ	[$-2].lw	;("Handles" count, set by Init).
	sub	cl,al
	xor	eax,eax		;Set regs. to clear "Handles" table.
	mov	bp,00004h
ScnM20:	jmp	XCClrT		;Clear "Handles" table & do request.
;
; Subroutine to set up the next free-memory "Handle".
;
SetHdl:	push	ds		;Save our DS-register.
	mov	ds,MSHTPtr.hw	;Point DS-reg. to "Handles" table.
	mov	[si].HFlag.lw,1	;Flag "Handle" as free memory.
	mov	[si].HBase,edi	;Post "Handle" 32-bit base address.
	mov	[si].HSize,eax	;Post "Handle" byte count.
	pop	ds		;Reload our DS-register.
	add	si,HDLSIZE	;Advance to next "Handle" in table.
	add	eax,edi		;Return ending block address.
	shl	eax,10
	dec	eax
	ret			;Exit.
MList	equ	$		;"E820h memory list" begins here.
;
; Initialization "A20" Standard Handler.
;
I_A20:	test	DvrSw,080h	;Are we using "I-O port 092h" logic?
	jnz s	P92Hdl		;Yes, use "Port 92h" handler below.
	push	cx		;Save CX-register for delay loops.
	mov	ah,0DFh		;Set enable byte for below.
	cmp	al,1		;Is this an enable request?
	je s	I_Go20		;Yes, go do it.
	mov	ah,0DDh		;Set disable byte for below.
I_Go20:	call	I_Sync		;Ensure keyboard controller is ready.
	jnz s	I_No20		;If not, post failure and exit.
	mov	al,0D1h		;Send 0D1h byte.
	out	064h,al
	call	I_Sync		;Wait for keyboard controller ready.
	jnz s	I_No20		;If error, post failure and exit.
	mov	al,ah		;Send enable or disable byte.
	out	060h,al
	call	I_Sync		;Wait for keyboard controller ready.
	jnz s	I_No20		;If error, post failure and exit.
	mov	al,0FFh		;Send 0FFh (pulse output port NULL).
	out	064h,al
	call	I_Sync		;Wait for keyboard controller ready.
	jz s	I_20X		;If no error, post success & exit.
	cmp	ah,0DDh		;Was this a disable request?
	je s	I_20X		;Yes, post success anyway and exit.
I_No20:	xor	ax,ax		;ERROR!  Return failure code.
	pop	cx		;Reload CX-register and exit.
	ret
I_20X:	mov	ax,1		;O.K. -- Return success code.
	pop	cx		;Reload CX-register and exit.
	ret
I_Sync:	xor	cx,cx		;8042 "sync" -- reset loop counter.
I_SyLp:	in	al,064h		;Get 8042 status
	and	al,002h		;Mask off "A20" flag.
	loopnz  I_SyLp		;If not set, loop back if we can.
	ret			;Exit -- AX-reg. is zero if O.K.
;
; I-O Port 092h "A20" Handler, for IBM PS/2 and other systems.
;   All registers except the AX-reg. are saved.
;
P92Hdl:	push	cx		;Save CX-register.
	xor	cx,cx		;Reset delay-loop counter.
	or	ax,ax		;Is this an enable request?
	in	al,P92PORT	;(Get current "A20" state).
	jz s	P92Dis		;No, use disable logic below.
	test	al,P92FLAG	;Is "A20" line already on?
	jnz s	P92Err		;Yes?  Return failure & exit!
	or	al,P92FLAG	;Turn on the "A20" line.
	out	P92PORT,al
P92On:	in	al,P92PORT	;Loop until "A20" line is on.
	test	al,P92FLAG
	loopz	P92On
	jz s	P92Err		;Never came on?  Return failure!
P92OK:	xor	ax,ax		;Return success and clear carry.
	inc	ax
	pop	cx		;Reload CX-register and exit.
	ret
P92Dis:	and	al,NOT P92FLAG	;Disable -- turn off "a20" line.
	out	P92PORT,al
P92Off:	in	al,P92PORT	;Loop until "A20" line is off.
	test	al,P92FLAG
	loopnz  P92Off
	jz s	P92OK		;If "A20" off, return success.
P92Err:	xor	ax,ax		;Return failure and set carry.
	stc
	pop	cx		;Reload CX-register and exit.
	ret
P92LEN	equ	$-P92Hdl	;Length of this handler.
;
; Initialization Variables.
;
VDSLen	dd	0		;VDS driver length  (set above).
	dd	0		;VDS 32-bit offset  (always zero).
VDSSeg	dd	0		;VDS 16-bit segment (set above).
	dd	0		;VDS 32-bit address.
Packet	dd	0		;DOS "Init" packet pointer.
UMBSig	db	'$UMBTbl!'	;UMBPCI memory table "signature".
DvrLen	dw	0		;Declared driver length.
ResSeg	dw	05000h		;"Boot" resident segment.
UMSeg	dw	0		;1st UMBPCI upper-memory segment.
DsktIRQ	dw	0		;Diskette set-IRQ number (if any).
P92Opt	db	0		;/P "Port 92h Always/Never" flag.
WSFlag	db	0		;/W "workspace" flag.
DriveCt	db	0		;Number of drives detected.
HDUnit	db	080h		;Current BIOS hard-disk unit no.
HDCount	db	0		;Current BIOS hard-disk counter.
;
; Initialization Messages.
;
TTLMsg	db	CR,LF,'XMGR'
BootFl	db	'$Boot$'
TTLMsg2	db	', 10-16-2011.   '
HdlMs	db	' 48 XMS handles.',CR,LF,'$'
A20EM	db	'NOTE:  A20 line found ON!',CR,LF,'$'
EDDMsg	db	'No EDD BIOS logic, Unit 8'
EDUnit	db	'0h ignored!',CR,LF,'$'
CHSMsg	db	'CHS data error, BIOS will do Unit 8'
CHUnit	db	'0h CHS I-O.',CR,LF,'$'
MgrMs	db	'Another XMS manager present$'
UMBMs	db	'Not enough UMBPCI memory$'
A20FM	db	'A20 line failed tests$'
VDSMsg	db	'VDS init error$'
PRMsg	db	'No 80386+ CPU'
Suffix	db	'; XMGR not loaded!',CR,LF,'$'
;
; DOS "Strategy" Routine, used only to handle the DOS "Init" packet.
;   This routine MUST be placed above all run-time logic, to prevent
;   CPU cache "code modification" ERRORS!
;
Strat:	mov	cs:Packet.lw,bx	;Save DOS "Init" packet pointer.
	mov	cs:Packet.hw,es
	retf			;Exit and await "Device Interrupt".
;
; DOS "Device Interrupt" Routine.   This logic initializes the driver.
;
DevInt:	pushf			;Entry -- save CPU flags.
	push	ds		;Save CPU segment registers.
	push	es
	push	ax		;Save needed 16-bit CPU registers.
	push	bx
	push	dx
	push	cs		;Set this driver's DS-register.
	pop	ds
	les	bx,Packet	;Point to DOS "Init" packet.
	cmp	es:[bx].RPOp,0	;Is this really an "Init" packet?
	jne s	I_Quit			    ;No?  Reload regs. & exit!
	mov	es:[bx].RPStat,RPDON+RPERR  ;Set init packet defaults.
	mov	es:[bx].RPSeg,cs
	and	es:[bx].RPLen,0
	push	sp		;See if CPU is an 80286 or newer.
	pop	ax		;(80286+ push SP, then decrement it).
	cmp	ax,sp		;Did SP-reg. get saved "decremented"?
	jne s	I_Junk		;Yes, CPU is an 8086/80186, TOO OLD!
	pushf			;80386 test -- save CPU flags.
	push	07000h		;Try to set NT|IOPL status flags.
	popf
	pushf			;Get resulting CPU status flags.
	pop	ax
	popf			;Reload starting CPU flags.
	test	ah,070h		;Did any NT|IOPL bits get set?
	jnz s	I_386		;Yes, CPU is at least an 80386.
I_Junk:	mov	dx,(PRMsg-@)	;Display "No 80386+ CPU" message.
	call	I_Msg
I_Quit:	jmp	I_Exit		;Go reload registers and exit quick!
I_386:	pushad			;80386+ CPU:  Save 32-bit registers.
	les	si,es:[bx].RPCL	;Point to command line that loaded us.
	xor	bx,bx		;Reset BX-register and avoid re-entry.
	mov	[bx+StratP-@].dwd,(((DummyX-@)*65536)+(Dummy-@))
I_NxtC:	mov	ax,es:[si]	;Get next 2 command-line bytes.
	inc	si		;Bump pointer past first byte.
	cmp	al,0		;Is byte the command-line terminator?
	je s	I_TTLJ		;Yes, go display our "title" message.
	cmp	al,LF		;Is byte an ASCII line-feed?
	je s	I_TTLJ		;Yes, go display our "title" message.
	cmp	al,CR		;Is byte an ASCII carriage-return?
I_TTLJ:	je	I_TTL		;Yes, go display our "title" message.
	cmp	al,'-'		;Is byte a dash?
	je s	I_NxtS		;Yes, see what next "switch" byte is.
	cmp	al,'/'		;Is byte a slash?
	jne s	I_NxtC		;No, check next command-line byte.
I_NxtS:	mov	al,ah		;Get byte after the slash or dash.
	and	al,0DFh		;Mask out lower-case bit (020h).
	cmp	al,'Z'		;Is this byte a "Z" or "z"?
	jne s	I_ChkW		;No, check for "W" or "w".
	inc	si		;Bump pointer past the "Z" or "z".
	mov	[@MovSC+1].lw,8 ;Set 2K protected-mode section ct.
I_ChkW:	cmp	al,'W'		;Is this byte a "W" or "w"?
	jne s	I_ChkB		;No, check for "B" or "b".
	inc	si		;Bump pointer past the "W" or "w".
	mov	WSFlag,al	;Set "workspace" flag for UMBPCI use.
I_ChkB:	cmp	al,'B'		;Is this byte a "B" or "b"?
	jne s	I_ChkT		;No, check for "T" or "t".
	inc	si		;Bump pointer past the "B" or "b".
	mov	BootFl,' '	;Enable "Boot" in title message.
I_ChkT:	cmp	al,'T'		;Is this byte a "T" or "t"?
	jne s	I_ChkM		;No, check for "M" or "m".
	inc	si		;Bump pointer past the "T" or "t".
	mov	al,es:[si]	;Get next command-line byte.
	cmp	al,'7'		;Is byte more than a "7"?
	ja s	I_NxtC		;Yes, check next command-line byte.
	sub	al,'0'		;Is byte less than a "0"?
	jb s	I_NxtC		;Yes, check next command-line byte.
	inc	si		;Bump pointer past "test" number.
	mov	DvrSw,al	;Post /T "memory scan" flag.
	jmp s	I_NxtC		;Continue scanning for a terminator.
I_ChkM:	cmp	al,'M'		;Is this byte an "M" or "m"?
	jne s	I_ChkP		;No, check for "P" or "p".
	inc	si		;Bump pointer past the "M" or "m".
	mov	al,es:[si]	;Get next command-line byte.
	cmp	al,'1'		;Is byte less than a "1"?
	jb s	I_NxtC		;Yes, see if byte is a terminator.
	cmp	al,'8'		;Is byte more than an "8"?
	ja s	I_NxtC		;Yes, see if byte is a terminator.
	inc	si		;Bump pointer past this number.
	shl	al,4		;Set "boot" resident-memory segment.
	mov	ResSeg.hb,al
	jmp s	I_NxtC		;Continue scanning for a terminator.
I_ChkP:	cmp	al,'P'		;Is this byte a "P" or "p"?
	jne s	I_ChkN		;No, check for "N" or "n".
	inc	si		;Bump pointer past the "P" or "p".
	mov	al,es:[si]	;Get next command-line byte.
	and	al,0DFh		;Mask out lower-case bit (020h).
	cmp	al,'A'		;Is this byte an "A" or "a"?
	je s	I_PFlg		;Yes, set it as Port 92h flag below.
	cmp	al,'N'		;Is this byte an "N" or "n"?
	jne s	I_NxtC		;No, continue scan for a terminator.
I_PFlg:	inc	si		;Bump pointer past "A" or "N" byte.
	mov	P92Opt,al	;Set "Always"/"Never" Port 92h flag.
I_NxtJ:	jmp	I_NxtC		;Continue scanning for a terminator.
I_ChkN:	cmp	al,'N'		;Is this byte an "N" or "n"?
	jne s	I_NxtJ		;No, see if byte is a terminator.
	inc	si		;Bump pointer past the "N" or "n".
	mov	ax,es:[si]	;Get next 2 command-line bytes.
	mov	cx,48		;Set up for 48 XMS "handles".
	mov	edx," 84 "
	cmp	ax,"84"		;Does user want 48 XMS "handles"?
	je s	I_SetB		;Yes, go set that value.
	mov	cl,80		;Set up for 80 XMS "handles".
	mov	edx," 08 "
	cmp	ax,"08"		;Does user want 80 XMS "handles"?
	je s	I_SetB		;Yes, go set that value.
	mov	cl,128		;Set up for 128 XMS "handles".
	mov	edx," 821"
	cmp	ax,"21"		;Does user want 128 XMS "handles"?
	jne s	I_NxtJ		;No, see if byte is a terminator.
	cmp	es:[si+2].lb,'8'
	jne s	I_NxtJ
	inc	si		;Bump pointer past extra byte.
I_SetB:	inc	si		;Bump pointer past "Handles" count.
	inc	si
	mov	MSHTCt.lb,cl	;Set driver "Handles" count.
	mov	HdlMs.dwd,edx	;Set count in "Handles" message.
	jmp s	I_NxtJ		;Continue scanning for a terminator.
I_TTL:	mov	dx,(TTLMsg-@)	;Display driver "title" message.
	call	I_Msg
	mov	dx,(TTLMsg2-@)
	call	I_Msg
	mov	ax,04300h	;Inquire about another XMS manager.
	call	I_XMS
	cmp	al,080h		;Has another manager "hooked" Int 2Fh?
	jne s	I_PS2		;No, see about using PS/2 "A20" logic.
	mov	ax,04310h	;Get XMS manager "entry" address.
	call	I_XMS
	cmp	es:[DvrNam].dwd,"RGMX"	;Is it our "boot" driver?
	jne s	I_NotQ			;No?  Display error and exit.
	cmp	es:[DvrNam+4].dwd,"$B2"
	jne s	I_NotQ
	cmp	BootFl,' '	;Trying to load AGAIN in "boot" mode?
	jne	I_QHMB		;No, go to driver "2nd load" routine.
I_NotQ:	mov	dx,(MgrMs-@)	;Point to "Another XMS manager" msg.
	jmp s	I_Err		;Go display error message and exit.
I_PS2:	mov	al,P92Opt	;Get user's Port 92h flag, if any.
	cmp	al,'A'		;Is Port 92h logic "Always" wanted?
	je s	I_PS2A		;Yes, go install Port 92h handler.
	cmp	al,'N'		;Is Port 92h logic "Never" wanted?
	je s	I_A20S		;Yes, go check status of "A20" line.
	mov	ah,0C0h		;Get BIOS "System Descriptor" ptr.
	stc
	int	015h
	jc s	I_A20S		;If none, keep normal "A20" logic.
	mov	al,es:[bx+5]	;Get "Feature Information" byte 1.
	and	ax,00002h	;Are we on an IBM PS/2 system?
	jz s	I_A20S		;No, keep normal "A20" logic.
I_PS2A:	cld			;Overlay regular "A20" handler
	mov	cx,P92LEN	;  with Port 092h handler logic.
	mov	si,(P92Hdl-@)
	mov     di,(A20Hdl-@)
	push	cs
	pop	es
	rep	movsb
	or	DvrSw,080h	;Set "I-O port 092h in use" flag.
	xor	ax,ax		;Do initial disable of "A20" line.
	call	I_A20		;(Fixes a PS/2 Ctl-Alt-Del "bug").
I_A20S:	call	I_T20		;Get "A20" status.
	push	cs		;(Reload our DS-register).
	pop	ds
	xor	al,001h		;Does "A20" appear to be on?
	mov	A20Flag,al	;Yes, post it permanently enabled.
	cmp	al,1		;Can "A20" line be changed?
	je s	I_A20T		;Yes, see if it tests properly.
	mov	dx,(A20EM-@)	;Display "A20 already on" message.
	call	I_Msg
	jmp s	I_Hook		;Go "hook" this driver into Int 2Fh.
I_A20T:	call	I_A20		;"A20" test -- enable the "A20" line.
	or	ax,ax		;Any "A20" errors?
	jz s	I_A20E		;Yes, display error message and exit.
	call	I_T20		;Get "A20" status.
	push	cs		;(Reload our DS-register).
	pop	ds
	or	ax,ax		;Does "A20" appear to be on?
	jz s	I_A20E		;No, display error message and exit.
	xor	ax,ax		;Disable the "A20" line.
	call	I_A20
	or	ax,ax		;Any "A20" errors?
	jz s	I_A20E		;Yes, display error message and exit.
	call	I_T20		;Get "A20" status.
	push	cs		;(Reload our DS-register).
	pop	ds
	or	ax,ax		;Does "A20" appear to be off?
	jz s	I_Hook		;Yes, "hook" this driver to Int 2Fh.
I_A20E:	mov	dx,(A20FM-@)	;Point to "A20 failed tests" message.
I_Err:	call	I_Msg		;Init ERROR!  Display message.
	mov	dx,(Suffix-@)	;Display error-message suffix.
	call	I_Msg
	jmp	I_Fail		;Go reload all registers and exit.
I_Hook:	mov	ax,0352Fh	;Get & save current Int 2Fh vector.
	call	I_DOS
	push	es
	push	bx
	pop	PrevI2F
	cmp	BootFl,' '	;Are we loading in "boot" mode?
	je	I_Boot		;Yes, do "boot" setup below.
;
; Initialization logic for XMGR if it loads in UMBPCI upper-memory.
;
	mov	bp,0C000h	;Initialize upper-memory scan.
I_UMB1:	mov	cx,2		;Examine next upper-memory "page".
	mov	si,offset UMBSig
	xor	di,di
	mov	es,bp
	rep	cmpsd		;Starting UMBPCI signature O.K.?
	jne s	I_UMB2			 ;No, bump to next "page".
	cmp	es:[di+UMBLEN+4],0AB5Fh  ;Ending signature O.K.?
	je s	I_UMB3			 ;Yes, we found our table.
I_UMB2:	inc	bp		;Bump to next upper-memory "page".
	cmp	bp,0F800h	;At the start of the run-time BIOS?
	jb s	I_UMB1		;No, test next upper-memory "page".
	jmp	I_Norm		;Go do "normal" driver loading.
I_UMB3:	mov	si,di		;Point DS:SI-regs. to UMBPCI data.
	mov	ds,bp
	mov	cl,(UMBLEN/4)	;Save UMBPCI upper-memory table.
	mov	di,(UMBTbl-@)
	push	cs
	pop	es
	rep	movsd
	xor	si,si		;Reset 1st UMBPCI "signature" word
	and	[si].dwd,0	;  so others CANNOT use our memory!
	push	cs		;Reload our DS-register.
	pop	ds
	mov	ax,UMBTbl.lw	;Set 1st UMBPCI upper memory segment.
	mov	UMSeg,ax
	call	I_GDTA		;Set all segments and GDT addresses.
	call	I_Hdls		;Set all "Handles" table pointers.
	mov	dx,(UMBMs-@)	;Point to UMBPCI error message.
	mov	ax,@HTEnd1	;Get driver size in pages.
	shr	ax,4
	add	UMBTbl.lw,ax	;Increment 1st upper-memory segment.
	sub	UMBTbl.hw,ax	;1st block too tiny for this driver?
	jbe	I_Err		;Yes?  Display error message and exit!
	add	@ReqLmt,2	;Accept requests for upper-memory.
	xor	ax,ax		;Point ES-reg. to low-memory.
	mov	es,ax
	mov	al,es:[HDISKS]	;Get BIOS hard-disk count.
	cmp	al,MAXBIOS	;More than our maximum hard-disks?
	jbe s	I_HdwF		;No, check for any diskettes.
	mov	al,MAXBIOS	;Set hard-disk count to our maximum!
I_HdwF:	mov	cl,es:[HDWRFL]	;Get BIOS "hardware installed" flag.
	test	cl,001h		;Any diskettes on this system?
	jz s	I_SetC		;No, set our drives counter.
	add	al,040h		;Bump BIOS diskette count.
	and	cl,0C0h		;More than one diskette to use?
	jz s	I_SetC		;No, set our drives counter.
	add	al,040h		;Bump BIOS diskette count.
I_SetC:	mov	DriveCt,al	;Save diskette and hard-disk counts.
	and	al,03Fh		;Any hard disks for us to use?
	jz s	I_WSCh		;No, go check for "workspace" request.
	mov	HDCount,al	;Set our hard-disc counter.
I_Next:	mov	ah,041h		;Get EDD "extensions" for this disk.
	mov	bx,055AAh
	call	I_In13
	jc s	I_NoEx		;If none, display msg. & ignore disk.
	cmp	bx,0AA55h	;Did BIOS "reverse" our entry code?
	jne s	I_NoEx		;No, display message and ignore disk.
	test	cl,001h		;Can this disk do LBA reads and writes?
	jnz s	I_CHS		;Yes, inquire about its CHS parameters.
I_NoEx:	mov	dx,(EDDMsg-@)	;Error!  Display "No EDD BIOS" message.
	call	I_Msg
	dec	DriveCt		;Decrement our drives count.
	mov	cx,00080h	;Flag this disk as "disabled".
	jmp s	I_SetP
I_CHS:	mov	ah,008h		;Get BIOS CHS data for this unit.
	call	I_In13
	jc s	I_CHSE		;If BIOS error, zero sectors/head.
	and	cl,03Fh		;Get sectors/head value (low 6 bits).
	jz s	I_CHSE		;If zero, ERROR!  Display message.
	mov	ch,dh		;Get heads/cylinder (BIOS value + 1).
	inc	ch
	jnz s	I_SetP		;If non-zero, save unit's CHS values.
I_CHSE:	mov	dx,(CHSMsg-@)	;Error!  Display "CHS data" message.
	call	I_Msg
	xor	cx,cx		;"Null" this unit's CHS parameters.
I_SetP:	movzx	bx,HDUnit	;Save CHS parameters in our table.
	shl	bl,1
	mov	[bx+CHSTbl-@],cx
I_More:	inc	HDUnit		;Bump BIOS unit number.
	inc	EDUnit.lb
	inc	CHUnit.lb
	dec	HDCount		;More hard-disks to check?
	jnz s	I_Next		;Yes, loop back and do next one.
I_WSCh:	cmp	WSFlag,0	;Should catcher use DOS "workspace"?
	je s	I_NoWS		;No, set our own I-O catcher buffer.
	clc			;Get DOS "List of Lists" pointer.
	mov	ax,05200h
	call	I_DOS
	jc s	I_NoWS		;Failed?  Use our own I-O buffer!
	call	ValPtr		;Is "List of Lists" pointer valid?
	jc s	I_NoWS		;No?  Must use our own I-O buffer.
	les	bx,es:[bx+012h]	;Get disk "Buffer Info" table ptr.
	call	ValPtr		;"Buffer Info" table pointer valid?
	jc s	I_NoWS		;No, must use our own I-O buffer.
	add	ax,0000Dh	;Adjust to "workspace" pointer addr.
	mov	DOSPtr,eax	;Save "Buffer Info" table pointer.
	xchg	ax,bx		;Get "workspace" buffer pointer.
	les	bx,es:[bx]
	mov	WSSeg,es	;Save old "workspace" segment value.
	call	ValPtr		;Is "workspace" pointer valid?
	jnc s	I_SetW		;Yes, set I-O buffer pointer.
	mov	al,ResSeg.hb	;Get "temporary memory" segment.
	shl	eax,24
	jmp s	I_SetW		;Go set "temporary" buffer pointer.
I_NoWS:	db	0B8h		;No "workspace" -- disable above rtn.
	jmp s	$+(Next-@NoWSP)
	mov	@NoWSP.lw,ax
	mov	DvrLen,544	;Set 544-byte resident driver size.
	mov	ax,cs		;Get our own I-O buffer pointer.
	inc	ax
	inc	ax
	shl	eax,16
I_SetW:	mov	IOBuf,eax	;Set working I-O buffer pointer.
	xor	ax,ax		;Point to vectors in low memory.
	mov	es,ax
	test	DriveCt,0C0h	;Any diskettes on this system?
	jz s	I_HDsk		;No, see if we have any hard-disks.
	cmp	es:[100h].dwd,0	;Has Int 40h vector been set?
	je s	I_No40		;No, use Int 13h for diskettes.
	mov	ax,03540h	;Get and save current Int 40h vector.
	call	I_DOS		;(Used during diskette I-O requests).
	mov	ax,02540h	;Set up for diskette Int 40h use.
	jmp s	I_Dskt		;Go "hook" diskette I-O.
I_No40:	mov	ax,03513h	;Get current Int 13h vector.
	call	I_DOS
	mov	ax,02513h	;Set up for diskette Int 13h use.
I_Dskt:	mov	DsktIRQ,ax	;Save diskette set-IRQ number.
	push	es		;Save prior diskette vector.
	push	bx
	pop	@DsktV
I_HDsk:	test	DriveCt,03Fh	;Any hard-disks on this system?
	jz s	I_Relo		;No, go "relocate" driver logic.
	mov	ax,03513h	;Get and save hard disk vector.
	call	I_DOS
	push	es
	push	bx
	pop	@HDiskV
I_Relo:	cld			;Move this driver to upper-memory.
	mov	cx,@HTEnd1
	xor	si,si
	xor	di,di
	mov	es,UMSeg
	rep	movsb
	cmp	@DsktV,0	;Was a diskette vector saved above?
	je s	I_HDV		;No, see about hard-disks.
	mov	ax,DsktIRQ	;"Hook" diskette I-O to this driver.
	mov	dx,(DsktIO-@)
	mov	ds,UMSeg
	call	I_DOS
I_HDV:	cmp	@HDiskV,0	;Was a hard disk vector saved above?
	je s	I_UEnd		;No, go "hook" driver into Int 2Fh.
	mov	ax,02513h	;"Hook" hard disk I-O to this driver.
	mov	dx,(DiskIO-@)
	mov	ds,UMSeg
	call	I_DOS
I_UEnd:	jmp s	I_Hk2F		;Go "hook" this driver into Int 2Fh.
;
; Initialization logic for XMGR if it loads "normally".
;
I_Norm:	mov	ax,cs		;Set all segments and GDT addresses.
	call	I_GDTA
	mov	ax,(ReqUMB-@)	;Put "Handles" table at UMB logic.
	call	I_Dsm1		;Dismiss UMBPCI & "Catcher" logic.
	jmp s	I_Hk2F		;Go "hook" driver into Int 2Fh.
;
; Initialization logic for XMGR if it loads in "boot" mode. 
;
I_Boot:	mov	[DvrNam+4].dwd,"$B2"   ;Set "boot" driver name.
	mov	ax,cs		       ;Set segments & GDT addresses.
	mov	MSHTPtr.hw,ax
	mov	@I2FSeg,ax
	mov	PrevI15.hw,ax
	mov	ax,ResSeg
	call	I_GDTS
	call	I_Dism		;Dismiss "Catcher" & UMBPCI logic.
	mov	es,ResSeg	;Move this driver to RESSEG:0000.
	mov	si,(HMAUsed-@)
	mov	di,si
	mov	cx,(MList-@)
	sub	cx,si
	shr	cx,1
	rep	movsw
	adc	cx,cx
	rep	movsb
	xor	eax,eax		;Set up to clear "Handles" table.
	mov	cx,MSHTCt
	les	di,MSHTPtr
	mov	al,001h		;Flag 1st "Handle" as free memory.
	jmp s	I_Clr2
I_Clr1:	mov	al,004h		;Flag next "Handle" totally unused.
I_Clr2:	stosw
	xor	ax,ax		;Clear remaining "Handle" values.
	stosd
	stosd
	loop	I_Clr1		;If more "Handles" left, loop back.
I_Hk2F:	mov	ax,0252Fh	;"Hook" this driver into Int 2Fh.
	mov	dx,(I2FEnt-@)
	mov	ds,@I2FSeg
	call	I_DOS
	jmp	I_Done		;Go display our "Handles" count.
;
; Initialization logic for XMGR if its "boot" has already loaded.
;
I_QHMB:	pushf			;"Boot" setup -- save CPU flags.
	cli			;Disable CPU interrupts.
	cld			;Ensure FORWARD "string" commands!
	push	fs		;Save FS-reg. (3rd 80386+ segment).
	mov	MSHTPtr.hw,es	;Set "stub" segment pointers.
	mov	@I2FSeg,es
	mov	ax,es:XMSRtnP.hw    ;Save "boot" resident segment.
	mov	ResSeg.lw,ax
	mov	fs,MSHTPtr.hw	    ;Reload "stub" segment in FS-reg.
	mov	es,ResSeg.lw	    ;Point to resident "boot" segment.
	mov	eax,es:HMAUsed.dwd  ;Copy over all "boot" variables.
	mov	HMAUsed.dwd,eax
	mov	ax,es:LECount	;Copy "A20" local-enable counter.
	mov	LECount,ax
	mov	al,es:DvrSw	;Copy driver switches and port flag.
	mov	DvrSw,al
	mov	ax,es:MSHTCt	;Copy "Handles" table count.
	mov	MSHTCt,ax
	mov	eax,es:HKAddr	;Copy "highest known address".
	mov	HKAddr,eax
	mov	ax,cs		;Set segments and GDT addresses.
	call	I_GDTS
	call	I_Dism		;Dismiss "Catcher" and UMBPCI logic.
	test	DvrSw,080h	;Was "boot" using port A20 logic?
	jz s	I_QI15		;No, see if Int 15h "hook" was done.
	mov	cx,P92LEN	;Overlay standard "A20" routine
	mov	si,(P92Hdl-@)	;  with Port 092h "A20" routine.
	mov     di,(A20Hdl-@)
	push	cs
	pop	es
	rep	movsb
I_QI15:	mov	ax,(ReqUMB+(MList+20-HTbl)-@)  ;Retain "scan" logic.
	mov	si,(StubEnd-@)	;Get "Handles" table pointer.
	cmp	fs:[si].lw,1	;Int 15h "hook" already done?
	jne s	I_QSDn		;Yes, disable scan-memory logic.
	cmp	fs:[si+2].dwd,0
	jne s	I_QSDn
	cmp	fs:[si+6].dwd,0
	je s	I_QLok		;If not done, set our declared length.
I_QSDn:	mov	ax,(XCCall-@)	;Disable "Scan Memory" dispatch.
	mov	Dspatch,ax
	mov	ax,(ReqUMB-@)	;Exclude UMB and "Scan Memory" logic.
I_QLok:	mov	DvrLen,ax	;Set declared length of this driver.
	xor	ax,ax		;Point ES-reg. to low memory.
	mov	es,ax
	cli			      ;Disable interrupts to test VDS.
	test	es:[VDSFLAG].lb,020h  ;"VDS services" (EMM386) active?
	jz s	I_QEnt		      ;No, continue "boot" setup.
	mov	ax,@HTEnd1	;Set "stub" length and segment.
	mov	VDSLen.lw,ax
	mov	ax,MSHTPtr.hw
	mov	VDSSeg.lw,ax
	call	I_VDS		;Lock driver "stub" in memory forever.
	jc s	I_QHME		;VDS error?  Display message and exit!
	mov	ax,DvrLen	;Set main driver length and segment.
	mov	VDSLen.lw,ax
	mov	VDSSeg.lw,cs
	call	I_VDS		;Lock main driver in memory forever.
	jnc s	I_QEnt		;If no error, continue "boot" setup.
I_QHME:	pop	fs		;BAAAD News!  Reload our registers.
	popf
	sti			;Re-enable CPU interrupts.
I_HMAE:	mov	dx,(VDSMsg-@)	;Point to "VDS init error" message.
	jmp	I_Err		;Go display error message and exit.
I_QEnt:	sti			;Re-enable CPU interrupts.
	les	di,fs:IntRtnP	;Get "stub" interrupt entry pointer.
	mov	eax,es:[di-4]	;Save previous interrupt vectors.
	mov	PrevI15,eax
	mov	eax,es:[di+(PrevI2F-IntRtn)]
	mov	PrevI2F,eax
	mov	ax,cs		;Point "stub" entry segments to us.
	mov	fs:XMSRtnP.hw,ax
	mov	fs:IntRtnP.hw,ax
	xor	ax,ax		;Clear "boot" driver from memory.
	mov	cx,(MList+20-@)
	xor	di,di
	mov	es,ResSeg.lw
	shr	cx,1
	rep	stosw
	pop	fs		;Reload FS-reg. and CPU flags.
	popf
;
; Driver Initialization "Final Routine".
;
I_Done:	mov	ax,DvrLen	;Get declared driver length.
	lds	bx,Packet	;Post results in "init" packet.
	mov	[bx].RPLen,ax
	mov	[bx].RPStat,RPDON
I_Fail:	popad			;Reload all 32-bit CPU registers.
I_Exit:	pop	dx		;Reload 16-bit CPU regs. we used.
	pop	bx
	pop	ax
	pop	es		;Reload CPU segment registers.
	pop	ds
	popf			;Reload CPU flags and exit.
	retf
;
; Subroutine to set all driver segments and GDT addresses.   Entry
;   at I_GDTA is for normal and UMBPCI loading.    Entry at I_GDTS
;   is for "boot" and "after boot" loading.    
;
I_GDTA:	mov	MSHTPtr.hw,ax	;Set "Handles" table segment.
	mov	@I2FSeg,ax	;Set Int 2Fh "entry" segment.
	mov	PrevI15.hw,ax	;Set Int 15h "hook" segment.
I_GDTS:	mov	XMSRtnP.hw,ax	;Set all "resident" segments.
	mov	IntRtnP.hw,ax
	mov	cx,16		;Set 32-bit descriptor base.
	mul	cx
	mov	[GDT_CS+2].lw,ax
	mov	[GDT_CS+4].lb,dl
	add	ax,(OurGDT-@)	;Set our 32-bit GDT base address.
	adc	dx,0
	mov	GDTPAdr.lw,ax
	mov	GDTPAdr.hw,dx
	ret			;Exit.
;
; Subroutine to dismiss all "I-O Catcher" and UMBPCI logic.
;
I_Dism:	mov	ax,(StubEnd-@)	  ;Put "Handles" table with "Stub".
I_Dsm1:	mov	MSHTPtr.lw,ax	  ;Set starting "Handles" pointer.
	mov	ax,(HTbl-ReqUMB)  ;Adjust needed address offsets.
	sub	Dspatch,ax
	sub	@ScanA1,ax
	sub	@ScanA2,ax
	add	[ScnM15+1].lw,ax
	add	[ScnM20+1].lw,ax
	call	I_Hdls		;Set all "Handles" table pointers.
	mov	ax,@HTEnd1	;Adjust resident driver length.
	mov	DvrLen,ax
	mov	cx,(MList-HTbl)	;Move up memory-scanning logic.
	mov	si,(HTbl-@)
	mov	di,(ReqUMB-@)
	push	cs
	pop	es
	rep	movsb
	ret			;Exit.
;
; Subroutine to set all "Handles" table values.
;
I_Hdls:	db	0B8h		;Get "stc"/"nop" for disables.
	stc
	nop
	test	DvrSw.lb,001h	;"E820h" memory tests desired?
	jnz s	I_Hdl1		;Yes, see about "E801h" tests.
	mov	@NoE820,ax	;Disable "E820" memory logic.
I_Hdl1:	test	DvrSw.lb,002h	;"E801h" memory logic desired?
	jnz s	I_Hdl2		;Yes, go set "Handles" bases.
	mov	@NoE801,ax	;Disable "E801h" memory logic.
I_Hdl2:	mov	ax,MSHTPtr.lw	;Set all "Handles" table bases.
	sub	ax,HDLSIZE
	mov	@HTbl1,ax
	mov	@HTbl3,ax
	add	ax,HDLSIZE
	mov	@HTbl2,ax
	mov	@HTbl4,ax
	mov	@HTbl5,ax
	mov	@HTbl6,ax
	mov	@HTbl8,ax
	mov	@HTbl9,ax
	add	ax,(7*HDLSIZE)
	mov	@HTbl7,ax
	mov	ax,MSHTCt	;Set all "Handles" table counts.
	mov	@HdlCt1,al
	mov	@HdlCt2,ax
	mov	@HdlCt3,ax
	mov	@HdlCt4,ax
	mov	ah,HDLSIZE	;Set "Handles" table limits.
	mul	ah
	add	ax,MSHTPtr.lw
	mov	@HTEnd1,ax
	mov	@HTEnd2,ax
	mov	@HTEnd3,ax
	ret			;Exit.
;
; Subroutine to validate a 32-bit pointer in the ES:DX-registers.
;
ValPtr:	mov	ax,es		;Get 32-bit pointer in EAX-reg.
	shl	eax,16
	mov	ax,bx
	inc	eax		;Is this pointer all-ones?
	jz s	ValPtE		;Yes, set carry and exit.
	dec	eax		;Is this pointer all-zeros?
	jz s	ValPtE		;Yes, set carry and exit.
	clc			;Valid pointer -- clear carry.
	ret			;Exit
ValPtE:	stc			;Invalid pointer -- set carry.
	ret			;Exit.
;
; Subroutine to test if the "A20" line is on.
;
I_T20:	mov	ax,0FFFFh	;Set DS:SI-regs. to FFFF:0090h.
	mov	ds,ax
	mov	si,00090h
	inc	ax		;Set ES:DI-regs. to 0000:0080h.
	mov	es,ax
	mov	di,00080h
	cld			;Compare 4 words at FFFF:0090h
	mov	cx,4		;   v.s. 4 words at 0000:0080h.
	repe	cmpsw
	je s	I_T20X		;Are the two areas the same?
	inc	ax		;No, "A20" is on -- post "success".
I_T20X:	ret			;Exit.
;
; Subroutines to issue initialization "external" calls.
;
I_VDS:	mov	ax,08103h	;VDS "lock" -- set parameters.
	mov	dx,0000Ch
	mov	di,offset VDSLen
	push	cs
	pop	es
	push	fs		;Issue VDS "lock" request.
	int	04Bh
	pop	fs
	jmp s	I_IntX		;Go restore critical settings & exit.
I_In13:	mov	dl,HDUnit	;Get disk BIOS unit number.
	clc			;Issue BIOS data interrupt.
	int	013h
	jmp s	I_IntX		;Restore driver settings, then exit.
I_Msg:	mov	ah,009h		;Get DOS "display string" command.
I_DOS:	int	021h		;Execute desired DOS request.
	jmp s	I_IntX		;Go restore critical settings & exit.
I_XMS:	int	02Fh		;Issue XMS interrupt.
I_IntX:	sti			;RESTORE all critical driver settings!
	cld			;(Never-NEVER "trust" external code!).
	push	cs
	pop	ds
	ret			;Exit.
CODE	ends
	end
