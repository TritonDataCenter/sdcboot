; Copyright (C) 1995-1997 CW Sandmann (sandmann@clio.rice.edu) 1206 Braelinn, Sugar Land, TX 77479
; Copyright (C) 1993 DJ Delorie, 24 Kirsten Ave, Rochester NH 03867-2954
;
; This file is distributed under the terms listed in the document
; "copying.cws", available from CW Sandmann at the address above.
; A copy of "copying.cws" should accompany this file; if not, a copy
; should be available from where this file was obtained.  This file
; may not be distributed without a verbatim copy of "copying.cws".
;
; This file is distributed WITHOUT ANY WARRANTY; without even the implied
; warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

	title	tables
	include segdefs.inc
	include tss.inc
	include gdt.inc

;------------------------------------------------------------------------

	start_data16

	extrn	_tss_ptr:word
	extrn	_was_exception:byte
	extrn	_hard_master_lo:byte
	extrn	_npx:byte
	extrn	_DPMIsp:word

has_error	db	1,0,1,1,1,1,1,0

	end_data16

;------------------------------------------------------------------------

	start_bss

	public	_i_tss
_i_tss	label	tss_s
	db	type tss_s dup (?)

ivec_number	dw	?

	extrn	_locked_stack:dword
	extrn	_locked_count:byte
	extrn	_user_interrupt_handler:fword
	extrn	_dpmisim_regs:dword
	end_bss

;------------------------------------------------------------------------

	start_code16
	extrn	_user_interrupt_return:near

; Code size reduction (improved) by Morten Welinder

	public	_ivec0, _ivec1
;	align	4		; fix segdefs
_ivec0:
	push	ds		; 1 byte
	call	ivec_common	; 3 bytes
_ivec1:
rept 255
	push	ds		; 1 byte
	call	ivec_common	; 3 bytes
endm

ivec_common:
	push	g_pdata
	pop	ds
	pop	ivec_number
	sub	ivec_number, offset _ivec1	; pushes address *after* call
	shr	ivec_number, 2
	pop	ds
	jmpt	g_itss			; macro for jump to task segment itss

; Task set up with ds = g_rdata, interrupts disabled
	public	_interrupt_common
_interrupt_common:
	mov	bx,_tss_ptr
	mov	ax,ivec_number
	mov	[bx].tss_irqn,al
	mov	esi,[bx].tss_esp
	mov	fs,[bx].tss_ss		; fs:esi -> stack
	cmp	al,15
	ja	short has_no_error
	sub	al,8
	jb	short has_no_error
	mov	di,ax			; DI has the IRQ value now
	cmp	has_error[di],0
	je	short check_SW_int
	mov	ax,fs:[esi+8]
	and	ah,30h			; Flags IOPL if SW int, CS if exception
	cmp	ah,30h
	je	check_SW_int		; We don't have selectors this large !
	mov	eax,fs:[esi]
	mov	[bx].tss_error,eax
	add	esi,4
	mov	eax,cr2			; For page faults
	mov	[bx].tss_cr2,eax
	jmp	short has_no_error
check_SW_int:
	lgs	ecx,fs:[esi]		; CS:EIP -> GS:ecx
	mov	ax,gs:[ecx-2]		; Get two bytes before; Int 0x??
	cmp	al,0cdh			
	jne	short has_no_error
	cmp	[bx].tss_irqn,ah
	jne	short has_no_error	; Not a SW interrupt
	mov	cx,di
	add	cl,_hard_master_lo	; SW int in range 8-15 redirected
	mov	[bx].tss_irqn,cl	
has_no_error:
	mov	eax,fs:[esi]		; eip
	mov	[bx].tss_eip,eax
	mov	eax,fs:[esi+8]
	mov	[bx].tss_eflags,eax
	mov	ax,fs:[esi+4]
	mov	cx,[bx].tss_cs
	mov	[bx].tss_cs,ax
	add	esi,12
	xor	ax,cx			; Are low 3 bits equal?
	test	al,3			; Our CPL = 0; is their CS RPL ?
	jz	short same_privilege
	mov	ax,fs:[esi+4]		; saved SS
	mov	[bx].tss_ss,ax
	mov	esi,fs:[esi]		; saved SP
same_privilege:
	mov	[bx].tss_esp,esi	; store corrected stack pointer
	mov	_was_exception,1
	xor	eax,eax
	mov	fs,ax			; just in case it becomes invalid!
	mov	gs,ax			; just in case it becomes invalid!
	mov	cr2,eax			; so we can tell INT 0E from page fault
	jmpt	g_ctss			; pass control back to real mode
	jmp	_interrupt_common	; it's a task

IF run_ring EQ 0
; Task set up with ds = g_rdata, interrupts disabled
	public	_double_fault
_double_fault:
	mov	bx,_tss_ptr
	mov	[bx].tss_irqn,8		; double fault
	pop	[bx].tss_error		; dword
	mov	_was_exception,1
	jmpt	g_ctss
	jmp	short _double_fault
ENDIF

;------------------------------------------------------------------------
;	This code takes HW interrupts which must be handled at Ring 0 to
;	make sure the stack is valid and converts them to Ring 3 user 
;	interrupt handlers on the locked stack.  This code assumes a 
;	ring transition (since we run with interrrupts disabled at our 
;	ring 0 code) but probably should be fixed to jump to the ivec
;	handler in that case.

	public	_irq0, _irq1
;	align	4		; fix segdefs

irq	macro	n
	push	n			; 2 bytes
	jmp	short irq_common	; 2 bytes
	endm
_irq0:
	irq	0
_irq1:
	x=6
	rept 15
	 irq	x
	 x=x+6
	endm

irq_common:
IF run_ring EQ 0
; Interrupts are disabled; the 32-bit IRET 3 dwords (and irq#) on the stack.
; Copy it to the locked stack adding the 2 stack dwords or move it down.
; user_interrupt_return emulates an IRET changing stacks on the same ring.
	push	ds
	push	g_pdata
	pop	ds

	cmp	_locked_count,0		; If on locked stack, OK
	jne	short already_locked
	mov	dword ptr _locked_stack+4064,edi	;Save edi
	lea	edi,_locked_stack+4064	; Locked stack - 20 - 12
	mov	[edi+4],eax		; saved eax
	mov	eax,[esp]		; ds/irq
	mov	[edi+8],eax
	mov	eax,[esp+4]		; offset
	mov	[edi+12],eax
	mov	eax,[esp+8]		; selector
	mov	[edi+16],eax
	mov	eax,[esp+12]		; flags
	mov	[edi+20],eax
	lea	eax,[esp+16]
	mov	[edi+24],eax
	mov	[edi+28],ss
	mov	ax,ds
	mov	ss,ax
	mov	esp,edi			; Now on locked stack
	pop	edi
	jmp	short @f1
already_locked:
	sub	esp,8			; Room for ESP:SS
	push	eax
	mov	eax,[esp+12]
	mov	[esp+4],eax		; ds/irq
	mov	eax,[esp+16]
	mov	[esp+8],eax		; offset
	mov	eax,[esp+20]
	mov	[esp+12],eax		; selector
	mov	eax,[esp+24]
	mov	[esp+16],eax		; flags
	lea	eax,[esp+28]
	mov	[esp+20],eax
	mov	[esp+24],ss
@f1:
	inc	_locked_count
;	At this point we have two dwords on new stack
	sub	esp,24
	mov	eax,[esp+24]
	mov	[esp],eax		; saved eax
	mov	eax,[esp+28]
	mov	[esp+4],eax		; saved ds/irq

	mov	dword ptr [esp+20],offset _TEXT:_user_interrupt_return
	mov	word ptr [esp+24],g_pcode
	mov	dword ptr [esp+28],3002h

	xchg	bx,[esp+6]		; IRQ # times size in BX
	mov	eax,dword ptr _user_interrupt_handler[bx]
	mov	dword ptr [esp+8],eax
	mov	ax,word ptr _user_interrupt_handler[bx+4]
	mov	word ptr [esp+12],ax
	mov	dword ptr [esp+16],3002h

	pop	eax
	pop	ds
	pop	bx
	iretd
ELSE
; Our stack will be ring 0 at end of TSS after ring change; interrupts are 
; disabled.  We have the 32-bit ring change IRET structure on the stack.
; Copy it to the ring 3 stack; user_interrupt_return emulates an IRET 
; changing stacks on the same ring.
	push	bp
	mov	bp,sp
	push	ds
	push	es

	push	esi
	push	edi
	push	ecx

	push	g_pdata
	pop	ds

	cmp	_locked_count,0		; If on locked stack, OK
	jne	short already_locked
	lea	edi,_locked_stack+4076	; Locked stack - 20
	push	g_pdata
	pop	es
	jmp	short @f1
already_locked:
	mov	edi,[bp+16]		; Saved ESP
	mov	es,[bp+20]		; Saved SS
	sub	edi,20			; Make room on current locked stack
@f1:
	inc	_locked_count
	mov	ecx,5			; 20 bytes
	movzx	esi,bp
	add	si,4			; SS:ESI (adjust for local pushes)
	cld
	rep	movs dword ptr es:[edi],dword ptr ss:[esi]	;ecx count
	sub	edi,32			; 20 bytes + 12 more for iret frame
	mov	dword ptr es:[edi+8],3002h
	mov	word ptr es:[edi+4],g_pcode
	mov	dword ptr es:[edi],offset _TEXT:_user_interrupt_return
	
	xchg	bx,[bp+2]		; IRQ # times size in BX
	mov	ecx,dword ptr _user_interrupt_handler[bx]
	mov	dword ptr [bp+4],ecx
	mov	cx,word ptr _user_interrupt_handler[bx+4]
	mov	word ptr [bp+8],cx
	mov	word ptr [bp+12],3002h
	mov	dword ptr [bp+16],edi	; The new ESP
	mov	word ptr [bp+20],es	; The new SS

	pop	ecx
	pop	edi
	pop	esi
	pop	es
	pop	ds
	pop	bp
	pop	bx
	iretd
ENDIF

IFDEF I31PROT
extrn _i_31prot:near

; Note for later: since we may be changing descriptors we should make sure that
; all segment registers are reloaded.  Ones that are no longer valid are zeroed.

	public	_ivec31
_ivec31:
	cmp	ah,3
	jne	_ivec0 + 31h * (_ivec1 - _ivec0)
	cmp	al,2
	jg	_ivec0 + 31h * (_ivec1 - _ivec0)
; Real mode call - ES:EDI points to structure
; Set up DS:ESI to point to their copy, ES:EDI to ours
	push	ds es esi edi ecx
	push	es
	pop	ds
	mov	esi,edi
	mov	cx,g_pdata
	mov	es,cx
	mov	edi,offset DGROUP:_dpmisim_regs
	mov	ecx,25
	cld
	rep	movs word ptr es:[edi],word ptr ds:[esi]
	pop	ecx edi esi es ds
	jmp	_ivec0 + 31h * (_ivec1 - _ivec0)

; This is where we return to restore the register structure
; Because of a bug in GO32 V1.10 & V1.11, we can't touch the user stack!
	public	_ivec31x
	public	_i30x_jump
	public	_i30x_stack
	public	_i30x_sti
_ivec31x:
; Real mode return - ES:EDI points to structure
; Set up DS:ESI to point to to our copy
	pushf
	push	ds esi edi ecx
	mov	cx,g_pdata
	mov	ds,cx
	mov	esi,offset DGROUP:_dpmisim_regs
	mov	ecx,21			; don't update CS:IP or SS:SP
	cld
	rep	movs word ptr es:[edi],word ptr ds:[esi]
	pop	ecx edi esi ds
	popf
	lss	esp,fword ptr cs:_i30x_stack
_i30x_sti	db	?
	db	66h, 0eah				; far jmp to 32 bit
_i30x_jump	dd	?
	dw	?
_i30x_stack	dd	?
	dw	?
	
ENDIF

	public	_ivec7
_ivec7:
	clts
;	push	eax
;	mov	eax,cr0
;	and	al,0F3h			; Clear EM & TS bits
;	mov	cr0,eax
;	pop	eax
	iretd

	public	_real_i8
_real_i8:
	int	08h
	iret
	int	09h
	iret
	int	0ah
	iret
	int	0bh
	iret
	int	0ch
	iret
	int	0dh
	iret
	int	0eh
	iret
	int	0fh
	iret

; Improved code by Morten Welinder.  CWS note: We must save bp si di here, 
; (since called by TC).

	public	_generic_handler
_generic_handler:
	push	bp si di
	push	_DPMIsp		; Needed for RMCB's to be recursive
	mov	_DPMIsp,sp
	sub	_DPMIsp,spare_stack	; space + extra for HW interrupt
	push	ds
	mov	bx,_tss_ptr
	mov	ax,word ptr [bx].tss_eflags
	and	ax,0011111011010101b		; user flags
	push	ax
	push	cs
	call	generic1			; ax, cs, "call" makes iret
	pushf
	cli
	push	ebx
	mov	bx,_tss_ptr
	mov	[bx].tss_eax,eax
	mov	[bx].tss_ecx,ecx
	mov	[bx].tss_edx,edx
	mov	[bx].tss_esi,esi
	mov	[bx].tss_edi,edi
	mov	[bx].tss_ebp,ebp
	pop	dword ptr [bx].tss_ebx
	pop	ax
	mov	dx,0000111011010101b		; user flags
	mov	cx,word ptr [bx].tss_eflags
	and	ax,dx
	not	dx
	and	cx,dx
	or	ax,cx
	mov	word ptr [bx].tss_eflags,ax

	pop	ds
	pop	_DPMIsp
	pop	di si bp
	xor	ax,ax
	retn

; Simulate interrupt call setup (bigger/slower than self modifying code,
; but I may need it with hw interrupt callbacks)
generic1:
	and	ah,not 2
	push	ax				; Flags with IF=0
	xor	cx,cx
	mov	es,cx	
	mov	cl,[bx].tss_irqn
	shl	cx,2
	mov	di,cx
	push	dword ptr es:[di]		; CS:IP of int handler

; DS & ES are undefined -- could use segment translation but don't bother

	mov	eax,[bx].tss_eax
	mov	ecx,[bx].tss_ecx
	mov	edx,[bx].tss_edx
	mov	esi,[bx].tss_esi
	mov	edi,[bx].tss_edi
	mov	ebp,[bx].tss_ebp
	mov	ebx,[bx].tss_ebx

	iret					; Actually an "int tss_irqn"

	end_code16

;------------------------------------------------------------------------

	end
