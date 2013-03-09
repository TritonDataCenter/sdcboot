	page    59,132
	title   CC -- Clear Cache.
	.386p			;Allow use of 80386 commands.
s	equ	<short>		;Make conditional jumps "short".
HDISKS	equ	00475h		;BIOS hard-disk count address.
;
; Segment Declarations.
;
CODE	segment	public use16 'CODE'
	assume	cs:CODE,ds:CODE
	org	00100h
;
; Main Program Routine.
;
Start:	mov	ax,cs		;Set our DS-register.
	mov	ds,ax
	xor	ax,ax		;Point ES-reg. to low-memory.
	mov	es,ax
	mov	al,es:HDISKS	;Get number of BIOS hard-disks.
	or	al,al		;Any hard-disks on this system?
	jz s	Exit		;No, just exit.
	mov	DiskCt,al	;Set our hard-disk count below.
Next:	mov	ah,0		;Do BIOS "reset" for next disk.
	mov	dl,UnitNo
	int	013h
	mov	ax,cs		;Reset our DS-register.
	mov	ds,ax
	inc	UnitNo		;Increment disk unit number.
	dec	DiskCt		;More disks to go?
	jnz s	Next		;Yes, go reset next one.
Exit:	mov	ax,04C00h	;Done -- Exit back to DOS.
	int	021h
;
; Program Variables.
;
DiskCt	db	0		;Number of BIOS hard-disks.
UnitNo	db	080h		;Current BIOS unit number.
CODE	ends
	end	Start

