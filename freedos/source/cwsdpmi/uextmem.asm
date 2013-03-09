; Copyright (C) 2009 CW Sandmann (cwsdpmi@earthlink.net) 1206 Braelinn, Sugar Land, TX 77479
;
; This file is distributed under the terms listed in the document
; "copying.cws", available from CW Sandmann at the address above.
; A copy of "copying.cws" should accompany this file; if not, a copy
; should be available from where this file was obtained.  This file
; may not be distributed without a verbatim copy of "copying.cws".
;
; This file is distributed WITHOUT ANY WARRANTY; without even the implied
; warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

; Should work on PCs built since about 2002.
; Find memory block above 4GB, return 4MB page number and count if present.
; Return 1 if found, 0 if not.
; Assumes ds=ss

	include segdefs.inc

	start_code16

	public	_uextmem
_uextmem:
	push	di			; di+24, di+26 is return IP
	sub	sp, 24			; Buffer on stack (extra dword)
	push	ss
	pop	es
	mov	di, sp			; ES:DI return buffer
	xor	ebx, ebx		; ebx 0 to start
e820loop:
	mov	edx, 534D4150h		; "SMAP" into edx
	mov	eax, 0e820h
	mov 	ecx, 20			; ask for 20 bytes
	int 	15h
	jc 	short failed		; carry set: unsupported function
	cmp	eax, 534D4150h		; on success, eax reset to "SMAP"
	jne	short failed
	jcxz	skip			; skip 0 length entries
;
	cmp	byte ptr [di+16], 1	; Entry type available memory?
	jne	short skip
;
	cmp	byte ptr [di+4],0	; Address >4GB?
	je	short skip
;
	mov	ecx, [di+10]		; region length in 64K increments
	shr	ecx, 6			; now 4MB pages, 256/GB, in cx
	jcxz	skip
	mov	bx, [di+30]		; argument 2
	mov	[bx], cx
;
	mov	eax, [di+1]		; region start, assume 256 byte aligned
	shr	eax, 14			; 14=22-8; could double check 4MB align
	mov	di, [di+28]		; argument 1
	mov	[di], ax		; 4MB page number start	

	mov	ax, 1
	jmp	short done
skip:
	test	ebx, ebx		; if ebx resets to 0, list is complete
	jne	short e820loop
failed:
	xor	ax, ax
done:
	add	sp, 24
	pop	di
	ret

;------------------------------------------------------------------------

	end_code16

	end
