;------------------------------------------------------------------------------
;		F R E E - D O S		X M S - D R I V E R
;------------------------------------------------------------------------------
;
; Improvments by Martin Str”mberg.
; Copyright 2001-2005, Martin Str”mberg.
; 
; Martin can be reached at <ams@ludd.ltu.se>. Please start the
; subject line with "FDXMS286". If you have a bug report, use the bug
; reporting tool on the FreeDOS site, <http://www.freedos.org>. Make
; sure to include the FDXMS286.SYS version number.
;
; Whacked the 386 code into 286 code.
;
; Totally relying on BIOS for moves.
;
;
; The method of dispatching xms calls are worth an explanation:
;
; There are three ways a call to an XMS routine can end:
; 1. Successful, AX=1.
; 2. Error, AX=0, BL=error code.
; 3. Successful but AX and other registers contain return values.
;
; We handle case 1 and 2 by pushing an extra "return address for failure"
; before calling the routine. If it fails it pops the original return
; address (success address) and rets (this is two bytes of code size).
; If the routine is successful it rets as usual and the extra return address
; is cleaned up.
; Case 3 is handled by jmping to a third place that cleans up the two
; unused return addresses.
;
;
;------------------------------------------------------------------------------
; Written by Till Gerken for the Free-DOS project.
;
; If you would like to use parts of this driver in one of your projects, please
; check up with me first.
;
; Till can be reached at:	till@tantalo.net (Internet)
;
; Comments and bug reports are always appreciated.
;
; Copyright (c) 1995, Till Gerken
;------------------------------------------------------------------------------
; -- IMPLEMENTATION NOTES --
;
; - I didn't care about reentrancy. If this thing should be used in
;   Multitasking Environments, well, somebody got to take a look at the code.
; - Some ideas were taken from the original XMS driver written by
;   Mark E. Huss (meh@bis.adp.com), but everything has been completely
;   rewritten, so if there are bugs, they are mine, not his. ;)
;------------------------------------------------------------------------------

ideal					; switch on ideal mode syntax
jumps
P8086				; We start in 8086 mode.
	
include "fdxms.inc"			; this file contains structures,
					; constants and the like

;------------------------------------------------------------------------------
; 16-bit resident code and data
;------------------------------------------------------------------------------

segment code16
assume cs:code16, ds:code16, es:nothing

;------------------------------------------------------------------------------
; device driver header

		dd	-1			; last driver in list
		dw	8000h			; driver flags
		dw	offset strategy		; pointer to strategy routine
		dw	offset interrupt	; pointer to interrupt handler
		db	'XMSXXXX0'		; device driver name

;------------------------------------------------------------------------------
; global data

bios_gdt:
		db	0,0,0,0,0,0,0,0		; Dummy entries for BIOS.
		db	0,0,0,0,0,0,0,0		; Dummy entries for BIOS.
		dw	0ffffh			; Bits 0-15 of source segment length.
bios_src_low	dw	0			; Bits 0-15 of source address.
bios_src_middle	db	0			; Bits 16-23 of source address.
		db	93h			; Source access rights.
		db	0fh			; More type bits and bits 16-19 of source segment length.
bios_src_high	db	0			; Bits 24-31 of source address.
		dw	0ffffh			; Bits 0-15 of dest. segment length.
bios_dst_low	dw	0			; Bits 0-15 of dest. address.
bios_dst_middle	db	0			; Bits 16-23 of dest. address.
		db	93h			; Dest. access rights.
		db	0fh			; More type bits and bits 16-19 of dest segment length.
bios_dst_high	db	0			; Bits 24-31 of dest. address.
		db	0,0,0,0,0,0,0,0		; Dummy entries for BIOS.
		db	0,0,0,0,0,0,0,0		; Dummy entries for BIOS.
	

a20_locks	dw	0	; Internal A20 lock count.

flags		db	0
	
hma_min		dw	0	; Minimal space in HMA that has to be requested.
	
int15_mem	dw	0	; The amount of memory reserved for old applications.

old_int15	dd	?	; Old INT15h vector.
old_int2f	dd	?	; Old INT2fh vector.

request_ptr	dd	?	; Pointer to request struct..

xms_delay	dw	1	; Delay calls after A20 line has been toggled.
xms_num_handles	dw	32	; Number of available handles.
	
IFDEF TRACE_CODE
trace		dw	0
trace2		dw	0
ENDIF ; TRACE_CODE
	
;------------------------------------------------------------------------------
; strategy routine. is called by DOS to initialize the driver once.
; only thing to be done here is to store the address of the device driver
; request block.
; In:	ES:BX - address of request header
; Out:	nothing

proc	strategy	far
	mov	[word cs:request_ptr+2],es	; store segment addr
	mov	[word cs:request_ptr],bx	; store offset addr
	ret					; far return here!
endp	strategy

;------------------------------------------------------------------------------
; interrupt routine. called by DOS right after the strategy routine to
; process the incoming job. also used to initialize the driver.

proc	interrupt	far
	push	di ds

	lds	di, [ cs:request_ptr ]		; Load address of request header.

	cmp	[ di+request_hdr.cmd ], CMD_ISTATUS ; Input status?
	je	@@ok_done
	cmp	[ di+request_hdr.cmd ], CMD_OSTATUS ; Output status?
	je	@@ok_done
	cmp	[ di+request_hdr.cmd ], CMD_INIT ; Do we have to initialize?
	jne	@@bad_done
	test	[ cs:flags ], FLAG_INITIALISED	; Have we initialised already?
	jnz	@@bad_done
	call	initialize			; No, do it now!
	
@@done:
	pop	ds di
	ret
	
@@ok_done:	
	mov	[ di+request_hdr.status ], STATUS_DONE
	jmp	@@done

@@bad_done:	
	mov	[ di+request_hdr.status ], STATUS_ERROR or STATUS_DONE or ERROR_BAD_CMD
	jmp	@@done
endp	interrupt

P286				; Turn on 286 instructions.

;------------------------------------------------------------------------------
; just delays a bit
;	AL - destroyed.

proc	delay
@@delay_start:
	in	al, 64h
	jmp	@@delay_check
@@delay_check:
	and	al, 2
	jnz	@@delay_start
	ret
endp	delay


;------------------------------------------------------------------------------
; enables the A20 address line
;	AL - destroyed.

proc	enable_a20
	push	cx
	mov	al, 0d1h
enable_a20_out_value_patch = $-1
	out	64h, al
enable_a20_out_port_patch = $-1
enable_a20_nop_start = $
	call	delay
	mov	al, 0dfh
	out	60h, al
	call	delay
	mov	al, 0ffh
	out	64h, al
enable_a20_nop_end = $
	mov	cx, [ cs:xms_delay ]
@@delay:	
	call	delay
	loop	@@delay
	
	pop	cx
	ret
endp	enable_a20

;------------------------------------------------------------------------------
; disables the A20 address line
;	AL - destroyed.

proc	disable_a20
	push	cx
	mov	al, 0d1h
disable_a20_out_value_patch = $-1
	out	64h, al
disable_a20_out_port_patch = $-1
disable_a20_nop_start = $
	call	delay
	mov	al, 0ddh
	out	60h, al
	call	delay
	mov	al, 0ffh
	out	64h, al
disable_a20_nop_end = $
	mov	cx, [ cs:xms_delay ]
@@delay:	
	call	delay
	loop	@@delay
	
	pop	cx
	ret
endp	disable_a20

;------------------------------------------------------------------------------
; tests if the A20 address line is enabled.
; compares 256 bytes at 0:0 with ffffh:10h
; Out:	ZF=0 - A20 enabled
;	ZF=1 - A20 disabled

proc	test_a20
	push	cx si di ds es

	xor	si, si
	mov	ds, si
	push	0ffffh
	pop	es
	mov	di, 10h

	mov	cx,100h/2
	rep	cmpsw

	pop	es ds di si cx
	ret
endp	test_a20

;------------------------------------------------------------------------------
; Checks if VDISK is already installed.
; In:	CS=DS
; Out:	ZF=0 -> VDISK not installed or HMA is allocated.
;	ZF=1 -> VDISK is installed.
;	DI - destroyed.

vdisk_id	db	VDISK_ID_STR

proc	check_vdisk
	push	ax cx si ds es

	test	[ flags ], FLAG_HMA_USED
	jnz	@@done

	push	0			; Get interrupt vector 19h segment.
	pop	ds
	mov	es, [ 19h*4+2 ]
	push	cs
	pop	ds

	;; INT19h check.
	mov	di, VDISK_ID_INT19_OFS
	mov	si, offset vdisk_id
	mov	cx, VDISK_ID_LEN
	rep	cmpsb			; Is VDISK here?
	jz	@@done

	;; HMA check.
	call	test_a20
	lahf				; Save ZF in AH.
	jnz	@@a20_is_on
	
	call	enable_a20

@@a20_is_on:	
	push	0ffffh
	pop	es
	mov	di, VDISK_ID_HMA_OFS
	mov	si, offset vdisk_id
	mov	cx, VDISK_ID_LEN
	rep	cmpsb
	pushf
	sahf				; Get ZF back.
	jnz	@@a20_was_on
	
	call	disable_a20

@@a20_was_on:	
	popf

@@done:	
	pop	es ds si cx ax
	ret
endp	check_vdisk

;------------------------------------------------------------------------------
; Interrupt handlers
;------------------------------------------------------------------------------

;------------------------------------------------------------------------------
; New INT15h handler.

proc	int15_handler
	cmp	ah,87h				; is it a block move request?
	je	@@do_move
	cmp	ah,88h				; is it a ext. mem size req.?
	je	@@ext_mem_size
	jmp	[cs:old_int15]			; jump to old handler
	
@@do_move:
	call	test_a20			; Check if A20 is on or off.
	jz	@@a20disabled
	or	[ cs:flags ], FLAG_A20_STATE	; Remeber A20 on.
	jmp	@@call_old_mover
@@a20disabled:
	and	[ cs:flags ], not FLAG_A20_STATE	; Remember A20 off.
@@call_old_mover:
	pushf			; Simulate INT call.
	call	[ cs:old_int15 ]	
	pushf			; Save flags for return.
	push	ax
	test	[ cs:flags ], FLAG_A20_STATE	; See what state A20 should be in.
	jz	@@disable_it
	call	enable_a20
	jmp	@@move_done
@@disable_it:
	call	disable_a20
@@move_done:
	pop	ax
	popf
	jmp	@@exit
	
@@ext_mem_size:
	mov	ax, [ cs:int15_mem ] ; Fill in what is left over.
	clc			; No error.
	sti			; Enable interrupts.

@@exit:	
	retf	2		; Perhaps there is a use for ret N after all?
endp	int15_handler

;------------------------------------------------------------------------------
; New INT2Fh handler. Catches functions 4300h and 4310h.

proc	int2f_handler
	cmp	ax, 4300h			; Is it "Installation Check"?
	jne	@@driver_address
	mov	al, 80h				; Yes, we are installed.
	iret
@@driver_address:
	cmp	ax, 4310h			; Is it "Get Driver Address"?
	jne	@@chain_old2f
	push	cs
	pop	es
	mov	bx, offset xms_dispatcher
	iret
@@chain_old2f:
	jmp	[ cs:old_int2f ]		; Jump to old handler.
endp	int2f_handler

;------------------------------------------------------------------------------
; XMS functions
;------------------------------------------------------------------------------

;------------------------------------------------------------------------------
; returns XMS version number
; In:	AH=0
; Out:	AX=XMS version number
;	BX=internal revision number
;	DX=1 if HMA exists, 0 if not

proc	xms_get_version
	mov	ax, INTERFACE_VER
	mov	bx, DRIVER_VER
	mov	dx, 1				; HMA always exists.
	jmp	xms_exit_jmp
endp	xms_get_version

;------------------------------------------------------------------------------
; requests HMA
; In:	AH=1
;	DX=space needed in HMA (0ffffh if application tries to request HMA)
; Out:	AX=1 if successful
;	AX=0 if not successful
;	  BL=80h -> function not implemented (implemented here ;) )
;	  BL=81h -> VDISK is detected
;	  BL=90h -> HMA does not exist
;	  BL=91h -> HMA already in use
;	  BL=92h -> DX less than HMA_MIN

proc	xms_request_hma
	test	[ flags ], FLAG_HMA_USED	; Is HMA already used?
	mov	bl, XMS_HMA_IN_USE
	jnz	xrh_err
	cmp	dx,[hma_min]			; is request big enough?
	mov	bl,XMS_HMAREQ_TOO_SMALL
	jb	xrh_err
	or	[ flags ], FLAG_HMA_USED	; Assign HMA to caller.
	retn
xrh_err:
	pop	ax
	retn
endp	xms_request_hma

;------------------------------------------------------------------------------
; releases HMA
; In:	AH=2
; Out:	AX=1 if successful
;	AX=0 if not successful
;	  BL=80h -> function not implemented
;	  BL=81h -> VDISK is detected
;	  BL=90h -> HMA doesn't exist
;	  BL=93h -> HMA wasn't allocated

proc	xms_release_hma
	test	[ flags ], FLAG_HMA_USED		; Is HMA used?
	mov	bl, XMS_HMA_NOT_USED
	jz	xrh_err
	and	[ flags ], not FLAG_HMA_USED	; Now release it.
	retn
endp	xms_release_hma

;------------------------------------------------------------------------------
; global A20 address line enable
; In:	AH=3
; Out:	AX=1 if successful
;	AX=0 if not successful
;	  BL=80h -> function is not implemented
;	  BL=81h -> VDISK is detected
;	  BL=82h -> A20 failure

proc	xms_global_enable_a20
	call	enable_a20			; enable A20
	call	test_a20			; is it really enabled?
	jz	xge_a20_err
	retn
xge_a20_err:
	mov	bl, XMS_A20_FAILURE
	pop	ax
	retn
endp	xms_global_enable_a20

;------------------------------------------------------------------------------
; global A20 address line disable
; In:	AH=4
; Out:	AX=1 if successful
;	AX=0 if not successful
;	  BL=80h -> function is not implemented
;	  BL=81h -> VDISK is detected
;	  BL=82h -> A20 failure
;	  BL=84h -> A20 still enabled

proc	xms_global_disable_a20
	call	disable_a20			; disable A20
	call	test_a20			; is it really disabled?
	jnz	xge_a20_err
	retn
endp	xms_global_disable_a20

;------------------------------------------------------------------------------
; enables A20 locally
; In:	AH=5
; Out:	AX=1 if A20 is enabled, 0 otherwise
;	BL=80h -> function not implemented
;	BL=81h -> VDISK is detected
;	BL=82h -> A20 failure

proc	xms_local_enable_a20
	inc	[a20_locks]			; increase lock counter
	call	enable_a20			; enable it
	call	test_a20			; test if it's really enabled
	jz	xge_a20_err
	retn
endp	xms_local_enable_a20

;------------------------------------------------------------------------------
; disables A20 locally
; In:	AH=6
; Out:	AX=1 if A20 is disabled, 0 otherwise
;	BL=80h -> function not implemented
;	BL=81h -> VDISK is detected
;	BL=82h -> A20 failure

proc	xms_local_disable_a20
	dec	[a20_locks]			; decrease lock counter
	jnz	@@xld_dont_disable		; disable only if needed
	call	disable_a20			; disable it
	call	test_a20			; test if it's really disabled
	jnz	xge_a20_err
@@xld_dont_disable:
	retn
endp	xms_local_disable_a20

;------------------------------------------------------------------------------
; returns the state of A20
; In:	AH=7
; Out:	AX=1 if A20 is physically enabled, AX=0 if not
;	BL=00h -> function was successful
;	BL=80h -> function is not implemented
;	BL=81h -> VDISK is detected

proc	xms_query_a20
	xor	ax,ax			; suppose A20 is disabled
	call	test_a20
	jz	@@xqa_a20dis
	mov	ax,1
@@xqa_a20dis:
	xor	bl,bl
	jmp	xms_exit_jmp
endp	xms_query_a20

;------------------------------------------------------------------------------
; searches a/next free XMS memory block
; In:	DS=CS
;	BX - offset of start handle (if search is continued)
;	CX - remaining handles (if search is continued)
; Out:	CY=1 - no free block
;	  BX - offset of end of handle table
;	CY=0 - free block found
;	  BX - offset of free handle
;	  CX - number of remaining handles

proc	find_free_block
	mov	bx,offset driver_end	; start at the beginning of the table
	mov	cx,[xms_num_handles]	; check all handles
@@find_free_block:
	cmp	[bx+xms_handle.used],0	; is it used?
	jnz	find_next_free_block	; yes, go on
	cmp	[bx+xms_handle.xbase],0	; assigned memory block or just blank?
	jnz	@@found_block		; assigned, return it
find_next_free_block:
	add	bx,size xms_handle	; skip to next handle
	loop	@@find_free_block	; check next handle
	stc				; no free block found, error
	ret
@@found_block:
	clc				; no error, return
	ret
endp	find_free_block

;------------------------------------------------------------------------------
; searches a/next free XMS memory handle
; In:	DS=CS
;	BX - offset of start handle (if search is continued)
;	CX - remaining handles (if search is continued)
; Out:	CY=1 - no free handle
;	  BX - offset of end of handle table
;	CY=0 - free handle found
;	  BX - offset of free handle
;	  CX - number of remaining handles

proc	find_free_handle
	mov	bx,offset driver_end	; start at the beginning of the table
	mov	cx,[xms_num_handles]	; check all handles
@@find_free_handle:
	cmp	[bx+xms_handle.used],0		; is it used?
	jnz	find_next_free_handle		; yes, go on
	cmp	[bx+xms_handle.xbase],0		; really blank handle?
	jz	@@found_handle			; found a blank handle
find_next_free_handle:
	add	bx,size xms_handle	; skip to next handle
	loop	@@find_free_handle	; check next handle
	stc				; no free block found, error
	ret
@@found_handle:
	clc				; no error, return
	ret
endp	find_free_handle

;------------------------------------------------------------------------------
; Verifies that a handle is valid and in use.
; In:	DX - handle to verify
; Out:	CY=1 - not a valid handle
;	  BL=0xa2 - XMS_INVALID_HANDLE
;	  AX=0 - Error return.
;	CY=0 - valid handle
;	  AX=modified

	
proc	is_handle_valid
	mov	ax, dx
	sub	ax, offset driver_end		; Is the handle below start of handles?
	jb	@@not_valid
	
	push	dx
	push	bx
	xor	dx, dx
	mov	bx, size xms_handle
	div	bx
	test	dx, dx				; Is the handle aligned on a handle boundary?
	pop	bx
	pop	dx
	jnz	@@not_valid
	
	cmp	ax, [ cs:xms_num_handles ]	; Is the handle number less than the number of handles?
	jae	@@not_valid
	
	; If we come here, the handle is a valid handle
	mov	ax, bx
	mov	bx, dx
	cmp	[ cs:bx+xms_handle.used ], 1	; Is the handle in use?
	mov	bx, ax
	jb	@@not_valid

	; Handle is valid.
	clc
	ret
	
@@not_valid:
	; Handle is not valid
	mov	bl, XMS_INVALID_HANDLE
	xor	ax, ax
	stc
	ret
endp	is_handle_valid
	
;------------------------------------------------------------------------------
; returns free XMS
; In:	AH=8
; Out:	AX=size of largest free XMS block in kbytes
;	DX=total amount of free XMS in kbytes
;	BL=0 if ok
;	BL=080h -> function not implemented
;	BL=081h -> VDISK is detected
;	BL=0a0h -> all XMS is allocated

proc	xms_query_free_xms
	push	bx cx

	mov	cx, [ xms_num_handles ]
	mov	bx, offset driver_end
	
	xor	ax, ax				; Contains largest free block.
	xor	dx, dx				; Contains total free XMS.

@@check_next:
	cmp	[ bx+xms_handle.used], 0
	jne	@@in_use
	
	mov	di, [ bx+xms_handle.xsize ]	; Get size.
	add	dx, di				; Update total amount.
	cmp	di, ax				; Check if larger than largest.
	jbe	@@not_larger
	
	mov	ax, di				; Larger, update.
	
@@in_use:		
@@not_larger:
	add	bx, size xms_handle
	loop	@@check_next

	pop	cx bx
	test	ax, ax				; Is there any free memory?
	jz	@@no_free_xms
	
	xor	bl,bl
	jmp	xms_exit_jmp

@@no_free_xms:
	mov	bl, XMS_ALREADY_ALLOCATED
	pop	ax
	retn
endp	xms_query_free_xms

;------------------------------------------------------------------------------
; allocates an XMS block
; In:	AH=9
;	DX=amount of XMS being requested in kbytes
; Out:	AX=1 if successful
;	  DX=handle
;	AX=0 if not successful
;	  BL=080h -> function not implemented
;	  BL=081h -> VDISK is detected
;	  BL=0a0h -> all XMS is allocated
;	  BL=0a1h -> no free handles left

proc	xms_alloc_xms
	push	bx cx

	call	find_free_block		; See if there's a free block.
	jc	@@no_free_memory	; If there isn't fail.
	jmp	@@check_size
	
@@get_next_block:
	call	find_next_free_block
	jc	@@no_free_memory
@@check_size:
	cmp	dx,[bx+xms_handle.xsize]	; check if it's large enough
	ja	@@get_next_block			; no, get next block

	mov	di,bx			; save handle address
	inc	[bx+xms_handle.used]	; this block is used from now on
	
@@find_handle:	
	call	find_free_handle	; see if there's a blank handle
	jc	@@perfect_fit		; no, there isn't, alloc all mem left
	mov	ax,[di+xms_handle.xsize]	; get size of old block
	sub	ax,dx				; calculate resting memory
	jz	@@perfect_fit			; if it fits perfectly, go on
	mov	[bx+xms_handle.xsize],ax	; store sizes of new blocks
	mov	[di+xms_handle.xsize],dx
	mov	ax,[di+xms_handle.xbase]	; get base address of old block
	add	ax,dx				; calculate new base address
	mov	[bx+xms_handle.xbase],ax	; store it in new handle

@@perfect_fit:
	mov	dx, di				; Return handle in DX.

	pop	cx bx
	retn
	
@@no_free_memory:
	;; If no memory was asked for, just allocate a handle.
	test	dx, dx
	jz	@@zero_size_allocation

	mov	al, XMS_ALREADY_ALLOCATED
	jmp	@@failure_epilogue

@@zero_size_allocation:
	call	find_free_handle	; see if there's a blank handle
	jc	@@no_handles_left 	; No, There isn't, fail.

	mov	di, bx			; Save handle address.

	;; We have the handle. Mark it as used.
	inc	[ bx + xms_handle.used ]
	jmp	@@perfect_fit

@@no_handles_left:	
	mov	al, XMS_NO_HANDLE_LEFT

@@failure_epilogue:		
	pop	cx bx
	mov	bl, al
	pop	ax
	retn
endp	xms_alloc_xms

;------------------------------------------------------------------------------
; frees an XMS block
; In:	AH=0ah
;	DX=handle to allocated block that should be freed
; Out:	AX=1 if successful
;	AX=0 if not successful
;	  BL=080h -> function not implemented
;	  BL=081h -> VDISK is detected
;	  BL=0a2h -> handle is invalid
;	  BL=0abh -> handle is locked

proc	xms_free_xms
	call	is_handle_valid
	jc	@@xms_free_not_valid
	
	push	bx cx dx

	mov	si, dx				; Get handle into SI.

	xor	dx, dx				; Set to zero for cmp and mov below.

	cmp	[ si+xms_handle.used ], 1	; Is the block locked?
	jne	@@locked			; Yes, bail out.

	cmp	[ si+xms_handle.xsize ], dx	; Is it a zero-length handle?
	je	@@zero_handle
	
@@xms_free_loop:
	mov	di, [ si+xms_handle.xbase ]	; Get base address.
	add	di, [ si+xms_handle.xsize ]	; Calculate end-address.
	
	call    find_free_block			; Check free blocks.
	jc	@@xms_free_done			; No, was last handle.
	
@@try_concat:
	cmp	di, [ bx+xms_handle.xbase ]	; Is it adjacent to old block?
	jne	@@try_concat_2
	
	mov	ax, [ bx+xms_handle.xsize ]	; Concat.
	add	di, ax
	add	[ si+xms_handle.xsize ], ax
	
	mov	[ bx+xms_handle.xbase ], dx	; Blank handle.
	mov	[ bx+xms_handle.xsize ], dx

	jmp	@@next
	
@@try_concat_2:
	mov	ax, [ bx+xms_handle.xbase ]	; Is it adjacent to old block?
	add	ax, [ bx+xms_handle.xsize ]
	cmp	ax, [ si+xms_handle.xbase ]
	jne	@@not_adjacent

	inc	[ bx+xms_handle.used ]		; This one in use temporarily.
	mov	ax, [ si+xms_handle.xsize ]	; Concat.
	add	[ bx+xms_handle.xsize ], ax
	mov	[ si+xms_handle.xbase ], dx	; Blank handle.
	mov	[ si+xms_handle.xsize ], dx
	mov	[ si+xms_handle.used], dl	; Not in use anymore.
	mov	si, bx
	jmp	@@xms_free_loop

@@next:	
@@not_adjacent:
	call	find_next_free_block		; See if there are other blks.
	jnc	@@try_concat
	
@@xms_free_done:
	mov	[ si+xms_handle.used ], dl	; Handle isn't used anymore.
	pop	dx cx bx
	retn

@@zero_handle:
	mov	[ si+xms_handle.xbase ], dx	; Blank handle.
	jmp	@@xms_free_done
	
@@locked:	
	pop	dx cx bx
	mov	bl, XMS_BLOCK_LOCKED
	
@@xms_free_not_valid:
	pop	ax
	retn
endp	xms_free_xms

;------------------------------------------------------------------------------
; calculates the move address
; In:	BX - handle (0 if DX:AX should be interpreted as seg:ofs value)
;	DX - high 16 bits of offset
;	AX - low 16 bits of offset
; Out:	DX - high 16 bits of absolute move address
;	AX - low 16 bits of absolute move address
;	CY=1 - not valid handle.
;	CY=0 - valid handle.
;	  
; Modifies: CX

proc	get_move_addr
	test	bx, bx			; translate address in DX?
	jnz	@@dont_translate
	mov	cx, dx			; Calculate the high bits.
	shr	cx, 12
	shl	dx, 4			; convert segment to absolute address
	add	ax, dx			; add offset
	adc	cx, 0			; Adjust for overflow.
	mov	dx, cx
	jmp	@@no_handle
	
@@dont_translate:
	push	ax dx
	mov	dx, bx
	call	is_handle_valid
	pop	dx ax
	jc	@@not_valid

	mov	cx, [ cs:bx+xms_handle.xbase ]	; get block base address
	push	cx
	shl	cx,10				; convert from kb to absolute
	add	ax, cx
	pop	cx
	pushf
	shr	cx, 6
	popf
	adc	dx, cx
	
@@no_handle:
	
@@not_valid:	
	ret
endp	get_move_addr

;------------------------------------------------------------------------------
; moves an XMS block
; In:	AH=0bh
;	DI:SI=pointer to XMS move structure
; Out:	AX=1 if successful
;	AX=0 if not successful
;	  BL=080h -> function not implemented
;	  BL=081h -> VDISK is detected
;	  BL=082h -> A20 failure
;	  BL=0a3h -> source handle is invalid
;	  BL=0a4h -> source offset is invalid
;	  BL=0a5h -> destination handle is invalid
;	  BL=0a6h -> destination offset is invalid
;	  BL=0a7h -> length is invalid
;	  BL=0a8h -> move has invalid overlap
;	  BL=0a9h -> parity error

proc	xms_move_xms
	push	bx cx dx bp
	push	es

	mov	ds, di			; DS = caller's DS.

	cli					; no interrupts

	call	test_a20			; get A20 state
	pushf					; save it for later
	jnz	@@a20_already_enabled	; We don't need to enable it if it's already enabled.
	call	enable_a20			; now enable it!
	
@@a20_already_enabled:	
	mov	bx,[si+xms_move_strc.dest_handle]
	mov	dx,[si+xms_move_strc.dest_offset_high]
	mov	ax,[si+xms_move_strc.dest_offset_low]
	call	get_move_addr		; get move address
	jc	@@dest_not_valid
	mov	bp, dx			; store in destination index
	mov	di, ax			; bp*2^16 + di is destination address.
	
	mov	bx,[si+xms_move_strc.src_handle]
	mov	dx,[si+xms_move_strc.src_offset_high]
	mov	ax,[si+xms_move_strc.src_offset_low]
	call	get_move_addr		; get move address
	jc	@@src_not_valid
	mov	bx, dx			; store in source index
	;; mov	si, ax	has been moved below length aquisition!
	
	mov	dx, [ si+xms_move_strc.len_high ]	; get length
	mov	cx, [ si+xms_move_strc.len_low ]	; dx*2^16+cx is length.

	mov	si, ax			; Now bx*2^16 + si is source address.
	
	test	cl,1				; Is the length even?
	jnz	@@invalid_length

	mov	ax, cx
	or	ax, dx	; Nothing to copy?
	jz	@@xms_move_ok

	cmp	bp, bx		; src == dest?
	jne	@@check_overlap

	cmp	di, si
	je	@@xms_move_ok	; Yes, nothing to do.

@@check_overlap:	
	push	cs			; Set DS to my segment.
	pop	ds

	push	cs			; Set ES to my segment.
	pop	es

	cmp	bx, bp	
	ja	@@bios_ok
	jne	@@perhaps_overlap
	
	cmp	si, di
	ja	@@bios_ok
	
@@perhaps_overlap:		
	mov	ax, si
	add	ax, cx
	push	ax
	mov	ax, bx
	adc	ax, dx
	cmp	ax, bp
	pop	ax
	ja	@@illegal_overlap
	jne	@@bios_ok

	cmp	ax, di
	ja	@@illegal_overlap

@@bios_ok:	
@@bios_move_loop:
	mov	ax, cx
	test	dx, dx
	jnz	@@use_max_length
	
	cmp	ax, 2000h
	jbe	@@length_ok
	
@@use_max_length:	
	mov	ax, 2000h

@@length_ok:
	pusha
	shr	ax, 1
	mov	cx, ax
	
	;; Fill in source entries.
	mov	[ bios_src_low ], si
	mov	ax, bx
	mov	[ bios_src_middle ], al
	mov	[ bios_src_high ], ah

	;; Fill in destination entries.
	mov	[ bios_dst_low ], di
	mov	ax, bp
	mov	[ bios_dst_middle ], al
	mov	[ bios_dst_high ], ah

	lea	si, [ bios_gdt ]

	clc
	mov	ah, 87h
	cli
	int	15h
	sti

	popa
	
	jc	@@a20_failure

	add	di, ax			; dest += copied length.
	adc	bp, 0
	add	si, ax			; src += copied length.
	adc	bx, 0
	sub	cx, ax			; length -= copied length.
	sbb	dx, 0
	mov	ax, cx
	or	ax, dx
	jnz	@@bios_move_loop

;	jmp	@@xms_move_ok
	
@@xms_move_ok:	
	popf				; get A20 state
	jnz	@@a20_was_enabled	; if it was enabled, don't disable
	call	disable_a20		; it was disabled, so restore state
	
@@a20_was_enabled:
	pop	es
	pop	bp dx cx bx			; restore everything
	retn				; Success.


@@src_not_valid:
	mov	al, XMS_INVALID_SOURCE_HANDLE
	jmp	@@xms_move_failure_end
		
@@dest_not_valid:
	mov	al, XMS_INVALID_DESTINATION_HANDLE
	jmp	@@xms_move_failure_end
	
@@a20_failure:
	mov	al, XMS_A20_FAILURE 
	jmp	@@xms_move_failure_end

@@illegal_overlap:
	mov	al, XMS_OVERLAP
	jmp	@@xms_move_failure_end

@@invalid_length:
	mov	al, XMS_INVALID_LENGTH
	;  Fall through.
	
@@xms_move_failure_end:	
	popf			; Saved a20 state.
	jnz	@@fail_a20_was_enabled	; if it was enabled, don't disable
	call	disable_a20		; it was disabled, so restore state
	
@@fail_a20_was_enabled:
	pop	es
	pop	bp dx cx bx
	mov	bl, al
	pop	ax
	retn
endp	xms_move_xms

;------------------------------------------------------------------------------
; locks an XMS block
; In:	AH=0ch
;	DX=XMS handle to be locked
; Out:	AX=1 if block is locked
;	  DX:BX=32-bit linear address of block
;	AX=0 if not successful
;	  BL=080h -> function not implemented
;	  BL=081h -> VDISK is detected
;	  BL=0a2h -> handle is invalid
;	  BL=0ach -> lock count overflow
;	  BL=0adh -> lock fails

proc	xms_lock_xms
	call	is_handle_valid
	jc	@@xms_lock_not_valid
	
	mov	bx,dx
	inc	[ bx+xms_handle.used ]		; Increase lock counter.
	jnz	@@locked_successful		; go on if no overflow
	dec	[ bx+xms_handle.used ]		; Decrease lock counter.
	mov	bl,XMS_LOCK_COUNT_OVERFLOW	; overflow, return with error
	
@@xms_lock_not_valid:	
	pop	ax
	retn
	
@@locked_successful:
	mov	dx, [bx+xms_handle.xbase]	; get block base address
	mov	bx, dx				; store LSW
	shl	bx, 10				; calculate linear address
	shr	dx, 6
	retn
endp	xms_lock_xms

;------------------------------------------------------------------------------
; unlocks an XMS block
; In:	AH=0dh
;	DX=XMS handle to unlock
; Out:	AX=1 if block is unlocked
;	AX=0 if not successful
;	  BL=080h -> function not implemented
;	  BL=081h -> VDISK is detected
;	  BL=0a2h -> handle is invalid
;	  BL=0aah -> block is not locked

proc	xms_unlock_xms
	call	is_handle_valid
	jc	@@xms_unlock_not_valid
	
	push	bx
	mov	bx, dx
	cmp	[ bx+xms_handle.used ], 1	; Check if block is locked.
	ja	@@is_locked			; Go on if true.
	pop	bx
	mov	bl, XMS_BLOCK_NOT_LOCKED
	
@@xms_unlock_not_valid:	
	pop	ax
	retn
	
@@is_locked:
	dec	[ bx+xms_handle.used ]		; Decrease lock counter.
	pop	bx
	retn
endp	xms_unlock_xms

;------------------------------------------------------------------------------
; returns XMS handle information
; In:	AH=0eh
;	DX=XMS block handle
; Out:	AX=1 if successful
;	  BH=block's lock count
;	  BL=number of free XMS handles
;	  DX=block's length in kbytes
;	AX=0 if not successful
;	  BL=080h -> function not implemented
;	  BL=081h -> VDISK is detected
;	  BL=0a2h -> handle is invalid

proc	xms_get_handle_info
	call 	is_handle_valid
	jc	@@not_valid

	push 	cx		; Save cx.

	push	dx		; Save handle for later.

	xor	ax, ax		; ax is number of free handles.

	;; Setup for loop.
	mov	bx, offset driver_end
	mov	cx, [ xms_num_handles ]
	
@@look_again:
	cmp	[ bx + xms_handle.used ], 0	; In use?
	jne	@@add_some			; Yes, go to next.
	inc	ax				; No, one more free handle.

@@add_some:
	add	bx, size xms_handle
	loop	@@look_again

	;;  Now ax contains number of free handles.
	
	pop 	bx 				; Get handle saved earlier.
	mov	dx, [ bx+xms_handle.xsize ]	; Store block size.
	mov	bh, [ bx+xms_handle.used ]	; Store lock count.
	dec	bh
	
	cmp	ax, 100h	; Make sure that we don't overflow bl.
	jb	@@less_than_256
	mov	al, 0ffh
@@less_than_256:	
	mov	bl, al				; Store number of free handles.

	pop	cx		; Restore.
	mov	ax, 1		; Success.

@@not_valid:	
	jmp	xms_exit_jmp
endp	xms_get_handle_info

;------------------------------------------------------------------------------
; reallocates an XMS block. only supports shrinking.
; In:	AH=0fh
;	BX=new size for the XMS block in kbytes
;	DX=unlocked XMS handle
; Out:	AX=1 if successful
;	AX=0 if not successful
;	  BL=080h -> function not implemented
;	  BL=081h -> VDISK is detected
;	  BL=0a0h -> all XMS is allocated
;	  BL=0a1h -> all handles are in use
;	  BL=0a2h -> invalid handle
;	  BL=0abh -> block is locked

proc	xms_realloc_xms
	call	is_handle_valid
	jc	@@xms_realloc_not_valid
	
	push	bx dx

	xchg	bx,dx
	cmp	dx,[bx+xms_handle.xsize]
	jbe	@@shrink_it

@@no_xms_handles_left:
	pop	dx bx
	mov	bl,XMS_NO_HANDLE_LEFT		; simulate a "no handle" error
@@xms_realloc_not_valid:	
	pop	ax
	retn

@@shrink_it:
	mov	di,bx
	call	find_free_handle		; get blank handle
	jc	@@no_xms_handles_left		; return if there's an error
	mov	ax,[di+xms_handle.xsize]	; get old size
	mov	[di+xms_handle.xsize],dx
	sub	ax,dx				; calculate what's left over
	jz	@@dont_need_handle		; skip if we don't need it
	add	dx,[di+xms_handle.xbase]	; calculate new base address
	mov	[bx+xms_handle.xbase],dx	; store it
	mov	[bx+xms_handle.xsize],ax	; store size
	mov	[ bx+xms_handle.used ], 0	; Block is not locked nor used.
@@dont_need_handle:
	pop	dx bx
	retn
endp	xms_realloc_xms

;------------------------------------------------------------------------------
; reallocates an UMB
; In:	AH=12h
;	BX=new size for UMB in paragraphs
;	DX=segment of UMB to reallocate
; Out:	AX=1 if successful
;	AX=0 if not successful
;	  BL=080h -> function not implemented
;	  BL=0b0h -> no UMB large enough to satisfy request
;	    DX=size of largest UMB in paragraphs
;	  BL=0b2h -> UMB segment is invalid

xms_realloc_umb:		; Not pretty but saves memory.
	;; Fall through.

;------------------------------------------------------------------------------
; requests an UMB block
; In:	AH=10h
;	DX=size of requested memory block in paragraphs
; Out:	AX=1 if successful
;	  BX=segment number of UMB
;	  DX=actual size of the allocated block in paragraphs
;	AX=0 if not successful
;	  DX=size of largest available UMB in paragraphs
;	  BL=080h -> function not implemented
;	  BL=0b0h -> only a smaller UMB are available
;	  BL=0b1h -> no UMBs are available

xms_request_umb:		; Not pretty but saves memory.
	xor	dx, dx		; No UMB memory.
	;; Fall through.

;------------------------------------------------------------------------------
; releases an UMB block
; In:	AH=11h
;	DX=segment of UMB
; Out:	AX=1 if successful
;	AX=0 if not successful
;	  BL=080h -> function not implemented
;	  BL=0b2h -> UMB segment number is invalid

proc	xms_release_umb
	mov	bl, XMS_NOT_IMPLEMENTED
	pop	ax
	retn
endp	xms_release_umb

IFDEF TRACE_CODE
trace_get_version		db	'get_version',13,10,0
trace_request_hma		db	'request_hma',13,10,0
trace_release_hma		db	'release_hma',13,10,0
trace_global_enable_a20		db	'global_enable_a20',13,10,0
trace_global_disable_a20	db	'global_disable_a20',13,10,0
trace_local_enable_a20		db	'local_enable_a20',13,10,0
trace_local_disable_a20		db	'local_disable_a20',13,10,0
trace_query_a20			db	'query_a20',13,10,0
trace_query_free_xms		db	'query_free_xms',13,10,0
trace_alloc_xms			db	'alloc_xms',13,10,0
trace_free_xms			db	'free_xms',13,10,0
trace_move_xms			db	'move_xms',13,10,0
trace_lock_xms			db	'lock_xms',13,10,0
trace_unlock_xms		db	'unlock_xms',13,10,0
trace_get_handle_info		db	'get_handle_info',13,10,0
trace_realloc_xms		db	'realloc_xms',13,10,0
trace_request_umb		db	'request_umb',13,10,0
trace_release_umb		db	'release_umb',13,10,0
trace_realloc_umb		db	'realloc_umb',13,10,0
ENDIF ; TRACE_CODE

;------------------------------------------------------------------------------
; XMS dispatcher
;------------------------------------------------------------------------------

;------------------------------------------------------------------------------
; XMS dispatcher
; In:	AH - function number
; Out:	AX=0 -> function not supported
;	else see appr. routine

xms_table	dw	xms_get_version, xms_request_hma
		dw	xms_release_hma, xms_global_enable_a20
		dw	xms_global_disable_a20, xms_local_enable_a20
		dw	xms_local_disable_a20, xms_query_a20
		dw	xms_query_free_xms, xms_alloc_xms
		dw	xms_free_xms, xms_move_xms
		dw	xms_lock_xms, xms_unlock_xms
		dw	xms_get_handle_info, xms_realloc_xms
		dw	xms_request_umb, xms_release_umb
		dw	xms_realloc_umb
	
IFDEF TRACE_CODE
trace_table	dw	trace_get_version, trace_request_hma
		dw	trace_release_hma, trace_global_enable_a20
		dw	trace_global_disable_a20, trace_local_enable_a20
		dw	trace_local_disable_a20, trace_query_a20
		dw	trace_query_free_xms, trace_alloc_xms
		dw	trace_free_xms, trace_move_xms
		dw	trace_lock_xms, trace_unlock_xms
		dw	trace_get_handle_info, trace_realloc_xms
		dw	trace_request_umb, trace_release_umb
		dw	trace_realloc_umb
ENDIF ; TRACE_CODE

proc	xms_dispatcher
	jmp	short @@dispatcher_entry
	nop					;
	nop					; guarantee hookability
	nop					;
@@dispatcher_entry:
	pushf					; Save flags.
	cld
	push	ds di si	; Save registers. But must NOT clobber si! 

	push	ds		; Prepare for caller's DS to be poped into DI.

	push	cs				; Setup DS.
	pop	ds

	cmp	ah,12h				; is it a supported function?
	ja	@@not_supported
	call	check_vdisk			; is VDISK installed?
	jz	@@vdisk_installed
hook_patch:
	jmp	short @@hook_ints
@@hook_return:
	mov	al, ah
	xor	ah, ah				; Mask away upper half.
	mov	di, ax

IFNDEF TRACE_CODE
	shl	di, 1
ELSE
	push	bx cx
	mov	cx, di	; Put function number in cl for trace shift check.
	shl	di, 1
	
	;; Check if a trace is wanted.
	mov	ax, 1
	mov	bx, offset trace
	test	cl, 0f0h
	jz	@@trace_check
	
	sub	cl, 10h
	mov	bx, offset trace2
	
@@trace_check:	
	shl	ax, cl
	test	[ bx ], ax
	jz	@@no_trace			; No trace wanted.
	
	mov	ax, [ trace_table + di ]
	call	print_string

@@no_trace:
	pop	cx bx
ENDIF ; TRACE_CODE
	
	mov	ax, [ xms_table + di ]
	pop	di				; DI = caller's DS.
	push	offset xms_exit_nok		; Failure return address.
	call	ax
	;; If the xms call is successful it retn here.
	pop	ax		; Clean up failure return address.
	mov	ax, 1		; All is well...

	;; The final cleanup of all xms calls.
@@xms_exit:				
	pop	si di ds			; Restore registers.
	popf					; flags should be forbidden
	retf

	;; A few xms calls jmp here to finish.
xms_exit_jmp:
	pop	di		; Clean up the return addresses.
	pop	di
	jmp	@@xms_exit
		
@@not_supported:
	mov	bl, XMS_NOT_IMPLEMENTED
	jmp 	short @@xms_exit_nok_pop_ds

@@vdisk_installed:
	mov	bl, XMS_VDISK_DETECTED

@@xms_exit_nok_pop_ds:
	pop	ax		; Throw away caller's DS.

	;; All xms calls that fails retn here.
xms_exit_nok:	
	xor	ax, ax
	jmp	short @@xms_exit

@@hook_ints:
	test	ah, ah				; non-version call?
	jz	@@hook_return			; no, don't patch

	push	ax bx dx es			; Save registers.
	mov	ax, 3515h			; Get INT15h vector.
	int	21h
	mov	[ word old_int15+2 ], es
	mov	[ word old_int15 ], bx

	mov	ax, 2515h			; Install own INT15h handler.
	mov	dx, offset int15_handler
	int	21h
	
	mov	[ word hook_patch ], 9090h	; Insert two NOPs.
	pop	es dx bx ax			; Restore registers.

	jmp	@@hook_return			; and finish it
endp	xms_dispatcher

IFDEF TRACE_CODE
include "print_string.inc"
ENDIF ; TRACE_CODE
		
;------------------------------------------------------------------------------
; mark for the driver end. above has to be the resident part, below the
; transient.

label	driver_end	near

;------------------------------------------------------------------------------
; 16-bit transient code and data. only used once.
;------------------------------------------------------------------------------

;------------------------------------------------------------------------------
; checks if CPU is a 286
; In:	nothing
; Out:	CY=0 - processor is a 286 or higher
;	CY=1 - processor lower than 286
;	AX - destroyed.
	
P8086				; This part must be able to run on 8086.
proc	check_cpu
	push	sp
	pop	ax
	cmp	ax, sp
	jne	@@not286
	clc
	ret
@@not286:
	stc
	ret
endp	check_cpu
P286					; Back to 286.
	

;------------------------------------------------------------------------------
; checks if A20 can be enabled and disabled
; Out:	CF=0, ZF=1 - A20 switching works
;	CF=0, ZF=0 - Turning on A20 worked but can't turn it off.
;	CF=1 - A20 failure

proc	check_a20
	call	enable_a20
	call	test_a20			; TEST_A20 should return ZF=0
	jz	@@a20failed
	call	disable_a20
	call	test_a20			; TEST_A20 should return ZF=1
	clc
	ret
@@a20failed:
	stc
	ret
endp	check_a20


;------------------------------------------------------------------------------
; initializes the driver. called only once!
; may modify DI
; In:	ES:DI - pointer to init structure

init_message		db	13,10,80 dup (0c4h),'FreeDOS XMS-Driver for 80286',13,10
			db	'Copyright (c) 1995, Till Gerken',13,10
			db	'Copyright 2001-2005, Martin Str”mberg',13,10,13,10
			db	'Driver Version: ',DRIVER_VERSION,9
			db	'Interface Version: ',INTERFACE_VERSION,13,10
			db	'Information: ',INFO_STR,13,10,'$'

old_dos			db	'XMS needs at least DOS version 3.00.$'
xms_twice		db	'XMS is already installed.$'
vdisk_detected		db	'VDISK has been detected.$'
no_286			db	'At least a 80286 is required.$'
a20_error		db	'Unable to switch on A20 address line.$'
a20_not_off		db      'Unable to switch off A20 address line.',13,10,'$'
xms_sizeerr		db	'Unable to determine size of extended memory.$'
xms_toosmall		db	'Extended memory is too small or not available.$'
int15_size_no_xms	db	'INT15 value clamped down to the size of the extended memory block that starts',13, 10
			db      'at 0x110000 minus 1.',13,10,'$'
	
error_msg		db	' Driver won''t be installed.',7,13,10,'$'

init_finished		db	80 dup (0c4h),13,10,'$'

command_line		db	XMS_COMMAND_LINE_LENGTH_MAX dup (0), 5 dup (0)
	
proc	initialize
P8086				; This part must be able to run on 8086.
	pushf
	push	es ax bx cx dx si

	push	ds		; Put DS into ES.
	pop	es
	push	cs		; Put my segment into DS.
	pop	ds
	
	cld

	mov	ah,9				; first, welcome the user!
	mov	dx,offset init_message
	int	21h

	mov	ax,3000h			; get DOS version number
	int	21h
	xchg	ah,al				; convert to BSD
	cmp	ax,300h				; we need at least 3.00
	mov	dx,offset old_dos
	jb	@@error_exit

	mov	ax,4300h			; check if XMS is already
	int	2fh				; installed
	cmp	al,80h
	mov	dx,offset xms_twice
	je	@@error_exit

	call	check_cpu			; do we have at least a 386?
	mov	dx,offset no_286
	jc	@@error_exit
	
P286				; Now we know we have at least a 286.
	
	;; Check command line. It's very important this is done early
	;; as it modifies A20 line toggling.
	les	ax, [ es:di+init_strc.cmd_line ]
	call	copy_command_line
	call	interpret_command_line
		
	call	check_a20			; Check A20.
	mov	dx, offset a20_error
	jc	@@error_exit

	jz	@@a20_off

	;; We managed to turn it on (already on?) but not turning it off.
	mov	ah, 9
	mov	dx, offset a20_not_off
	int	21h

@@a20_off:	
	call	check_vdisk			; Is VDISK installed?
	mov	dx, offset vdisk_detected
	jz	@@error_exit

	;; SI points to start of XMS handle table.
	mov	si, offset driver_end

	;; Find memory.
	clc
	mov	ah,88h				; extended memory size
	int	15h
	mov	dx,offset xms_sizeerr
	jc	@@error_exit

	mov	dx,offset xms_toosmall
	sub	ax,64				; Save HIMEM area.
	jc	@@error_exit			; If there aren't 64Ki,
						; there's nothing to do.

	cmp	ax, XMS_MAX		; Watch out for overflow in addresses.
	jbe	@@fill_first_entry

	mov	ax, XMS_MAX

@@fill_first_entry:
	mov	[ si+xms_handle.xbase ], XMS_START
	mov	[ si+xms_handle.xsize ], ax
	mov	[ si+xms_handle.used ], 0
	add	si, size xms_handle

	;; Save memory size.
	push	ax

	;; Any INT15 reserved memory?
	mov	ax, [ int15_mem ]
	test	ax, ax
	jz	@@print_memory_size	; No INT15 reservation.

	mov	si, offset driver_end	; SI points to first XMS handle.
	sub	ax, 64
	cmp	ax, [ si+xms_handle.xsize ]
	jb	@@int15_size_ok

@@int15_size_nok:
	mov	ah, 9
	mov	dx, offset int15_size_no_xms
	int	21h
	
	mov	ax, [ si+xms_handle.xsize ]
	dec	ax
	cmp	ax, INT15_MAX
	jbe	@@size_below_int15_max

	mov	ax, INT15_MAX

@@size_below_int15_max:	
	mov	[ int15_mem ], ax
	
@@int15_size_ok:	
	add	[ si+xms_handle.xbase ], ax
	sub	[ si+xms_handle.xsize ], ax

@@print_memory_size:	
	;; Put size of XMS in bytes in DX:AX.
	pop	ax
	mov	dx, ax
	shl	ax, 10
	shr	dx, 6

	;; Print out how much memory we've found.
	push	ax dx
	call	print_hex_number
	mov	ah, 9
	mov	dx, offset xms_mem_found_middle
	int	21h
	pop	dx ax
	call	print_dec_number
	mov	ah, 9
	mov	dx, offset xms_mem_found_post
	int	21h

	mov	ax, 352fh			; Get INT2Fh vector.
	int	21h
	mov	[ word old_int2f+2 ], es
	mov	[ word old_int2f ], bx

	mov	ax,252fh			; install own INT2Fh
	mov	dx,offset int2f_handler
	int	21h

	les	di, [ request_ptr ]
	mov	[word es:di+2+init_strc.end_addr],cs	; set end address
	mov	ax, [ xms_num_handles ]
	imul	ax, ax, size xms_handle			; Worst case 1024*5.
	add	ax,offset driver_end
	mov	[ word es:di+init_strc.end_addr ],ax
	mov	[ word es:di+2+init_strc.cmd_line ], 0	; RBIL says this should
	mov	[ word es:di+init_strc.cmd_line ], 0	; be zeroed.
	mov	[ es:di+request_hdr.status ], STATUS_DONE ; We're alright!

	or	[ flags ], FLAG_INITIALISED	; Mark that we have initialised.

	;; Finish printing.
	mov	ah,9
	mov	dx,offset init_finished
	int	21h

	;; Zero out empty handles.
	;; First calculate how much to zero.
	mov	ax, [word es:di+init_strc.end_addr]
	mov	cx, ax
	sub	ax, offset driver_end	; AX=size of handle array.
	sub	ax, size xms_handle	; There's only one used handle.
	xchg	ax, cx			; CX=bytes to zero out; AX=end of driver.

	;; Setup registers for zeroing.
	push	ds
	pop	es
	mov	di, offset driver_end + size xms_handle ; Skip the one used handle.
	
	;; Now check if we need to relocate.
	cmp	ax, offset @@zero_out_code_start
	jbe	@@zero_out_code_start

	;; Need to relocate.
	push	cx di

	std
	mov	cx, @@zero_out_code_end - @@zero_out_code_start
	mov	si, offset @@zero_out_code_end - 1
	mov	di, ax
	dec	di
	add	di, cx
	rep	movsb
	cld

	pop	di cx

	jmp	ax

@@zero_out_code_start:
	xor	al, al
	rep	stosb

P8086				; This part must be able to run on 8086.
@@exit:
	pop	si dx cx bx ax es
	popf
	ret
@@zero_out_code_end:	
	
@@error_exit:
	les	di, [ request_ptr ]
	mov	[word es:di+2+init_strc.end_addr],cs	; set end address
	mov	[word es:di+init_strc.end_addr],0	; now, we don't use
							; any space
	mov	[ word es:di+2+init_strc.cmd_line ], 0	; RBIL says this should
	mov	[ word es:di+init_strc.cmd_line ], 0	; be zeroed.
	mov	[ es:di+request_hdr.status ], STATUS_ERROR or STATUS_DONE ; Waaaah!
	mov	ah,9					; print msg
	int	21h
	mov	dx,offset error_msg
	int	21h
	mov	ah,9
	mov	dx,offset init_finished
	int	21h
	jmp	@@exit
P286				; Back to 286.
endp	initialize

;------------------------------------------------------------------------------
; Prints a hexadecimal number, using INT21, AH=0x9.
; In:	DX:AX - the number to be printed.
;	DS - initialized with code segment
number	db	'0x????????$'
proc	print_hex_number
	push	ax bx cx dx ax
	mov	bx, offset number + 2
	mov	cx, 0000h + 12	; CH == 0 first time in loop.

@@loop:	
	mov	ax, dx
	shr	ax, cl
	and	al, 0fh
	cmp	al, 0ah
	jb	@@digit
	add	al, 'A'-'0'-10
@@digit:
	add	al, '0'
	mov	[ bx ], al
	inc	bx
	sub	cl, 4
	jb	@@end?
	jmp	@@loop

@@end?:
	test	ch, ch
	jnz	@@print
	pop	dx
	mov	cx, 0ff00h + 12	; CH != 0 for the second time in loop.
	jmp	@@loop
	
@@print:
	mov	dx, offset number
	mov	ah, 9
	int	21h
	
	pop	dx cx bx ax
	ret
endp	print_hex_number

	
;------------------------------------------------------------------------------
; Prints a decimal number, using INT21, AH=0x9.
; In:	DX:AX - the number to be printed.
;	DS - initialized with code segment
struc	long
  high		dw	?		; High bits of long.
  low		dw	?		; Low bits of long.
ends	long

ten_array:	long	<  3b9ah, 0ca00h >
		long	<   5f5h, 0e100h >
		long	<    98h,  9680h >
		long	<    0fh,  4240h >
		long	<     1h,  86a0h >
		long	<     0h,  2710h >
		long	<     0h,   3e8h >
		long	<     0h,    64h >
		long	<     0h,    0ah >
		long	<     0h,     1h >

dec_number	db	'4294967296$'
	
proc	print_dec_number
	push	ax bx cx dx si
	mov	bx, offset dec_number
	mov	si, offset ten_array
	mov	cx, 10*100h
	
@@loop:
	cmp	dx, [ si + long.high ]
	jbe	@@equal?

@@subtract:	
	inc	cx
	sub	ax, [ si + long.low ]
	sbb	dx, [ si + long.high ]
	jmp	@@loop

@@equal?:
	jne	@@next
	cmp	ax, [ si + long.low ]
	jb	@@next

	jmp	@@subtract

@@next:
	add	cl, '0'
	mov	[ bx ], cl
	inc	bx
	xor	cl, cl

	dec	ch
	jz	@@print
		
	add	si, size long
	jmp	@@loop

@@print:
	mov	[ byte bx ], '$'
	mov	bx, offset dec_number - 1
	
@@zero?:	
	inc	bx
	cmp	[ byte bx ], '0'
	je	@@zero?

	cmp	[ byte bx ], '$'	; At end?
	jne	@@not_null
	
	dec	bx

@@not_null:	
	mov	dx, bx
	mov	ah, 9
	int	21h
	
	pop	si dx cx bx ax
	ret
endp	print_dec_number

IFNDEF TRACE_CODE
include "print_string.inc"
ENDIF ; TRACE_CODE
		
;------------------------------------------------------------------------------
; Saves a string pointed to by es:ax into command_line. 
; command_line is truncated to a length of 255 bytes and upper cased.
; In:	ES:AX - pointer to string.
; Out:	AL - destroyed.

proc	copy_command_line
	push	di si cx

	mov	cx, XMS_COMMAND_LINE_LENGTH_MAX
	mov	si, ax
	mov	di, offset command_line
@@loop:
	mov	al, [ es:si ]
	cmp	al, 'a'
	jb	@@do_move
	cmp	al, 'z'
	ja	@@do_move
	;; Must be a lower case letter
	add	al, 'A'-'a'	; which now is uppercase.
@@do_move:	
	mov	[di], al
	dec	cx
	jcxz	@@too_long
	inc	di
	inc	si
	cmp	al, 0ah
	je	@@too_long	; Stop if we copied 0xa - DOZE have some interesting ideas of ending the command line.
	test	al, al
	jnz	@@loop		; Stop if we did copy nul, else continue.

@@the_end:
	pop	cx si di
	ret

@@too_long:
	mov	[byte di], 0		; Terminate command line.
	jmp	@@the_end
		
endp	copy_command_line

	
;------------------------------------------------------------------------------
; Analyses the contents of command_line and sets variables accordingly.
; In:	Nothing.
; Out:	AX - destroyed.
;	DX - destroyed.
;	ES - destroyed.
	
proc	interpret_command_line
	push	di si cx bx

	push	ds
	pop	es
	
	;; First we must step over FDXMS.SYS, which we do by scaning for the first space, tab or ^M character.
	mov	si, offset command_line		; Remember where search started.
	mov	cx, XMS_COMMAND_LINE_LENGTH_MAX
	add	cx, si				; cx is guard.
	mov	bx, 0920h	; BH=tab, BL=space.
	mov	ah, 0dh		; AH=^M.
	
	
@@look_again_for_white_space:
	mov	al, [ si ]
	cmp	al, bl
	je	@@found_white_space
	cmp	al, bh
	je	@@found_white_space
	cmp	al, ah
	je	@@found_white_space
	inc	si
	cmp	si, cx
	jae	@@done
	jmp	@@look_again_for_white_space
			
@@found_white_space:
@@next_arg:
	mov	bx, 0d0ah	; BH=^M, BL=^J
	mov	ax, 0920h	; AH=tab, AL=space
	call	eat_characters
	
	cmp	[byte si], 0
	je	@@done		; End of string?
	
	cmp	si, cx
	jae	@@done		; Is this necessary?
	
	mov	bx, si		; Remember current position
	
IFDEF TRACE_CODE
	;; TRACE argument.
	mov	di, offset trace_str
	call	strcmp
	jc	@@try_ps

	push	ax
	cmp	[ byte si ], '='
	je	@@trace_equal_sign
	mov	ax, 0ffffh
	jmp	@@trace_no_value

@@trace_equal_sign:	
	inc	si
	call	read_integer
	jc	@@trace_expects_integer
	
@@trace_no_value:
	mov	[ trace ], ax
	mov	[ trace2 ], dx
	pop	ax
	jmp	@@next_arg

@@trace_expects_integer:
	pop	ax
	mov	ax, offset trace_no_int_str
	call	print_string
	jmp	@@next_arg
	
	;; PS argument.
@@try_ps:
	mov	si, bx
ENDIF ; TRACE_CODE

	mov	si, bx
	mov	di, offset ps_str
	call	strcmp
	jc	@@try_delay

	mov	[ byte enable_a20_out_value_patch ], 02h ; Patch value out.
	mov	[ byte enable_a20_out_port_patch ], 92h	 ; Patch out port address.
	mov	[ byte disable_a20_out_value_patch ], 0 ; Patch value out.
	mov	[ byte disable_a20_out_port_patch ], 92h ; Patch out port address.
	mov	al, 090h	; NOP.
	mov	di, offset enable_a20_nop_start
	mov	cx, enable_a20_nop_end - enable_a20_nop_start
	rep	stosb
	mov	di, offset disable_a20_nop_start
	mov	cx, disable_a20_nop_end - disable_a20_nop_start
	rep	stosb
	jmp	@@next_arg

	;; DELAY argument.
@@try_delay:
	mov	si, bx
	mov	di, offset delay_str
	call	strcmp
	jc	@@try_numhandles
	
	push	ax
	cmp	[ byte si ], '='
	jne	@@delay_no_value_given

	inc	si
	call	read_integer
	jc	@@delay_no_value_given
	
	test	dx, dx
	jz	@@delay_value_ok

	mov	ax, offset delay_too_big_str
	call	print_string
	mov	ax, 0ffffh

@@delay_value_ok:	
	mov	[ xms_delay ], ax	
	pop	ax
	jmp	@@next_arg	

@@delay_no_value_given:	
	mov	ax, offset delay_no_value_str
	call	print_string
	pop	ax
	jmp	@@next_arg

	;; NUMHANDLES argument.
@@try_numhandles:
	mov	si, bx
	mov	di, offset numhandles_str
	call	strcmp
	jc	@@try_int15

	push	ax
	cmp	[ byte si ], '='
	jne	@@numhandles_no_value_given

	inc	si
	call	read_integer
	jc	@@numhandles_no_value_given

	test	dx, dx
	jnz	@@numhandles_too_big
	
	cmp	ax, 2
	jae	@@numhandles_at_least_two

	mov	ax, offset numhandles_too_small_str
	call	print_string
	mov	ax, 2

@@numhandles_at_least_two:
	cmp	ax, NUMHANDLES_MAX
	jbe	@@numhandles_value_ok
	
@@numhandles_too_big:	
	mov	ax, offset numhandles_too_big_str
	call	print_string
	mov	ax, NUMHANDLES_MAX

@@numhandles_value_ok:
	mov	[ xms_num_handles ], ax
	pop	ax
	jmp	@@next_arg	

@@numhandles_no_value_given:	
	mov	ax, offset numhandles_no_value_str
	call	print_string
	pop	ax
	jmp	@@next_arg

	;; INT15 argument.
@@try_int15:
	mov	si, bx
	mov	di, offset int15_str
	call	strcmp
	jc	@@bad_arg

	push	ax
	cmp	[ byte si ], '='
	jne	@@int15_no_value_given

	inc	si
	call	read_integer
	jc	@@int15_no_value_given

	test	dx, dx
	jnz	@@int15_too_big
	
	cmp	ax, 64
	jae	@@int15_hma_included
	
	mov	ax, offset int15_hma_not_included_str
	call	print_string
	xor	ax, ax
	
@@int15_hma_included:
	cmp	ax, INT15_MAX
	jbe	@@int15_value_ok

@@int15_too_big:	
	mov	ax, offset int15_too_big_str
	call	print_string
	mov	ax, INT15_MAX

@@int15_value_ok:
	mov	[ int15_mem ], ax
	pop	ax
	jmp	@@next_arg

@@int15_no_value_given:
	mov	ax, offset int15_no_value_str
	call	print_string
	pop	ax
	jmp	@@next_arg
	
	;; Bad argument.
@@bad_arg:
	mov	ax, offset bad_arg_pre
	call	print_string
	mov	si, bx
	mov	di, offset bad_arg_arg
	
@@bad_arg_loop:	
	mov	al, [si]
	mov	[di], al
	test	al, al
	jz	@@bad_arg_end
	cmp	al, ' '
	je	@@bad_arg_end
	cmp	al, 9		; tab
	je	@@bad_arg_end
	cmp	al, 0ah		; lf
	je	@@bad_arg_end
	cmp	al, 0dh		; cr
	je	@@bad_arg_end
	inc	si
	inc	di
	jmp	@@bad_arg_loop
	
@@bad_arg_end:
	mov	[byte di], 0
	mov	ax, offset bad_arg_arg
	call	print_string
	mov	ax, offset bad_arg_post
	call	print_string	

	mov	al, [si]
	test	al, al
	jz	@@done
	
	inc	si
	jmp	@@next_arg

	;; The end.
@@done:	
	pop	bx cx si di
	ret	
endp	interpret_command_line
	
IFDEF TRACE_CODE
trace_str			db	'TRACE',0
trace_no_int_str		db	'TRACE expects an integer (e.g. TRACE=0xff)',13,10,0
ENDIF ; TRACE_CODE

numhandles_str			db	'NUMHANDLES',0
numhandles_too_small_str	db	'Too small argument to NUMHANDLES increased to 2',13,10,0
numhandles_too_big_str		db	'Too big argument to NUMHANDLES clamped down to 0x400',13,10,0
numhandles_no_value_str		db	'NUMHANDLES expects an argument (e.g. NUMHANDLES=32)',13,10,0
delay_str			db	'DELAY',0
delay_too_big_str		db	'Too big argument to DELAY clamped down to 0xffff',13,10,0
delay_no_value_str		db	'DELAY expects an argument (e.g. DELAY=5)',13,10,0
ps_str				db	'PS',0
int15_str			db	'INT15',0
int15_hma_not_included_str      db      'Too small argument to INT15 ignored.',13,10,0
int15_too_big_str               db      'Too big argument to INT15 clamped down to '
                                db      INT15_MAX_STR
                                db      '.',13,10,0
int15_no_value_str              db      'INT15 expects an argument (e.g. INT15=0x800).',13,10,0
bad_arg_pre			db	'Ignoring invalid option: ',0
bad_arg_post			db	13,10,0
bad_arg_arg			db	256 dup (?)
xms_mem_found_middle		db	' ($'
xms_mem_found_post		db	' decimal) bytes extended memory detected', 13, 10, '$'

;------------------------------------------------------------------------------
; Reads an integer from a string pointed at by SI.
; In:	SI - pointers to string.
; Out:	CY=0 - successful conversion.
;	  SI - updated.
;	  DX:AX - integer read.
;	CY=1 - First character wasn't a digit.
;	  DX:AX - destroyed.
		
proc	read_integer
	push	bx cx di
	xor	ax, ax
	xor	cx, cx
	xor	dx, dx
	mov	al, [si]
	cmp	al, 0
	je	@@failure
	cmp	al, '0'
	jb	@@failure
	cmp	al, '9'
	ja	@@failure
	sub	al, '0'
	mov	cx, ax
	inc	si
	mov	al, [si]
	cmp	al, 'X'
	je	@@perhaps_hex_number
	
@@decimal_loop:
	cmp	al, '0'
	jb	@@done
	cmp	al, '9'
	ja	@@done

	and	al, 0fh		; ax contains 0 - 9 now.
	mov	bx, dx
	mov	di, cx		; bx:di == dx:cx
	add	di, di
	adc	bx, bx		; bx:di == 2*dx:cx
	add	di, di
	adc	bx, bx		; bx:di == 4*dx:cx
	add	cx, di
	adc	dx, bx		; dx:cx = 5*(original dx:cx)
	add	cx, cx
	adc	dx, bx		; dx:cx = 10*(original dx:cx)
	add	cx, ax
	adc	dx, 0
	inc	si
	mov	al, [si]
	cmp	al, 0
	je	@@done
	jmp	@@decimal_loop
	
@@done:
	mov	ax, cx
	pop	di cx bx
	clc
	ret

@@perhaps_hex_number:
	cmp	cx, 0
	jne	@@done

	inc	si
	mov	al, [si]
	cmp	al, 0
	je	@@looked_like_hex_but_was_not
@@hex_number:	
	cmp	al, '0'
	jb	@@looked_like_hex_but_was_not
	cmp	al, 'F'
	ja	@@looked_like_hex_but_was_not
	cmp	al, '9'
	jbe	@@digit
	cmp	al, 'A'
	jb	@@looked_like_hex_but_was_not
	add	al, '0'-'A'+10	; Sets al to the ASCII code for the characters
				; after '9'.
@@digit:
	sub	al, '0'
	mov	cx, ax
	inc	si
	mov	al, [si]
@@hex_loop:	
	cmp	al, '0'
	jb	@@done
	cmp	al, 'F'
	ja	@@done
	cmp	al, '9'
	jbe	@@another_digit
	cmp	al, 'A'
	jb	@@done
	add	al, '0'-'A'+10	; Sets al to the ASCII code for the characters
				; after '9'.
@@another_digit:
	sub	al, '0'
	add	cx, cx
	adc	dx, dx		; *2
	add	cx, cx
	adc	dx, dx		; *2
	add	cx, cx
	adc	dx, dx		; *2
	add	cx, cx
	adc	dx, dx		; *2
	add	cx, ax
	adc	dx, 0
	inc	si
	mov	al, [si]
	cmp	al, 0
	je	@@done
	jmp	@@hex_loop
	
@@looked_like_hex_but_was_not:
	dec	si
	jmp	@@done

@@failure:
	pop	di cx bx
	stc
	ret
	
endp	read_integer
;------------------------------------------------------------------------------
; Increases SI until a character that don't match contents of AH, AL, BH or BL is found.
; In:	SI - pointer to string.
;	AL - character to step over
;	AH - character to step over, 0x0 to ignore and to ignore BX contents.
;	BL - character to step over, 0x0 to ignore and to ignore BH contents.
;	BH - character to step over, 0x0 to ignore.
; Out:	SI - updated
	
proc	eat_characters
@@loop:	
	cmp	al, [si]
	jne	@@try_ah
	inc	si
	jmp	@@loop
@@try_ah:
	test	ah, ah
	jz	@@done
	cmp	ah, [si]
	jne	@@try_bl
	inc	si
	jmp	@@loop
@@try_bl:
	test	bl, bl
	jz	@@done
	cmp	bl, [si]
	jne	@@try_bh
	inc	si
	jmp	@@loop
@@try_bh:
	test	bh, bh
	jz	@@done
	cmp	bh, [si]
	jne	@@done
	inc	si
	jmp	@@loop

@@done:	
	ret
endp	eat_characters
	
;------------------------------------------------------------------------------
; Compares two strings up to DI points at nul.
; In:	DI, SI - pointers to strings to compare.
; Out:	CY=0 - strings equal
;	  DI - points at null
;	  SI - points at character where DI is null.
;	  AX - destroyed
;	CY=1 - strings not equal
;	  DI - points at character which diffs
;	  SI - points at character which diffs
;	  AX - destroyed
		
proc	strcmp
@@loop:	
	mov	al, [di]
	cmp	al, 0		; Is it nul?
	je	@@done		; Yes, we have a match!
	
	cmp	al, [si]
	jne	@@failure	; Not equal so no match.
	inc	si
	inc	di
	jmp	@@loop

@@failure:	
	stc
	ret
	
@@done:
	clc
	ret		
endp	strcmp
	
ends	code16

;******************************************************************************

end
