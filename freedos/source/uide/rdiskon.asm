	page	59,132
	title	RDISKON -- "Re-enabler" For RDISK Drives.
;
; General Equations.
;
	.386p			;Allow use of 80386 instructions.
s	equ	<short>		;Make a conditional jump "short".
CR	equ	00Dh		;ASCII carriage-return.
LF	equ	00Ah		;ASCII line-feed.
;
; Main Routine.
;
CODE	segment	public use16 'CODE'
	assume	cs:CODE,ds:CODE
	org	00100h
Start:	mov	si,00081h	;Point to PSP command-line data.
	push	cs
	pop	ds
NxtChr:	mov	al,[si]		;Get next command-line byte.
	inc	si		;Bump pointer past this byte.
	cmp	al,0		;Is byte the command-line terminator?
	je s	Exit		;Yes, drive not found -- exit below.
	cmp	al,LF		;Is byte an ASCII line-feed?
	je s	Exit		;Yes, drive not found -- exit below.
	cmp	al,CR		;Is byte an ASCII carriage-return?
	je s	Exit		;Yes, drive not found -- exit below.
	and	ax,000DFh	;Mask out byte's lower-case bit.
	cmp	al,'Z'		;If byte is above a 'Z', skip it.
	ja s	NxtChr
	sub	al,'A'		;If byte is below an 'A', skip it.
	jb s	NxtChr
	xchg	ax,dx		;Set drive number in DX-reg.
	mov	ax,05F07h	;Re-enable desired drive.
	int	021h
Exit:	mov	ax,04C00h	;Exit back to DOS.
	int	021h
CODE	ends
	end	Start

