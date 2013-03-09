;------------------------------------------------------------------------------
; Prints null-terminated string pointed to by ax through INT10, AH=0xe.
; In:	AX - pointer to string to print.
; Out:	AX - destroyed.

proc	print_string
	push	bp di bx
	push	ds
	mov	di, ax
	mov	ax, cs
	mov	ds, ax
	mov	ah, 0eh
	mov	bx, 0070h
@@loop:	
	mov	al, [di]
	or	al, al
	jz	@@epilogue
	int	10h
	inc	di
	jmp	@@loop
@@epilogue:	
	pop	ds
	pop	bx di bp
	ret
endp	print_string

