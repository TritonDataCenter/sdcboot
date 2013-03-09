	page	59,132
	title	RDISK -- 2-Gigabyte "RAM Disk" Driver.
;
; This is a RAM-disk driver for V4.0 MS-DOS systems and up.   RDISK
; uses XMS memory and provide 2-MB to 2-GIGABYTES of storage, using
; its /Snnnn switch.   It uses a FAT-16 file structure (V4.0 MS-DOS
; or newer), and it uses V2.0 XMS logic for under 64-MB or V3.0 XMS
; logic for 64-MB RAM disks or greater.   If loaded as a .SYS file,
; RDISK will use the first-available DOS drive letter to access its
; files.   If loaded as a .COM file, the  /:  switch may be used to
; specify a desired drive letter.    RDISK is a simple "load it and
; forget-about-it!" driver without resizing or other complex items.
;
; General Program Equations.
;
	.386p			;Allow use of 80386 instructions.
s	equ	<short>		;Set a conditional jump to "short".
SYSSTAK	equ	356		;Driver local-stack size.
COMSTAK	equ	348		;Driver local-stack size.
CR	equ	00Dh		;ASCII carriage-return.
LF	equ	00Ah		;ASCII line-feed.
;
; Driver Return Codes.
;
SUCCESS	equ	00100h		;Request executed successfully.
BUSY	equ	00200h		;"Busy" flag for removable checks.
CMDERR	equ	08103h		;Invalid command error.
SNFERR	equ	08108h		;Sector not found error.
GENERR	equ	0810Ch		;"General" error.
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
; DOS "Current Directory Structure" Layout.
;
CDS	struc
CDSPath	db	67 dup (?)	;Current drive "path".
	db	?		;Lower flag bits (not used by us).
CDSFlag	db	?		;Upper flag bits ("in-use", etc.).
CDSDPB	dd	?		;Drive Parameter Block pointer.
	db	15 dup (?)	;(Initialized but unused by us).
CDS	ends
CDSSize	equ	88		;Length of each CDS array entry.
;
; DOS "Request" Packet Layout.
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
RPCL	dd	?		;Command-line and BPB pointer.
RPDN	db	?		;Drive number (0 = A: etc.).
RP	ends
;
; DOS "Read" or "Write" Packet Layout.
;
RWP	struc
	db	14 dup (?)	;Header thru unit, same as above.
RWBuf	dd	?		;Read and write buffer address.
RWSCt	dw	?		;Read and write sector count.
RWSec	dw	?		;Read and write starting sector.
	dd	?		;(Unused).
RWSecL	dd	?		;Read/Write 32-bit starting sector,
				;  use when 16-bit sector is 0FFFFh
				;  and attribute bit 1 (below) set.
RWP	ends
;
; Segment Declarations.
;
CODE	segment	public use16 'CODE'
	assume	cs:CODE,ds:CODE
@	equ	$		;.SYS driver relocation base.
B	equ	$-256		;.COM driver relocation base.
;
; DOS Driver Device Header.
;
DvrHdr:	jmp	I_COM		;Go to .COM file init logic (MUST be
COMFlag	db	0FFh		;  reset to 0FFFFFFFFh before exit!).
	dw	00802h		;Device attributes:  "Block" device
				;  handling "Open/Close" and 32-bit
				;  sector nos.  NO useless "extras"!
	dw	(Strat-@)	;"Strategy" routine pointer.
DevIntP	dw	(I_DevI-@)	;"Device Interrupt" routine pointer.
	db	1,'RDISK  '	;Driver name.
	dw	0		;(Unused alignment "filler").
;
; XMS Move Parameter Block.
;
XMSBlk	dd	256		;XMS move length   (256 for Init).
XMSSH	dw	0		;XMS source "handle" (0 for Init).
	dd	(Buffer-@)	;XMS source address (set by Init).
XMSDH	dw	0		;XMS destination "handle".
	dd	0		;XMS destination address.

XMSWAP	equ	(XMSSH-@) XOR (XMSDH-@)	;(I-O "swap operands" value).
;
; General Driver Variables.
;
CStack	dd	0		;Caller's saved stack pointer.
RqPkt	dd	0		;DOS request-packet pointer.
MaxSec	dd	0		;Maximum I-O sector number.
XEntry	dd	0		;XMS manager "entry" pointer.
XMHdl	dw	0		;"Handle" for our XMS memory.
	dw	0		;(Unused alignment "filler").
;
; Resident "BIOS Parameter Block" (BPB).
;
; NOTE:  As requested, RDISK now supports up to 4-GB of XMS, but ONLY
;	 when driver word-address 0038h ("ResBPB" below) is "patched"
;	 to the value 00FFFh (4095-MB maximum).   NOT ALL versions of
;	 DOS are designed for 64K clusters used by 4-GB FAT-16 disks!
;	 V6.22 MS-DOS "CHKDSK" will give a Divide Overflow, and other
;	 DOS systems/programs may also FAIL!   RDISK will permit 4-GB
;	 only with this user "patch" and only "at your own risk"!!
;
ResBPB	dw	2047		;Bytes per sector,   512 after Init.
	db	0		;Sectors per cluster,	set by Init.
	dw	1		;Reserved sector count,	always 1.
	db	1		;Number of FAT copies,	always 1.
	dw	512		;RootDirectory entries,	always 512.
	dw	0		;FAT-12 total sectors,	0 if FAT-16.
	db	0F8h		;Media-descriptor byte, always 0F8h.
	dw	0		;Sectors per FAT table,	set by Init.
	dw	63		;Sectors per track,	always 63.
	dw	255		;Number of heads,	always 255.
	dd	0		;"Hidden" sector count,	always 0.
	dd	25		;FAT-16 total sectors,	0 if FAT-12.
;
; DOS "Device Interrupt" Routine.   This handles all driver requests.
;
DevInt:	pushf			;Save CPU flags & disable interrupts.
	cli
	mov	cs:CStack.lw,sp	;Save caller's stack pointer.
	mov	cs:CStack.hw,ss
	push	cs		     ;Switch to driver's stack (avoid
	pop	ss		     ;  CONFIG "STACKS=0,0" trouble).
	mov	sp,offset (ComEnd-@) ;(Changed to "SysEnd+SYSSTAK" if
@Stack	equ	[$-2].lw	     ;  we are loaded as a .SYS file).
	sti			;Re-enable CPU interrupts.
	push	eax		;Save all needed CPU registers.
	push	bx
	push	si
	push	ds
	lds	si,cs:RqPkt	;Get DOS request-packet address.
	mov	al,[si].RPOp	;Get request command code.
	mov	bx,(XMSSH-@)	;Set read XMS-block source index.
	cmp	al,4		;Is this a read request?
	je s	RqIO		;Yes, go do it.
	mov	bl,(XMSDH-@)	;Set write XMS-block source index.
	cmp	al,8		;Is this a write request?
	je s	RqIO		;Yes, go do it.
	cmp	al,1		;Is this a media-check request?
	je s	RqMCh		;Yes, go do it.
	cmp	al,2		;Is this a build-BPB request?
	je s	RqBPB		;Yes, go do it.
	cmp	al,9		;Is this a write-verify request?
	je s	RqIO		;Yes, go do it.
	cmp	al,13		;Is request code less than "Open?
	jb s	RqCmdE		;Yes, return "invalid command" code.
	sub	al,15		;Is this an "Open" or "Close" request?
	jb s	RqDone		;Yes, ignore -- post "success" & exit.
	mov	ax,BUSY+SUCCESS	;Preset "busy" flag for removable req.
	jz s	RqExit		;If removable request, go post "busy".
RqCmdE:	mov	ax,CMDERR	;Invalid!  Get "invalid command" code.
	jmp s	RqExit		;Go post "invalid command" and exit.
;
; DOS Request Code 1 -- Media Check (has "removable" disk changed?).
;
RqMCh:	mov	[si].RPLen.lb,al  ;NO change -- we are a "fixed" disk!
	jmp s	RqDone		  ;Go post "success" and exit.
;
; DOS Request Code 2 -- Build "BIOS Parameter Block" (BPB).
;
RqBPB:	push	cs		;Stack pointer to our resident BPB.
	push	(ResBPB-@)
	pop	[si].RPCL	;Set BPB pointer in request packet.
	jmp s	RqDone		;Go post "success" and exit.
;
; DOS Request Code 4, 8 and 9 -- Read, Write and Write-Verify.
;
RqIO:	movzx	eax,[si].RWSCt	;Set I-O sectors in XMS block.
	mov	cs:XMSBlk,eax
	movzx	eax,[si].RWSec	;Get 16-bit starting sector no.
	cmp	ax,-1		;Should we use the 32-bit number?
	jne s	RqSetS		;No, set 16-bit starting sector.
	mov	eax,[si].RWSecL	;Get 32-bit starting sector no.
RqSetS:	mov	cs:[bx+2],eax	;Set starting sector in XMS block.
	add	eax,cs:XMSBlk	;Calculate ending sector number.
	cmp	eax,cs:MaxSec	;Is ending sector too large?
	mov	ax,SNFERR	;  (Preset "sector not found" code).
	ja s	RqErr		;Yes?  Return "sector not found"!
	mov	eax,[si].RWBuf	;Get user-buffer offset/segment addr.
	push	cs		;Point to our XMS parameter block.
	pop	ds
	mov	si,(XMSBlk-@)
	shl	[si].dwd,9		;Shift sectors to byte count.
	push	[si+(XMHdl-XMSBlk)].lw	;Set our XMS memory "handle".
	pop	[bx].lw
	shl	[bx+2].dwd,9	;Set XMS offset (start sector * 512).
	xor	bl,XMSWAP	;"Swap" user and XMS-memory indexes.
	and	[bx].lw,0	;Set user-buffer "handle" of zero.
	mov	[bx+2],eax	;Set user-buffer offset/segment addr.
	mov	ah,00Bh		;Call XMS manager to move our data.
	call	[si+(XEntry-XMSBlk)].dwd
	lds	si,cs:RqPkt	;Reload DOS request-packet address.
	dec	ax		;Any XMS move errors?
	jnz s	RqXErr		;Yes?  Return "general error" code.
;
; Request exit routine.
;
RqDone:	mov	ax,SUCCESS	;Done!  Get "success" return code.
RqExit:	mov	[si].RPStat,ax	;Set return code in DOS request pkt.
	pop	ds		;Reload all CPU registers we used.
	pop	si
	pop	bx
	pop	eax
	lss	sp,cs:CStack	;Switch back to caller's stack.
	popf			;Reload CPU flags and exit.
	retf
RqXErr:	mov	ax,GENERR	;XMS error!  Get "general error" code.
RqErr:	and	[si].RWSCt,0	;Tell DOS "no sectors transferred".
	jmp s	RqExit		;Go post error return code and exit.
;
; DOS "Strategy" Routine.   This must be LAST in the run-time logic,
;   to prevent setting the @Stack stack-pointer (above) from causing
;   a CPU cache "code modification" ERROR!
;
Strat:	mov	cs:RqPkt.lw,bx	;Save DOS request-packet addr.
	mov	cs:RqPkt.hw,es
	retf			;Exit & await Device Interrupt.
SysEnd	equ	$		;(End of resident .SYS driver logic).
;
; Resident "Drive Parameter Block" (DPB), only for .COM driver loads.
;
ResDPB	db	0FFh		;Drive number (A: = 0), set by Init.
	db	0		;Unit number,           always 0.
	dw	512		;Bytes per sector,      always 512.
	db	0		;Sectors/cluster - 1,   set by Init.
	db	0		;Cluster=>sector shift, set by Init.
	dw	1		;Reserved sector count, always 1.
	db	1		;Number of FAT copies,  always 1.
	dw	512		;RootDirectory entries, always 512.
	dw	0		;1st data-sector no.,   set by Init.
	dw	0		;Number of clusters +1, set by Init.
	dw	0		;Sectors per FAT table, set by Init.
	dw	0		;1st directory sector,  Set by Init.
DPBHdrP	dd	0FFF40000h	;Driver header pointer, set by Init.
	db	0F8h		;Media descriptor byte, always 0F8h. 
	db	0		;"Drive accessed" flag, initially 0.
	dd	00000FFFFh	;Pointer to next DPB,   set by others.
	dw	0		;1st "free" cluster,    initially 0.
	dw	0FFFFh		;Free cluster count,    initially -1.
	db	7 dup (0)	;(Unused alignment "filler").
ComEnd	equ	$+COMSTAK	;(End of resident .COM driver).
Buffer	equ	$		;(Start of XMS initialization buffer).
;
; Initialization Messages, Part 1.
;
TTLMsg	db	CR,LF,'RDISK, 10-16-2011.  '
TTLSz	db	'  25-MB Memory.',CR,LF,'$'
NDMsg	db	'No V4.0+ DOS$'
PRMsg	db	'No 80386+ CPU$'
NXMsg	db	'No XMS manager$'
;
; Common Driver Initialization Routine.
;
I_Init:	mov	ah,030h		;Get our DOS version number.
	call	I_In21		;(Sets driver DS-reg. upon exit).
	mov	dx,(NDMsg-@)	;Point to "No V4.0+ DOS" message.
	cmp	al,004h		;Is this system V4.0 MS-DOS or newer?
	jb s	I_Quit		;No?  Display error message and exit!
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
I_Junk:	mov	dx,(PRMsg-@)	;Point to "No 80386+ CPU" message.
I_Quit:	call	I_Msg		;CANNOT use this "old" system --
	mov	dx,offset Suffx	;  display error message & suffix!
	call	I_Msg
	stc			;Set carry (Init error) and exit!
	retf
I_Sv32:	pushad			;Save all 32-bit CPU regs.
I_NxtC:	mov	ax,es:[si]	;Get next command-line byte.
	inc	si		;Bump pointer past first byte.
	cmp	al,0		;Is byte the command-line terminator?
	je s	I_TTLJ		;Yes, go display driver "title" msg.
	cmp	al,LF		;Is byte an ASCII line-feed?
	je s	I_TTLJ		;Yes, go display driver "title" msg.
	cmp	al,CR		;Is byte an ASCII carriage-return?
I_TTLJ:	je	I_TTL		;Yes, go display driver "title" msg.
	cmp	al,'-'		;Is byte a dash?
	je s	I_NxtS		;Yes, see what its "switch" byte is.
	cmp	al,'/'		;Is byte a slash?
	jne s	I_NxtC		;No, check next command-line byte.
I_NxtS:	mov	al,ah		;Get next command-line byte.
	sar	COMFlag,1	;Were we loaded as a .COM file?
	jz s	I_ChkS		;No, see if byte is "S" or "s".
	cmp	al,':'		;Is this byte a colon?
	jne s	I_ChkS		;No, see if byte is "S" or "s".
	inc	si		;Bump scan pointer past colon.
	mov	ax,es:[si]	;Get next command-line byte.
	and	al,0DFh		;Mask out byte's lower-case bit.
	cmp	al,'Z'		;Is byte above a "Z"?
	ja s	I_NxtC		;Yes, continue scan for a terminator. 	
	sub	al,'A'		;Is byte below an "A"?
	jb s	I_NxtC		;Yes, continue scan for a terminator.
	mov	ResDPB,al	;Save user's desired drive number.
	inc	si		;Bump scan pointer past drive letter.
I_ChkS:	and	al,0DFh		;Mask out byte's lower-case bit.
	cmp	al,'S'		;Is this byte an "S" or "s"?
	jne s	I_NxtC		;No, continue scan for a terminator.
	inc	si		;Bump scan ptr. past "S" or "s".
	and	DSize16.lw,0	;Reset XMS size and title bytes.
	mov	TTLSz.dwd,"    "
I_Siz0:	mov	al,es:[si]	;Get next command-line byte.
	cmp	al,'9'		;Is byte greater than a '9'?
	ja s	I_Siz1		;Yes, go see what is in bucket.
	cmp	al,'0'		;Is byte less than a '0'?
	jb s	I_Siz1		;Yes, go see what is in bucket.
	cmp	TTLSz.lb,' '	;Have we already found 4 digits?
	jne s	I_Siz2		;Yes, reset default 25-MB size.
	inc	si		;Bump scan pointer past digit.
	push	[TTLSz+1].dwd	;Shift title size 1 place left.
	pop	TTLSz.dwd
	mov	TTLSz+3,al	;Insert next "title" message byte.
	mov	cx,DSize16.lw	;Multiply current blocksize by 10.
	shl	cx,2
	add	cx,DSize16.lw
	shl	cx,1
	and	ax,0000Fh	;"Add in" next memory-size digit.
	add	cx,ax
	mov	DSize16.lw,cx	;Update desired user memory size.
	jmp s	I_Siz0		;Go scan for more memory-size digits.
I_Siz1:	mov	ax,DSize16.lw	;Get user's desired memory size.
	cmp	ax,ResBPB	;Does user want too much memory?
				;  (See note at "ResBPB" above!).
	ja s	I_Siz2		;Yes, reset default 25-MB size.
	cmp	ax,2		;Does user want less than 2-MB?
	jae s	I_Siz3		;No, continue scan for terminator.
I_Siz2:	mov	DSize16.lw,25	;Invalid!  Reset to default 25-MB.
	mov	TTLSz.dwd,"52  "
I_Siz3:	jmp	I_NxtC		;Continue scan for a terminator.
I_TTL:	mov	dx,(TTLMsg-@)	;Display driver "title" message.
	call	I_Msg
	mov	ax,(DevInt-@)	;Set run-time Device Interrupt ptr.
	mov	DevIntP,ax
	mov	ax,04300h	;Inquire if we have an XMS manager.
	call	I_In2F
	mov	dx,(NXMsg-@)	;Point to "No XMS manager" message.
	cmp	al,080h		;Is an XMS manager installed?
	jne	I_Err		;No?  Display error message and exit!
	mov	ax,04310h	;Get XMS manager "entry" address.
	call	I_In2F
	push	es		;Save XMS manager "entry" addresses.
	push	bx
	pop	XEntry
	mov	esi,DSize16.dwd	;Get user's XMS memory size.
	shl	esi,11		;Convert from MB to sectors.
	xor	ecx,ecx		;Initialize cluster size to 1.
	inc	cx
	mov	edx,65536	;Get sectors used by cluster size 2.
I_CTst:	cmp	esi,edx		;Using this many sectors or more?
	jb s	I_FAT0		;No, use previous cluster size.
	shl	edx,1		;Shift values for next cluster size.
	shl	cl,1
	jmp s	I_CTst		;Loop back and check next size.
I_FAT0:	mov	CluSecs,cl	;Set our cluster size in BPB.
	xor	edx,edx		;Divide number of sectors by cluster
	mov	eax,esi		;  size to get FAT-table entries.
	div	ecx
	add	eax,255		;"Round" entries to a whole sector.
	shr	eax,8		;Get 32-bit FAT sector count.
	add	ax,8		;Add 8 for overhead & EOF entries.
	mov	FATSecs,ax	;Set number of FAT sectors in BPB.
	add	ax,33		;Add 1 "Boot" + 32 Root Directory.
	add	eax,esi		;Set final total of drive sectors.
	mov	MaxSec,eax
	cmp	eax,65535	;More than 65K total sectors?
	ja s	I_FAT1		;Yes, set FAT-16 sector count.
	mov	DSize12,ax	;Set total FAT-12 sector count.
	and	DSize16.dwd,0	;Reset unused FAT-16 sector count.
	jmp s	I_FAT2		;Go get required XMS memory.
I_FAT1:	mov	DSize16.dwd,eax	;Update total FAT-16 sector count.
I_FAT2:	xchg	eax,edx		;Get total sectors in EDX-reg.
	shr	edx,1		;Convert back to KB for XMS allocate.
	adc	edx,0		;If "odd" sector, get one extra KB.
	mov	ah,009h		;Get V2.0 XMS "allocate memory" code.
	cmp	edx,(60*1024)	;Will we be requesting over 60-MB?
	jbe s	I_GetX		;No, do V2.0 XMS "allocate memory".
	mov	ah,089h		;Use V3.0 XMS "allocate memory" code.
I_GetX:	call	I_XMS		;Request all necessary XMS memory.
	jnz s	I_XmsE		;Error?  Display message and quit!
	mov	XMHdl,dx	;Save XMS buffer "handle" number.
	mov	XMSDH,dx
	mov	ah,00Ch		;"Lock" our XMS memory.
	call	I_XMS
	jnz s	I_XmsE		      ;Error?  Display msg. & quit!
	mov	cx,offset BPBLen      ;Save all resident BPB data.
	mov	si,offset (BootBPB-@)
	mov	di,(ResBPB-@)
	push	cs
	pop	es
	rep	movsb
	mov	cx,(COMSTAK/2)	;Zero .COM stack and XMS buffer.   We
	mov	di,(Buffer-@)	;  zero our 1st 160K of XMS, to reset
	rep	stosw		;  BootBlock, FAT, and RootDirectory.
	mov	cx,(160*4)	;Get 256-byte block count in 160K.
	mov	si,(XMSBlk-@)	;Point to our XMS move block.
	mov	[si+8],cs	;Set move source-address segment.
I_ClrX:	push	cx		;Zero next 256-byte block of XMS.
	call	I_XMov
	pop	cx
	jnz s	I_XmsE		;Error?  Display message and quit!
        inc     [si+13].lw	;Bump XMS destination offset.
	loop	I_ClrX		      ;Loop if more blocks to clear.
	mov	[si].lw,offset BBLen  ;Set "Boot" Block XMS values.
	mov	[si+6].lw,offset (BBlock-@)
	mov	[si+13].lw,ax	      ;Set "sector 0" as XMS dest.
	call	I_XMov		      ;Move "Boot" Block up to XMS.
	jnz s	I_XmsE		      ;Error?  Display msg. and quit!
	mov	[si].lb,offset FATLen ;Set Initial FAT Table values.
	mov	[si+6].lw,offset (FATTbl-@)
	mov	[si+13].lb,2	      ;Set "sector 1" as XMS dest.
	call	I_XMov		      ;Move Initial FAT Table to XMS.
	jnz s	I_XmsE		      ;Error?  Display msg. and quit!
	mov	[si].lb,offset VLLen  ;Set Volume Label XMS values.
	mov	[si+6].lw,offset (VolLbl-@)
	mov	ax,FATSecs	      ;Calculate & set label sector.
	shl	ax,1		      ;(1st "root directory" entry).
	add	[si+13],ax
	call	I_XMov		;Move our Volume Label up to XMS.
I_XmsE:	jnz	I_XErr		;Error?  Display message and quit!
	sar	COMFlag,1	;Were we loaded as a .COM file?
	jz	I_Done		;No, go display drive we shall use.
	mov	ax,cs		;Set driver-header segment in DPB.
	add	DPBHdrP.hw,ax
	mov	ah,052h		;Get DOS "List of Lists" pointer.
	call	I_In21
	mov	ah,es:[bx+21h]	;Get maximum "block device" number.
	les	bx,es:[bx+16h]	;Get initial CDS pointer.
	mov	al,ResDPB	;Get user's desired drive number.
	add	DEMsg.lb,al	;Make it a letter in "unusable" msg.
	cmp	al,0FFh		;Does user want a specific drive?
	je s	I_CDS1		;No, go use next "free" CDS entry.
	cmp	al,ah		;Is user's drive number too high?
	jae s	I_CDSE		;Yes?  Display error message & exit!
	mov	ah,CDSSize	;Point to user's specific CDS entry.
	mul	ah
	add	bx,ax	
	test	es:[bx].CDSFlag,0C0h  ;Is this entry already in use?
	jz s	I_CDS5		      ;No, go set up user CDS entry.
I_CDSE:	mov	dx,offset DEMsg	      ;BAD News!  Get "unusable" ptr.
	jmp s	I_CDS3		      ;Go display error msg. & exit!
I_CDS1:	mov	al,0		      ;Use "next":  Reset CDS index.
I_CDS2:	test	es:[bx].CDSFlag,0C0h  ;Is this entry already in use?
	jz s	I_CDS4		      ;No, go set user drive number.
	add	bx,CDSSize	;Bump to next CDS entry in array.
	inc	ax		;Bump CDS index (i.e. user "drive").
	cmp	al,ah		;Have we checked all CDS entries?
	jb s	I_CDS2		;No, go see if this one is "free".
	mov	dx,offset NFMsg	;BAD News!  Get "no free drive" ptr.
I_CDS3:	jmp	I_Err		;Go display error message and exit!
I_CDS4:	mov	ResDPB,al	;Set user drive number in our DPB.
I_CDS5:	mov	di,bx		;Set up to store CDS data.
	mov	eax,"\:A"	;Get basic drive "path" data.
	add	al,ResDPB	;Add in user's drive number.
	mov	DMLtr.lb,al	;Set drive letter in display msg.
	stosd			  ;Set CDS entry "path" data.
	lea	di,[bx+CDSFlag-1] ;Skip to CDS flag word.
	mov	ax,04000h	  ;Set "physical" CDS flags.
	stosw
	mov	ax,(ResDPB-@)	;Set DPB offset in CDS entry.
	stosw
	xchg	ax,bp		;Save DPB offset for Int 53h below.
	mov	ax,DPBHdrP.hw	;Set DPB segment in CDS entry.
	stosw
	xor	ax,ax		;Reset current-directory entry.
	stosw
	dec	ax		;Set CDS entry "FFFFFFFFh" dword.
	stosw
	stosw
	mov	ax, 2		;Set CDS entry "root offset" to 2.
	stosw
	xor	ax,ax		;Zero CDS entry "type" & "IFSRedir".
	stosb
	stosw
	stosw
	mov	si,offset (BootBPB-@)  ;Point DS:SI to "boot" block.
	push	ds		       ;Point ES:BP to our DPB.
	pop	es
	mov	ah,053h		;Have DOS "translate" Boot Parameter
	call	I_In21		;  Block into Drive Parameter Block.
	mov	ah,052h		;Reload DOS "List of Lists" pointer.
	call	I_In21
	mov	al,ResDPB	;Get our assigned drive number.
	cmp	al,es:[bx+20h]	;Are we now the high "block device"?
	jb s	I_Link		;No, go link into DOS driver chain.
	inc	ax		;Update the highest "block device"
	mov	es:[bx+20h],al	;   number in DOS "List of Lists".
I_Link:	mov	eax,es:[bx+22h]	;Link to next driver after "NUL".
	mov	DvrHdr.dwd,eax
	mov	eax,DPBHdrP.dwd	;Link "NUL" driver to this driver.
	mov	es:[bx+22h],eax
	les	bx,es:[bx]	   ;Get initial DPB pointer.
I_DPB1:	cmp	es:[bx+19h].lw,-1  ;Last Drive Parameter Block?
	je s	I_DPB2		   ;Yes, go link our DPB to it.
	les	bx,es:[bx+19h]	;Point to next DPB in chain.
	jmp s	I_DPB1		;Go see if this is the last DPB.
I_DPB2:	push	DPBHdrP.hw	;Link current ending DPB to our DPB.
	push	(ResDPB-@)
	pop	es:[bx+19h].dwd
	mov	cx,(ComEnd-@)/2	;Move resident driver down into PSP.
	xor	si,si		;(Avoid using initial 64 PSP bytes).
	les	di,DPBHdrP
	rep	movsw
I_Done:	mov	dx,offset DvMsg	;Display which drive we shall use.
	call	I_Msg
	clc			;Clear carry flag (no Init errors).
I_Bye:	popad			;Reload all 32-bit CPU registers.
	retf			;Exit.
I_XErr:	mov	dx,offset XEMsg	;Point to "XMS init error" message.
I_Err:	push	dx		;Init ERROR!  Save message pointer.
	mov	dx,XMHdl	;Load our XMS memory "handle".
	or	dx,dx		;Did we reserve any XMS memory?
	jz s	I_LDMP		;No, reload pointer & display msg.
	mov	ah,00Dh		;Unlock and "free" our XMS memory.
	push	dx
	call	I_XMS
	mov	ah,00Ah
	pop	dx
	call	I_XMS
I_LDMP:	pop	dx		;Reload error message pointer.
	call	I_Msg		;Display error message and suffix.
	mov	dx,offset Suffx
	call	I_Msg
	stc			;Set carry flag (Init error!).
	jmp s	I_Bye		;Go reload 32-bit regs. and exit.
;
; .SYS Driver Initialization Routine.
;
I_DevI:	pushf			;Entry -- save CPU flags.
	push	ds		;Save CPU segment registers.
	push	es
	push	ax		;Save needed 16-bit CPU registers.
	push	bx
	push	dx
	push	cs		;Set this driver's DS-reg.
	pop	ds
	xor	ax,ax		;Zero "loaded as a .COM file" flag.
	mov	COMFlag,al
	les	bx,RqPkt	;Point to DOS "Init" packet.
	cmp	es:[bx].RPOp,al	;Is this really an "Init" packet?
	jne s	I_Exit		       ;No?  Reload regs. and exit!
	mov	es:[bx].RPStat,CMDERR  ;Set init packet defaults.
	mov	es:[bx].RPUnit,al
	and	es:[bx].RPLen,ax
	mov	es:[bx].RPSeg,cs
	mov	al,es:[bx].RPDN	;Set our assigned drive number.
	add	DMLtr.lb,al
	les	si,es:[bx].RPCL	;Get command-line data pointer.
	push	cs		;Do common initialization above.
	call	I_Init
	jc s	I_Exit		;If any init error, exit now!
	or	DvrHdr.dwd,-1	;Get rid of .COM entry "jump".
	xor	ax,ax		;Clear .SYS driver's stack.
	mov	cx,(SYSSTAK/2)
	mov	di,(SysEnd-@)
	push	cs
	pop	es
	rep	stosw
	mov	@Stack,di	;Set .SYS driver stack pointer.
	lds	bx,RqPkt	;Post "init" packet results.
	mov	[bx].RPStat,SUCCESS
	mov	[bx].RPLen,di
	inc	[bx].RPUnit
	push	cs
	push	offset (BBArray-@)
	pop	[bx].RPCL
I_Exit:	pop	dx		;Reload 16-bit registers we used.
	pop	bx
	pop	ax
	pop	es		;Reload CPU segment registers.
	pop	ds
	popf			;Reload CPU flags and exit.
	retf
;
; .COM Driver Initialization Routine.   Note that this logic gets a
;   code-segment that is 256 bytes LOWER than the driver, since DOS
;   adds a "Program Segment Prefix" (PSP) as we load.
;
I_COM:	mov	si,129		;Load command-line pointer (command
	push	cs		;  line data starts at PSP byte 129).
	pop	es
	push	cs		     ;Stack initialization exit addr.
	mov	ax,offset (I_CRet-B)
	push	ax
	mov	ax,cs		;"Call" common initialization logic.
	add	ax,((@-B)/16)	;(Must use adjusted code-segment and
	push	ax		;   also must use only 8086 commands
	mov	ax,(I_Init-@)	;   as CPU type is not-yet "known"!).
	push	ax
	retf
I_CRet:	mov	ax,04C00h	;Get Int 21h "exit" code.
	jc s	I_CEnd		;If any init error, go exit!
	xor	cx,cx		;Load & reset "environment" segment.
	xchg	cx,cs:[0002Ch]
	mov	es,cx		;Free our "environment" memory (saves
	mov	ah,049h		;  96 bytes for use by other things!).
	int	021h
	mov	ah,031h		      ;Get Int 21h "TSR" code.
	mov	dx,((ComEnd+64-@)/16) ;Get resident paragraph count.
I_CEnd:	int	021h		      ;Issue "exit" or "TSR" request.
;
; Subroutines to issue initialization "external" calls.
;
I_XMov:	mov	ah,00Bh		;XMS move -- set move request code.
I_XMS:	push	si		;XMS init -- save SI-reg. for moves.
	call	XEntry		;Issue desired XMS request.
	pop	si		;Reload SI-reg.
	dec	ax		;Zero AX-reg. if success, -1 if error.
	jmp s	I_IntX		;Restore driver settings, then exit.
I_Msg:	mov	ah,009h		;Message -- get "display string" code.
I_In21:	int	021h		;General DOS request -- issue Int 21h.
	jmp s	I_IntX		;Restore driver settings, then exit.
I_In2F:	int	02Fh		;"Multiplex" -- issue XMS request.
I_IntX:	sti			;RESTORE all critical driver settings!
	cld			;(Never-NEVER "trust" external code!).
	push	cs
	pop	ds
	ret			;Exit.
;
; Initialization Messages, Part 2.
;
DvMsg	db	'Drive '
DMLtr	db	'A: assigned.',CR,LF,'$'
NFMsg	db	'No free drive$'
DEMsg	db	'A: unusable$'
XEMsg	db	'XMS init error$'
Suffx	db	'; RDISK not loaded!',CR,LF,'$'
	align	4
;
; "Boot" Block Array.
;
BBArray	dw	(BootBPB-@),0	;Array (only 1) of "boot" blocks.
;
; Initialization "Boot" Block.
;
BBlock	db	0,0,0		;"Boot" block header.
	db	'MSDOS4.1'	;MSDOSx.x REQUIRED by some programs!
BootBPB	dw	512		;Bytes per sector,	always 512.
CluSecs	db	0		;Sectors per cluster,	set by Init.
	dw	1		;Reserved sector count,	always 1.
	db	1		;Number of FAT copies,	always 1.
	dw	512		;RootDirectory entries,	always 512.
DSize12	dw	0		;FAT-12 total sectors,	0 if FAT-16.
	db	0F8h		;Media-descriptor byte,	always 0F8h.
FATSecs	dw	0		;Sectors per FAT table,	set by Init.
	dw	63		;Sectors per track,	always 63.
	dw	255		;Number of heads,	always 255.
	dd	0		;"Hidden" sector count,	always 0.
DSize16	dd	25		;FAT-16 total sectors,	0 if FAT-12.
BPBLen	equ	($-BootBPB)	;(Resident BPB length).
	db	0F0h		;BIOS disk number,	"fake" value.
	db	0		;Windows/NT disk flags,	always 0.
	db	029h		;DOS "signature" byte,	always 029h.
	dw	05000h		;Serial-number time, always 10 A.M.
	dw	01421h		;Serial-number date, always 1-Jan-90.
	db	'REALWHAMBAM'	;Fixed volume name.
	db	'FAT16   '	;Fixed file-system type.
BBLen	equ	($-BBlock)	;"Boot" Block total length.
;
; Initialization FAT Table Data (1st 4 bytes only, all we need!).
;
FATTbl	dd	0FFFFFFF8h	;Initial FAT Table data.
FATLen	equ	($-FATTbl)	;Initial FAT Table total length.
;
; Initialization Volume Label.
;
VolLbl	db	'HalleLU-Jah'	;Ye Olde Volume Label.
	db	008h		;Directory "attribute" flags.
	db	10 dup (0)	;(Unused).
	dw	05000h		;Recording time, always 10 A.M.
	dw	01421h		;Recording date, always 1-Jan-90.
VLLen	equ	($-VolLbl)	;Volume Label total length.
CODE	ends
	end
