; Copyright (C) 1995-1997 CW Sandmann (sandmann@clio.rice.edu) 1206 Braelinn, Sugar Land, TX 77479
	title	xms
	include	segdefs.inc

	start_bss

xms_entry	label	dword
	dd	?

	end_bss

;------------------------------------------------------------------------

	start_code16

; int xms_installed(void);
	public	_xms_installed
_xms_installed	proc	near
	mov	ax,04300h
	int	02fH
	cmp	al,080h
	je	short present
	xor	ax,ax
	ret
present:
	mov	ax,04310h
	int	02fh
	mov	word ptr xms_entry,bx
	mov	word ptr (xms_entry+2),es
	mov	ax,1
	ret
_xms_installed	endp

; ulong xms_query_extended_memory(void) - return largest free block
	public	_xms_query_extended_memory
_xms_query_extended_memory	proc	near
	mov	ah,88H
	call	[xms_entry]
	or	bl,bl
	jne	short fail_new_call
	mov	edx,eax
	shr	edx,16
	ret
fail_new_call:
	mov	ah,08H
	call	[xms_entry]
	xor	dx,dx
	ret
_xms_query_extended_memory	endp

; int xms_local_enable_a20(void)
	public	_xms_local_enable_a20
_xms_local_enable_a20	proc	near
	mov	ah,05H
	jmp	short no_arg_common
_xms_local_enable_a20	endp

; int xms_local_disable_a20(void)
	public	_xms_local_disable_a20
_xms_local_disable_a20	proc	near
	mov	ah,06H
	jmp	short no_arg_common
_xms_local_disable_a20	endp

; int xms_emb_free(short handle);
	public	_xms_emb_free
_xms_emb_free	proc	near
	mov	ah,00aH
handle_common:
	push	bp
	mov	bp,sp
	mov	dx,[bp+4]
	pop	bp
no_arg_common:
	call	[xms_entry]
	ret
_xms_emb_free	endp

; int xms_unlock_emb(short handle);
	public	_xms_unlock_emb
_xms_unlock_emb	proc	near
	mov	ah,00dH
	jmp	short handle_common
_xms_unlock_emb	endp

; ulong xms_lock_emb(short handle);
	public	_xms_lock_emb
_xms_lock_emb	proc	near
	push	bp
	mov	bp,sp
	mov	ah,00cH
	mov	dx,[bp+4]
 	pop	bp
	call	[xms_entry]
	or	ax,ax
	je	short lock_failed
	xchg	bx,ax				; Shorter than mov ax,bx
	ret
lock_failed:
	mov	dx,ax				; Zero ulong
 	ret
_xms_lock_emb	endp

; int xms_emb_allocate(ulong size);
	public	_xms_emb_allocate
_xms_emb_allocate	proc	near
	push	bp
	mov	bp,sp
	mov	edx,[bp+4]
	pop	bp
	mov	eax,edx
	shr	eax,16
	je	short old_call
	mov	ah,80h
old_call:
	add	ah,09h
	call	[xms_entry]
	or	ax,ax
	je	short alloc_failed
	xchg	ax,dx
	ret
alloc_failed:
	dec	ax				; ax = -1
	ret
_xms_emb_allocate	endp

	end_code16

	end
