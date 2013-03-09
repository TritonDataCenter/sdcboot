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

	title	dpmisim
	include	segdefs.inc
	include tss.inc
	include gdt.inc

;------------------------------------------------------------------------

	start_data16

	public	_DPMIsp
_DPMIsp dw	?
_DPMIss dw	DGROUP

	end_data16

	start_bss

	extrn	_a_tss:tss_s
	extrn	_tss_ptr:word
	extrn	_locked_stack:dword
	extrn	_locked_count:byte
	extrn	_dpmisim_rmcb:dword

	public	_dpmisim_regs
_dpmisim_regs	label	dword
ds_edi	dd	?
ds_esi	dd	?
ds_ebp	dd	?
ds_res	dd	?
ds_ebx	dd	?
ds_edx	dd	?
ds_ecx	dd	?
ds_eax	dd	?
ds_flg	dw	?
ds_es	dw	?
ds_ds	dw	?
ds_fs	dw	?
ds_gs	dw	?
ds_ip	dw	?
ds_cs	dw	?
ds_sp	dw	?
ds_ss	dw	?

	public	_in_rmcb
_in_rmcb	label	byte
	db	?

	end_bss

;------------------------------------------------------------------------

	start_code16

	extrn	_go_til_stop:near
	extrn	_DPMIstartup:near
	extrn	_cputype:near
	extrn	_main1:near

; CWS note: dpmisim may be called recursively.  This routine can transfer
; control back to another program which can also request DPMI services.
; The other routine which is recursive and handles most of the setup is
; DPMIstartup, which is called to enter protected mode the first time.

ds_savesp	dd	?
ds_tmpds	dw	?

	public	_dpmisim
_dpmisim:
	push	bp
	push	si
	push	di

	push	_DPMIsp				;Save old value for restore
	mov	_DPMIsp,sp			;Where to start stack next call

	call	restore_registers
	lss	sp, dword ptr ds_sp

	push	cs
	push	offset dpmisim_return
	push	ds_flg
	push	ds_cs
	push	ds_ip
	mov	ds, ds_ds
	iret	; actually a far call in disguise, loading flags
dpmisim_return:

	call	save_registers			;Also disables interrupts
;	mov	ds_sp, sp			;Not returned
	lss	sp, dword ptr _DPMIsp

	pop	_DPMIsp
	
	pop	di
	pop	si
	pop	bp
	cld
	ret

restore_registers:
	mov	edi, ds_edi
	mov	esi, ds_esi
	mov	ebp, ds_ebp
	mov	ebx, ds_ebx
	mov	edx, ds_edx
	mov	ecx, ds_ecx
	mov	eax, ds_eax
	mov	es, ds_es
	mov	fs, ds_fs
	mov	gs, ds_gs
	ret

save_registers:
	pushf
	cli					;Interlock, this is static
	mov	cs:[ds_tmpds], ds
	push	DGROUP
	pop	ds
	pop	ds_flg

	mov	ds_edi, edi
	mov	ds_esi, esi
	mov	ds_ebp, ebp
	mov	ds_ebx, ebx
	mov	ds_edx, edx
	mov	ds_ecx, ecx
	mov	ds_eax, eax
	mov	ds_es, es
	mov	ds_fs, fs
	mov	ds_gs, gs
	mov	ds_ss, ss
	
	mov	ax, cs:[ds_tmpds]
	mov	ds_ds, ax
	ret

rmcb	macro	n
	push	n
	jmp	short rmcb_common
	endm

	public	_dpmisim_rmcb0
_dpmisim_rmcb0:
	rmcb	0
	public	_dpmisim_rmcb1
_dpmisim_rmcb1:
	x=1
	rept 23				;num_rmcb-1 from dpmisim.h
	 rmcb x
	 x=x+1
	endm

; We enter from real mode here, and must put registers in ES:EDI real mode
; call structure, set DS:ESI as pointer to caller's stack (for modification)
; and set SS:ESP to the 32 bit locked stack

; Needs to be reentrant, and is!  The a_tss values are saved on our stack.
; _DPMISP is stored by all methods to real mode, just in case we get another
; callback.  The static structures are only used temporarily and with 
; interrupts disabled.  Interrupts may be re-enabled during the 
; user routine, but we shut them back off in the critical exit sections.
; This is used for HW interrupt reflection into prot mode from real mode.
; Note, only a single mode switch!

rmcb_common:
	call	save_registers			;Also disables interrupts
	pop	bx			; rmcb number
	mov	ds_sp,sp

	lss	sp,dword ptr _DPMIsp	; set up our local stack
	sub	sp,300h			; A HW interrupt may occur anywhere
					; I give up trying to find it!

; Save a_tss values on our 16 bit stack while we use it
	cld
	push	ss
	pop	es			; ES=DS=SS
	sub	sp,64			; make room to save a_tss info
	mov	si,offset DGROUP:_a_tss.tss_eip
	mov	di,sp
	mov	cx,32
	rep	movsw			; DS:_a_tss to SS:sp

; Set up 32 bit stack for callback
	cmp	_locked_count,0		; already on locked stack?
	jne	short already_locked
	mov	_a_tss.tss_ss, g_pdata	; ss:esp for pointer to temp stack
	lea	ecx,_locked_stack+4096	; end of locked stack
	and	cl,0fch			; longword align stack
	mov	_a_tss.tss_esp, ecx

already_locked:
	inc	_locked_count
	inc	_in_rmcb
	
	shl	bx,4			; each element 16 bytes
	mov	eax, _dpmisim_rmcb[bx]	; cb_address (eip for func)
	mov	cs:rmcboff, eax
	mov	ax, word ptr _dpmisim_rmcb[bx+4]  ; cb_sel (cs for func call)
	mov	cs:rmcbseg, ax
	mov	ax, word ptr _dpmisim_rmcb[bx+6]  ; cb_type
	mov	word ptr _a_tss.tss_eax, ax

	or	al,al
	je	short user_call_type

; Internal RMCB type for interrupts
	mov	ax, _a_tss.tss_ss
	mov	_a_tss.tss_es, ax		; We want regs stored on stack
	sub	_a_tss.tss_esp, 34h		; Make room, long aligned
	mov	eax, _a_tss.tss_esp
	mov	_a_tss.tss_edi, eax		; ES:EDI == SS:ESP
	jmp	short type_common

user_call_type:
	mov	eax,_dpmisim_rmcb[bx+8]	; reg_ptr
	mov	_a_tss.tss_edi, eax
	mov	ax, word ptr _dpmisim_rmcb[bx+12]  ; reg_sel
	mov	_a_tss.tss_es, ax
type_common:
	mov	word ptr _a_tss.tss_eflags, 3002h	; disable interrupts

	mov	_a_tss.tss_eip, offset rmcb_task
	mov	_a_tss.tss_cs, g_pcode

	mov	_a_tss.tss_ds, g_core	; ds:esi for pointer to caller stack
	movzx	eax,ds_ss
	shl	eax,4
	movzx	ebx,ds_sp
	add	eax,ebx
	mov	_a_tss.tss_esi, eax

; We take advantage of the fact that some registers are "undefined" in the spec
; We set FS:EBX to point to dpmisim_regs so we can do the copies in prot mode

	mov	_a_tss.tss_fs, g_pdata
	mov	_a_tss.tss_ebx, offset DGROUP:_dpmisim_regs

	push	_tss_ptr
	mov	_tss_ptr,offset DGROUP:_a_tss

	call	_go_til_stop			;tss_ptr acts as non-zero arg
	
	pop	_tss_ptr

; Note: go_til_stop leaves interrupts disabled on entry to real mode, so OK!

	dec	_in_rmcb
	dec	_locked_count
	push	ds
	pop	es			; ES=DS=SS
	mov	si,sp
	mov	di,offset DGROUP:_a_tss.tss_eip
	mov	cx,32
	rep	movsw			; Restore a_tss from stack

	call	restore_registers
	lss	sp, dword ptr ds_sp

	push	ds_flg
	push	ds_cs
	push	ds_ip
	mov	ds, ds_ds
	iret				; jmp to wherever the regs said to

; This is a wrapper around their RMCB which copies the regs.  On entry:
; DS:ESI points to real mode SS:SP         AX     is RMCB type
; ES:EDI points to PM register structure   FS:EBX points to RM register struct

rmcb_task:
	mov	ecx,25			; Size of reg struct in words
	push	esi
	push	edi
	mov	esi,ebx			; Copy RM regs to PM
	rep	movs word ptr es:[edi],word ptr fs:[esi]
	pop	edi
	pop	esi
	push	ebx			; Save for return restore
	push	fs
	push	ds
	push	esi			; Real stack for us if internal int
	push	eax
; Now set up the iret frame for their call
	pushfd
	db	66h, 09ah		; Far call from 16 seg to 32 seg
rmcboff	dd	?
rmcbseg	dw	?

; They return via IRET, so flags are correct, interrupts disabled
; Restore the reg structure es:edi to ss:(stack value)
	pop	eax
	pop	esi
	pop	ds
	or	al,al
	je	short not_internal
	lods	dword ptr [esi]		; Get CS:IP from real stack in eax
	mov	dword ptr es:[edi+2ah], eax
	lods	word ptr [esi]			; Get flags
	mov	word ptr es:[edi+20h], ax
	add	word ptr es:[edi+2eh], 6	; Adjust RM SP

not_internal:
	push	es
	pop	ds
	mov	esi,edi			; ds:esi now = es:edi
	pop	es
	pop	edi			; es:edi is now reg struct
	mov	ecx,25			; Size of reg struct in words
	rep	movs word ptr es:[edi],word ptr [esi]
	xor	bx,bx			; Indicate not valid raw switch (sp=0)
	jmpt	g_ctss			; Back to real mode

	public	_oldint2f
_oldint2f	label	dword
	dw	2 dup (?)

_dpmientrysw	label	dword
	dw	dpmientry
	dw	_TEXT

	public	_cpu_family
_cpu_family	label	byte
	dw	?

	public	_init_size
_init_size 	label	word
	dw	6

	public	_dpmiint2f
_dpmiint2f:
	cmp	ax,1687h
	jne	short not_us
	mov	cl,cs:_cpu_family
	xor	ax,ax			;Yes, we are here
	mov	bx,1			;32 bit programs supported
	mov	dx,5ah			;0.90 version
	mov	si,cs:_init_size	;paragraphs needed for tss save
	les	di,cs:_dpmientrysw
	iret
not_us:
	jmp	DWORD PTR cs:_oldint2f

dpmientry:				;far return point cs:ip on stack
	test	al,1			;Bogus code since we don't do 16 bit
	jnz	short ok_32		;Bogus code
;	stc				;Bogus code - but bcc make doesn't check
;	retf				;Bogus code - carry so wedges machine !
	mov	dx,offset bogus_msg	;Bogus code
	push	cs			;Bogus code
	pop	ds			;Bogus code
	mov	ah,9			;Bogus code
	int	21h			;Bogus code
	mov	ax,4c01h		;Bogus code
	int	21h			;Bogus code
bogus_msg	db "16-bit DPMI unsupported.",13,10,"$"
ok_32:					;Bogus code
	push	ds
	push	ebx
	push	es
	pop	ds			;Real mode segment of DPMI host area
	xor	bx,bx
	pop 	[bx].tss_ebx
	pop 	[bx].tss_ds
	pop	word ptr [bx].tss_eip	;saved ip on stack from far call here
	mov	word ptr [bx+2].tss_eip,bx	;Clear upper word
	pop	[bx].tss_cs		;saved cs on stack from far call here
	mov	[bx].tss_eax,eax
	mov	[bx].tss_ecx,ecx
	mov	[bx].tss_edx,edx
	mov	word ptr [bx].tss_esp,sp ;stack clean at this point (req short)
	mov	word ptr [bx+2].tss_esp,bx	;Clear upper word
	mov	[bx].tss_ebp,ebp
	mov	[bx].tss_esi,esi
	mov	[bx].tss_edi,edi
;	mov	[bx].tss_es,es		;not used
	mov	[bx].tss_ss,ss
	push	DGROUP			;setup our DS so we can setup tss
	pop	ds

; Their state is now stored.  Finish switching to our state
	lss	sp,dword ptr _DPMIsp
	cld
	call	_DPMIstartup
	push	0
	call	_go_til_stop			; never to return

; We enter this routine at user ring in protected mode after the user exception
; handler has done its far return.  It must restore the EIP/CS/EFLAGS/ESP/SS 
; from the structure on the stack (possibly modified by the user).

	public	_user_exception_return
_user_exception_return:
	add	esp,4				;Clear error code

	public	_user_interrupt_return
_user_interrupt_return:
IF run_ring EQ 0
; This code moves the EIP/CS/EFLAGS to the specified stack-12, swaps to that 
; stack, and then does an iretd.  This code doesn't work for HW interrupts 
; since it touches their stack which may not be present or may be messed up by 
; the CWS HW int to exception algorithm.  Fix me.
	push	ebp				;Save for later
	mov	ebp,esp				;Pointer to our stack
	push	eax				;Work area
	push	ebx				;Save for later
	push	ds				;Save
	lds	ebx,[ebp+16]			;Info for new SS:ESP
	sub	ebx,16				;Room for 4 dwords
	mov	[ebp+16],ebx			;Update ESP for reload
	mov	eax,[ebp]			;Saved EBP
	mov	[ebx],eax
	mov	eax,[ebp+4]			;Saved EIP
	mov	[ebx+4],eax
	mov	eax,[ebp+8]			;Saved CS
	mov	[ebx+8],eax
	mov	eax,[ebp+12]			;Saved Eflags
	mov	[ebx+12],eax
	push	g_pdata
	pop	ds
	dec	_locked_count			;we may be leaving locked stack
	pop	ds
	pop	ebx
	pop	eax
	lss	esp,[ebp+16]			;Info for new SS:ESP
	pop	ebp
	iretd
ELSE
	db	09ah				;Call intersegment to call gate
	dw	0, g_iret			;Should point to code below

; We enter here on ring 0 stack with 4 dwords.  We need 5 dwords for the iret
; structure on the old stack, which (while it is locked) is a ring 3 stack,
; maybe in the user code space.  Copy the structure here then do the IRET.

	public	_ring0_iret
_ring0_iret:
	sub	esp,4				;More room (we need EFLAGS)
	push	ebp				;Save for later
	mov	ebp,esp				;Pointer to our stack
	push	ds
	push	es
	push	esi
	push	edi
	push	ecx
	mov	ecx,5				;Dword count
	mov	edi,ebp
	add	edi,4				;Our stack address, adjusted
	push	ss
	pop	es
	lds	esi,[ebp+16]			;Old ESP:SS
	cld
	rep	movs dword ptr es:[edi],dword ptr ds:[esi]	;ecx count
	pop	ecx
	pop	edi
	pop	esi
	push	g_pdata
	pop	ds
	dec	_locked_count			;we may be leaving locked stack
	pop	es
	pop	ds
	pop	ebp
	iretd					;Restore SS:ESP, CS:EIP, EFLAGS
ENDIF

; We enter here from protected mode wanting to raw switch to real
	public	_do_raw_switch
_do_raw_switch:
	push	ebp si di
	pushf	
	push	_DPMIsp				;For nested calls
	xor	ax,ax
	mov	fs,ax
	mov	gs,ax
	mov	_DPMIsp,sp
;	mov	_DPMIss,ss
	mov	bx,_tss_ptr
	mov	ss,word ptr [bx].tss_edx
	mov	sp,word ptr [bx].tss_ebx	; high word???
	mov	es,word ptr [bx].tss_ecx
	mov	ebp,[bx].tss_ebp
	push	word ptr [bx].tss_esi		; CS
	push	word ptr [bx].tss_edi		; IP
	mov	ds,word ptr [bx].tss_eax
	retf

; We enter here from real mode wanting to switch back to protected
	public	_do_raw_switch_ret
_do_raw_switch_ret:
	push	DGROUP
	pop	ds
	lss	sp,dword ptr _DPMIsp
	push	ebx
	mov	bx,_tss_ptr
	mov	[bx].tss_fs,0
	mov	[bx].tss_gs,0
	mov	[bx].tss_ds,ax
	mov	[bx].tss_es,cx
	mov	[bx].tss_ss,dx
	pop	[bx].tss_esp
	mov	[bx].tss_cs,si
	mov	[bx].tss_eip,edi
	mov	[bx].tss_ebp,ebp
	pop	_DPMIsp
	popf
	pop 	di si ebp
	ret

; Since we don't do anything in saving state, same for both modes
	public	_savestate_real
_savestate_real:
;	push	es
;	push	0b800h
;	pop	es
;	mov	byte ptr es:[0f9ch],'R'
;	pop	es
;	retf
	public	_savestate_prot
_savestate_prot:
;	push	es
;	push	esi
;	push	g_core
;	pop	es
;	mov	esi,0b8f9eh
;	mov	byte ptr es:[esi],'P'
;	pop	esi
;	pop	es
	retf

	public	_int23
	public	_int24
_int24:
	mov	al,3
_int23:
	iret

badcpu_msg	db "80386 required.",13,10
badsize equ	$ - badcpu_msg
	public	_main
_main:
	call	_cputype
	mov	cs:_cpu_family,al
	cmp	al,3
	jb	short badcpu
	jmp	_main1
badcpu:
	push	cs
	pop	ds
	mov	dx,offset _TEXT:badcpu_msg
	mov	ah,40h
	mov	bx,2		;Standard error
	mov	cx,badsize
	int	21h
	mov	ax,4c01h
	int	21h

	end_code16

;------------------------------------------------------------------------

	end
