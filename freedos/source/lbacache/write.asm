 ; This file is part of LBAcache, the 386/XMS DOS disk cache by
 ; Eric Auer (eric@coli.uni-sb.de), 2001-2004.

 ; LBAcache is free software; you can redistribute it and/or modify
 ; it under the terms of the GNU General Public License as published
 ; by the Free Software Foundation; either version 2 of the License,
 ; or (at your option) any later version.

 ; LBAcache is distributed in the hope that it will be useful,
 ; but WITHOUT ANY WARRANTY; without even the implied warranty of
 ; MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 ; GNU General Public License for more details.

 ; You should have received a copy of the GNU General Public License
 ; along with LBAcache; if not, write to the Free Software Foundation,
 ; Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307 USA
 ; (or try http://www.gnu.org/licenses/licenses.html at www.gnu.org).

; LBAcache - a hard disk cache based on XMS, 386 only,
; and aware of the 64bit LBA BIOS Int 13 Extensions.
; GPL 2 software by Eric Auer <eric@coli.uni-sb.de> 2001-2004

; Check out the CHS version as well (limited to 8 GB,
; uses less DOS memory, and wimps out on LBA write)...

cnterr_lba	db '/LBA#',0	; improved error messages 22sep2004
cnterr_chs	db '/CHS#',0	; improved error messages 22sep2004

; %define FLUSHWRITE 1	; flush after any write rather than
			; updating the cache (writemain),
			; in case the updating is broken!
			; (* a very obscure debugging option *)

	; main write handling functions
	; for CHS: hdwrite
	; es:bx is buffer, cx/dh location, dl drive, al size
	; for LBA: lbawrite
	; dl is drive, ds:si points to a structure of:
	; B 0x10 (0x18 to allow a 64bit flat pointer)
	; B 0
	; W number of sectors (also used for a return value:
	;   number of sucessfully read/written sectors)
	; D DOS pointer to buffer (or -1 to use flat pointer)
	; Q sector number
	; Q optional flat pointer
	; (we do NOT handle the flat pointer or sector numbers
	;  longer than 32bit, the dispatcher checks this!)

		; Our replacement for function 0x43, LBA write
lbawrite:	; very straightforward: FIRST calls the real
		; write, then copies as many sectors to the
		; cache as were successfully written
	STACKDEBUG
	cmp word [cs:rwbusy],0	; nesting protection
	jnz lbawrnest
	inc word [cs:rwbusy]	; nesting protection
	inc word [cs:pendingwrite]	; cache and disk out of sync
	push cx

		call lbatodisk	; <<< do the real call and check for errors

; ***	jcxz nolbwr	; nothing to update
	push eax
	push es
	push bx
	mov eax,[ds:si+8]	; load sector number
	mov bx,[ds:si+4]	; load buffer offset
	mov es,[ds:si+6]	; load buffer segment

		call writemain	; <<< update cache (ESBX EAX DL CX)
				; (preserves carry flag from lbatodisk)

	pop bx
	pop es
	pop eax
nolbwr:
	pop cx
	pushf
	dec word [cs:pendingwrite]	; cache and disk in sync
	dec word [cs:rwbusy]	; nesting protection
	popf
lwdone:	STACKDEBUG
	jmp i13retf	; RETF +2 (i13retf also handles local stack)

lbawrnest:
	mov word [ds:si+2],0	; no sectors written
		push word nesterr
		call meep	; warn
	mov ax,0x8000	; busy (or is it 0x0aa for hard disks?)
	stc
	jmp lwdone

; ---------------------------------------------------------------

		; Our replacement for function 0x03, CHS write.
hdwrite:	; FIRST calls the real write, then copies as many
		; sectors to the cache as were successfully written
	STACKDEBUG
	cmp word [cs:rwbusy],0	; nesting protection
	jnz hdwrnest
	inc word [cs:rwbusy]	; nesting protection
	inc word [cs:pendingwrite]	; cache and disk out of sync
	push bp		; destroyed by chstodisk
	push cx 	; same...
	push eax	; same...

		call chstodisk	; <<< do actual disk writing
				; returns EAX (lba) BP (status) CX (count)

; ***	jcxz nohdwr	; nothing to update
		; eax already with sector number from chstodisk
		; es bx already set as usual (buffer offset/segment)

		call writemain	; <<< update cache (ESBX EAX DL CX)
				; preserves carry flag from chstodisk

nohdwr: pop eax ; destroyed by chstodisk
	mov ax,bp	; status+size from chstodisk is in BP
	pop cx	; also destroyed
	pop bp	; same...
	pushf
	dec word [cs:pendingwrite]	; cache and disk in sync
	dec word [cs:rwbusy]	; nesting protection
	popf
hwdone:	STACKDEBUG
	jmp i13retf	; RETF +2 (i13retf also handles local stack)

hdwrnest:	push word nesterr
		call meep	; warn
	mov ax,0x8000	; *** no sectors written, busy (or is it
	stc		; *** 0x0aa for hard disks?)
	jmp hwdone

; ---------------------------------------------------------------

lbatodisk:	; do the real LBA write
		; destroys CX (now number of sectors to be updated)
	mov cx,[ds:si+2]	; sectors to be transferred
	push cx	; SAVE intended size
	push dx
	push dword [ds:si+4]	; buffer pointer paranoia
	push dword [ds:si+8]	; sector number paranoia
		call callold	; call original int 0x13
	pop dword [ds:si+8]	; sector number paranoia
	pop dword [ds:si+4]	; buffer pointer paranoia
	pop dx
	pop cx	; RESTORE intended size
	jc lba_wrerr		; Carry was set

lba_wrok:			; stupid enough, my LBA BIOS seems
				; to set [ds:si+2]w to 0 on success!
	xor ch,ch		; return INTENDED count
	ret

lba_wrerr:			; // error handling changed 22sep2004
	mov ch,[ds:si+2]	; actual count, in theory. CL is intended...
		push ax
		mov al,dl	; D for message: status.drive
		push word wrerr	; D *offset*
		call meep	; D
		mov ax,cx	; D
		push word cnterr_lba	; D HI of LO written
		call meep	; D
		pop ax
	call flushone		; *** for safety (was full flush - 22sep2004)
	cmp ch,cl		; 'all written' in spite of error?
	mov cl,ch		; preferrably return ACTUAL count
	jnz lbacnt_b		; normal error
	mov cl,0		; assume that nothing was written
	; mov [ds:si+2],0	; even change here!?
lbacnt_b:
	xor ch,ch		; return ACTUAL count or ZERO
	stc
	ret

; ---------------------------------------------------------------

chstodisk:	; Do the real CHS write
		; puts number of sectors to be updated in CX
		; and puts LBA sector number in EAX
		; what would normally end up in AX is stored in BP.
	push di
	  push cx
	xor cx,cx
	mov cl,al	; number of sectors
	mov di,cx	; SAVE intended number of sectors
	  pop cx
	push cx		; for chs to lba
	push dx		; for chs to lba
	push di		; SAVE intended number of sectors
		call callold	; call original int 0x13
	mov bp,ax	; AX -> BP
	pop di		; RESTORE intended number of sectors
	pop dx		; for chs to lba
	pop cx		; for chs to lba
		call CHStoLBA
			; EAX now LBA position
	mov cx,di	; RESTORE intended number of sectors
	pop di
	jc chs_wrerr

chs_wrok:		; My BIOS returns AL=0 on success,
			; there is only a count on error!?!?
			; return INTENDED number of sectors in CX,
			; not looking at the returned AX in BP !
	ret		; return (EAX BP CX are set)

chs_wrerr:		; // error handling changed 22sep2004
	push ax
	mov ax,bp	; old AX value, as returned by real int 0x13
	mov ch,al	; CH actual, CL intended count
%ifdef MUTEFDWRERR
	test dl,0x80		; the define MUTEFDWRERR suppresses the
	jz chs_wr_mute_normerr	; "flush/write error" message for floppies.
%endif
		mov al,dl	; D for message: status.drive
		push word wrerr	; D *offset*
		call meep	; D
		mov ax,cx	; D
		push word cnterr_chs	; D HI of LO written
		call meep	; D
%ifdef MUTEFDWRERR
chs_wr_mute_normerr:
%endif
	pop ax
	call flushone		; *** for safety (was full flush - 22sep2004)
	cmp ch,cl		; 'all written' in spite of error?
	mov cl,ch		; preferrably return ACTUAL count
	jnz chscnt_b		; normal error
	mov cl,0		; assume that nothing was written
chscnt_b:
	xor ch,ch		; return ACTUAL count or ZERO
	stc
	ret	; (EAX is lba sector number, BP ax-of-int-0x13, CX count)


; ---------------------------------------------------------------

writemain:	; generic write update function, using sector
		; number EAX drive number DL, size CX, buffer ES BX
		; and writes (copies) all the buffer to the cache
	pushf		; must save (carry...) FLAGS !
	pusha		; loop for all sectors: copy to cache
	mov di,es	; save ES

%ifdef FLUSHWRITE
	push ax
		mov ax,cx
		push word colonmsg
		call meep
	pop ax
	call flushone
	jmp wrdone
%endif

	jcxz wrdone

wrsec:	inc dword [cs:wrhit]	; statistics

	push eax	; save
		call findbin	; find EAX.DL bin, if any -> AX
	jnc short wrnotnew	; do not alloc a new bin if found

	dec dword [cs:wrhit]	; statistics
	inc dword [cs:wrmiss]	; statistics

	test byte [cs:tuneflags],2	; *** new 7/2004
	jz no_alloc_on_write		; *** new 7/2004

		; We even store written stuff into newly allocated
		; cache slots because we hope the data will be read
		; back again later. Otherwise, storing is optional.

	pop eax		; restore
	push eax	; save again
		call newbin	; create a new bin otherwise -> AX
				; (will store EAX and DL there)

wrnotnew:
		call copytoxms	; ES:BX to slot AX
			; *** table and sectors in sync again,
			; *** now the cache may change again

no_alloc_on_write:

	pop eax 	; restore
	inc eax 	; next sector (LBA)
	mov si,es	; si is unused
	add si,0x20	; 512/16
	mov es,si	; advance buffer pointer
	loop wrsec
wrdone:
	mov es,di	; restore ES
	popa
	popf		; must save FLAGS !
	ret

