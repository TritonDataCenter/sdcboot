; Copyright (C) 1995-1999 CW Sandmann (sandmann@clio.rice.edu) 1206 Braelinn, Sugar Land, TX 77479
;
; This file is distributed under the terms listed in the document
; "copying.cws", available from CW Sandmann at the address above.
; A copy of "copying.cws" should accompany this file; if not, a copy
; should be available from where this file was obtained.  This file
; may not be distributed without a verbatim copy of "copying.cws".
;
; This file is distributed WITHOUT ANY WARRANTY; without even the implied
; warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

	title	cwsdpmi
	include	segdefs.inc

;------------------------------------------------------------------------

	start_data16
	public	__psp, __osmajor, __osminor, __brklvl, ___brklvl
__psp		dw	0
__osmajor	db	0
__osminor	db	0
__brklvl 	dd	0
___brklvl 	dw	offset DGROUP:bss_end
firstf		dw	offset DGROUP:bss_end
	extrn __stklen:word
	end_data16

	start_bss
bss_start 	label	byte
	end_bss

_bssend segment use16
bss_end		label	word
	ends

;------------------------------------------------------------------------

	start_code16

extrn	_main:near
	.8086

start	proc near
	call	lsetup
	jmp	_main		; Never return, so this is OK
	endp

	PUBLIC	lsetup
lsetup	proc near
	mov	bp, ds:[2]	; Highest memory segment
	mov	dx, DGROUP
	mov	ds, dx
	mov	__psp, es

	sub	bp, dx		; Our current DGROUP size in paragraphs
	mov	di, __stklen	; Requested stack size
	add	di, offset DGROUP:bss_end
	mov	cl, 4
	shr	di, cl		; Convert bss_end+stack to paragraphs
	inc	di		; Wanted DGROUP size in paragraphs
	cmp	bp, di
	jb	short _exit	; Not enough memory (but we fixed header!?)

	mov	di, 1000h	; More than 64K?
	cmp	bp, di
	ja	short mem_ok	; Someone carried away with CWSPARAM, 64K Max
	mov	di, bp		; The usual case

;	Move to new stack, then return extra memory to DOS

mem_ok:	mov	bx, di
	shl	di, cl		; Convert back to bytes from paragraphs
	pop	ax		; Return address
	mov	ss, dx		; Set the program stack
	mov	sp, di
	push	ax		; Return address

	add	bx, dx		; Segment value for end of our new DGROUP
	mov	word ptr __brklvl+2, bx
;	mov	ax, es		; __psp 
;	sub	bx, ax		; Number of paragraphs to keep
;	mov	ah, 04Ah	; DOS resize memory block call
;tsr..	int	021h

;	Clear BSS area to zero

	push	ds
	pop	es
	mov	di, offset DGROUP:bss_start
	mov	cx, offset DGROUP:bss_end
	sub	cx, di
	xor	ax, ax
	cld
	rep	stosb

	mov	ah, 30h		; get DOS version into TCC like variable
	int	21h	
	mov	word ptr __osmajor, ax

	mov	ds:[bss_end],8000h	; For malloc
	mov	es, __psp
	ret
	endp
	
	PUBLIC	__exit,_exit	; Always error exit here
__exit	proc	near
_exit:	mov	ax,4C01h
	int	21h
	endp

	PUBLIC	_unlink
_unlink	proc	near
	mov	ah,41h		; Delete file
	jmp	short comn1
	endp

	PUBLIC	__creat
__creat	proc	near
	xor	cx,cx		; No attributes (ignore bp+6)
	mov	ah,3Ch		; Create new (or zero length existing) file
comn1:	push	bp
	mov	bp,sp
	mov	dx,[bp+4]	; Name of file
	int	21h
	jnc	short do_ret
	mov	ah,-1		; AX thus negative, shorter, AL has err code
	jmp	short do_ret
	endp

	PUBLIC	__write
__write	proc	near
	mov	ah,40h		; Write
com3:	push	bp
	mov	bp,sp
	mov	cx,[bp+8]	; Buffer size
	mov	dx,[bp+6]	; Buffer offset
com1:	mov	bx,[bp+4]	; File handle
	int	21h
	jnc	short do_ret
	xor	ax,ax
do_ret:	pop	bp
	ret
	endp

	PUBLIC	__read
__read	proc	near
	mov	ah,3fh		; Read
	jmp	short com3
	endp

	PUBLIC	_lseek
_lseek	proc	near
	mov	ax,4200h	; Seek absolute (always abs here)
	jmp	short com3
	endp

	PUBLIC	__close
__close	proc	near
	push	bp
	mov	bp,sp
	mov	ah,3eh		; Close
	jmp	short com1
	endp

	PUBLIC	__restorezero	; Bogus, for TCC compatibility
__restorezero	proc	near
	ret
	endp

	PUBLIC	_free
_free	proc	near
	push	bp
	mov	bp,sp
	mov	bx,[bp+4]	; pointer
	dec	bx
	or	byte ptr [bx], 80h	; negative size means "free"
	dec	bx
	cmp	firstf,bx
	jb	short f1
	mov	firstf,bx
f1:	pop	bp
	ret
	endp
	
	PUBLIC	_malloc
_malloc	proc	near
	push	bp
	mov	bp,sp
	mov	dx,[bp+4]	; size
	add	dx,3		; for header and roundup
	and	dl,0feh		; clear roundup

	mov	bx,firstf
m0:	mov	ax,[bx]		; header size/tag
	test	ax,ax
	js	short m1
	cmp	firstf,bx	; not free!
	jne	short m3
	add	firstf,ax
m3:	add	bx,ax
	jmp	short m0
	
m1:	and	ah,7fh		; get the size
	cmp	ax,dx
	jb	short m2

;	We have a big enough block

	sub	ax,dx		; ax is now extra
	jmp	short m4

m2:	or	ax,ax
	jne	short m3	; don't try to merge since all same size	

;	End of chain (ax = 0)

	mov	cx,bx
	add	cx,dx		; new end
	add	cx,380h		; enough for a single HW interrupt
	cmp	cx,sp
	ja	short mend
	mov	ax,8000h
m4:	mov	[bx],dx
	add	bx,dx
	or	ax,ax
	je	short m5
	or	ah,80h
	mov	[bx],ax
m5:	mov	ax,bx
	sub	ax,dx
	cmp	firstf,ax
	jne	short m6
	mov	firstf,bx
m6:	add	ax,2
mend:	pop	bp
	ret
	endp

csub	macro	name
	public	name
name :	push	bp
	mov	bp,sp
	endm

asub	macro	name
	public	name
name :
	endm

csub	_getvect
	mov	ah,35h
	mov	al,[bp+4]
	int	21h
	mov	dx,es
	xchg	bx,ax		; Shorter than mov ax,bx
	pop	bp
	ret

csub	_setvect
	mov	ah,25h
	push	ds
	mov	al,[bp+4]
	lds	dx,[bp+6]
	int	21h
	pop	ds
	pop	bp
	ret

csub	_memset
	push	ds
	pop	es
ms1:	cld
	push	di
	mov	di,[bp+4]
	mov	al,[bp+6]
	mov	cx,[bp+8]
	rep	stosb
	pop	di
	pop	bp
	ret

csub	_memsetf
	mov	es,[bp+10]
	jmp	short ms1
	
	
csub	_movedata
	push	si
	push	di
	push	ds
	mov	ds,[bp+4]
	les	si,[bp+6]		; si=6,es=8 (horrid hack for space!)
	mov	di,[bp+10]
	mov	cx,[bp+12]
	cld
	rep	movsb
	pop	ds
	pop	di
	pop	si
	pop	bp
	ret

csub	N_SCOPY@
	push	si
	push	di
	push	ds
	lds	si,[bp+4]
	les	di,[bp+8]
	cld
	rep	movsb
	pop	ds
	pop	di
	pop	si
	pop	bp
	ret	8

asub	N_LXMUL@			; Cheat - assume only result is long
	mul	bx
	ret

	.386
asub	N_LXLSH@
	shld	dx,ax,cl
	xor	bx,bx
	shld	ax,bx,cl
	ret

asub	N_LXRSH@
	mov	bx,dx
	sar	bx,15
rsh:	shrd	ax,dx,cl
	shrd	dx,bx,cl
	ret

asub	N_LXURSH@
;	xor	bx,bx
;	jmp	short rsh
	push	dx
	push	ax
	pop	edx
	shr	edx,cl
	push	edx
	pop	ax
	pop	dx
	ret

IFDEF TC2FIXUP
asub	SCOPY@
	pop	eax
	push	ax
	jmp	short N_SCOPY@
asub	LXMUL@
	call	N_LXMUL@
	retf
asub	LXLSH@
	call	N_LXLSH@
	retf
asub	LXRSH@
	call	N_LXRSH@
	retf
asub	LXURSH@
	call	N_LXURSH@
	retf
ENDIF
	end_code16

_STACK	segment stack 'STACK'
	db	128 dup(?)	;minimal startup stack (overwritten)
	ends

	end	start
