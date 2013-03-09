 ; This file is part of LBAcache, the 386/XMS DOS disk cache by
 ; Eric Auer (eric@coli.uni-sb.de), 2001-2003.

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
; GPL 2 software by Eric Auer <eric@coli.uni-sb.de> 2001-2003




	; main read handling functions
	; for CHS: hdread
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


%ifdef DBGcnt
%define TSTCX call tstcx2
cxmsg	db ' cx',0
tstcx2:	pushf
	push ax
	mov ax,cx
		push word cxmsg
		call meep
	pop ax
	popf
	ret
%else
%define TSTCX nop
%endif

%ifdef DBGcnt
%define TSTBX call tstbx2
bxmsg	db ' bx',0
tstbx2:	pushf
	push ax
	mov ax,bx
		push word bxmsg
		call meep
	pop ax
	popf
	ret
%else
%define TSTBX nop
%endif




		; our replacement for function 0x42, LBA read
lbaread:	; DL is left intact all the time - important...
		; Most of this is done in readmain, because
		; both CHS and LBA are converted to LBA and
		; readmain "is" LBA read (with fallback to CHS)
	STACKDEBUG
	cmp word [cs:rwbusy],0	; nesting protecion
	jnz lrnested
	inc word [cs:rwbusy]	; nesting protection

	push bx	; save
	push ax		; because AL is not supposed to change
		call readmain	; very complete this time!
	pop bx		; TO BX to merge saved AL and new AH there
	mov bh,ah	; update saved AH to status
	mov ax,bx	; the new AX is ready
	pop bx	; restore
	pushf
	dec word [cs:rwbusy]	; <- nesting protection
	popf
lrdone:	STACKDEBUG
	jmp i13retf	; RETF +2 (i13retf also handles local stack)

lrnested:	push word nesterr
		call meep	; warn
	mov ax,0x8000		; busy (or is it 0xaa ?)
	mov word [ds:si+2],0	; no sectors success
	stc
	jmp lrdone

; --------------

	; *** NOTE ABOUT HDREAD:
	; *** To stay compatible with non-lba BIOSes, we convert
	; *** back to CHS in readmain if needed - BUT this works
	; *** only if the geometry has (funny constraint) numheads
	; *** and numsectors -as least as big- as the actual values!
	; *** the CHS <-> LBA functions will deactivate the cache
	; *** completely if any inconsistency is found.

	; *** Update: If > 1 sector is accessed at a time, we will
	; *** get havoc even if numheads or numsectors is -bigger-
	; *** than the actual values! (Jan. 2002)

	; Our replacement for function 0x02, CHS read:
	; simply translates the call to a LBA one and calls readmain!
hdread:	STACKDEBUG
	cmp word [cs:rwbusy],0	; nesting protecion
	jnz hrnested
	inc word [cs:rwbusy]	; nesting protection

	push ds
	push si
	push eax

	mov ah,0	; B->W
	mov si,ax	; save sector count
		call CHStoLBA	; CX DH to EAX in context DL
	; we create this structure:
		; B 0x10 (size of structure)
		; B 0
		; W count (from and to AL)
		; D pointer (from ESBX)
		; Q sector number (LBA)
	push dword 0	; C: sector number high part is 0
	push eax	; 8: sector number
	push es 	; 6: segment buffer
	push bx 	; 4: offset buffer
	push si 	; 2: stored sector count
	push word 0x10	; 0: 0x10, 0x00
	mov si,sp	; point to our new structure
	mov ax,ss
	mov ds,ax	; point to our new structure
	mov ax,0x4200	; (not really needed by readmain)
		call readmain	; very complete this time
	mov al,[ds:si+2]	; get returned sector count
	mov si,ax	; save AX
	pop eax ; remove
	pop eax ;  our
	pop eax ;   structure
	pop eax ;    from stack!

	pop eax
	mov ax,si	; restore AX (L: count H: status)

%ifdef DBGr
		push word meepmsg	; DBG (beep, no message)
		call meep		; DBG - *** REALLY CURIOUS ***
%endif

	pop si
	pop ds
	pushf
	dec word [cs:rwbusy]	; nesting protection
	popf
hrdone:	STACKDEBUG
	jmp i13retf	; RETF +2 (i13retf also handles local stack)

hrnested:	push word nesterr
		call meep	; warn
	mov ax,0x8000		; busy, 0 sectors (or is it 0xaa?)
	stc
	jmp hrdone

; ---------------------------------------------------------------

		; The main reading loop, using LBA semantics in
		; general (DS:SI has a buffer, DL the drive).
readmain:	; Should be improved:
		; For now, multisector reads are done by doing 
		; either a read from disk or from cache for every
		; single sector (speed loss!).

	; *** Update (Jan. 2002): we count cache misses and
	; *** pool the actual disk reads (no more speed loss...) !

	cmp byte [cs:pendingwrite],0
	jnz near lrd_busy	; skip with busy if write in progress

	push word [ds:si+6]	; store original buffer segment
	push dword [ds:si+8]	; store original sector number
	pusha
	push eax

	mov cx,[ds:si+2]	; sector count
	TSTCX
	xor bp,bp		; COUNT of read sectors...
				; @@@       @@@
		xor bx,bx	; @@@ COUNT of CACHE MISSES
				; @@@       @@@
	jcxz lrd_done
	mov ah,0		; status: OK
	mov di,ax		; this will store our AX value!

lrd_lp: mov eax,[ds:si+8]	; load sector number
		call findbin	; EAX DL found in cache?
	jc short notcached	; if not found, read from disk

cached:		call REGDUMP		; @@@    @@@
		call POOLED_READS	; @@@ do BX actual disk reads
					; @@@    @@@
		jc lrd_done	; @@@
	inc dword [cs:rdhit]	; statistics
	push es
		push bx		; @@@
	mov bx,[ds:si+4]	; buffer offset
	mov es,[ds:si+6]	; buffer segment
		call copytodos	; if found, read data from XMS bin AX
		pop bx		; @@@
	pop es
		and di,0x00ff	; status: OK
	jmp short nextlrd	; on to the next sector

notcached:
%ifdef DBGcnt
        push ax			; DEBUG
        push bx			; DEBUG
        mov ah,0x0e     ; tty	; DEBUG
        mov al,'.'		; DEBUG
        mov bx,7		; DEBUG
        int 0x10		; DEBUG
        pop bx			; DEBUG
        pop ax			; DEBUG		; @@@       @@@
%endif
		inc bx		; @@@ COUNT but do NOT read from disk yet
				; @@@       @@@
	inc dword [cs:rdmiss]	; statistics

; @@@	push cx		; <- SAVE COUNT
; @@@	mov cx,1		; *** do ONE sector at a time!

; @@@		mov ax,di	; have some AL :-)
; @@@		call readfromdisk	; CX from [si+8]l.DL to
					; [si+6]w:[si+4]w
; @@@		mov di,ax	; status: FROM CALL
			; COUNT assumed to be 1 iff NC here
			; - or read from CX !
; @@@	pop cx		; <- RESTORE COUNT
; @@@	jc lrd_done		; the first error ends the call!

; @@@	mov eax,[ds:si+8]	; load sector number
; @@@		call newbin	; find a space in XMS -> bin AX
; @@@	push es
; @@@		push bx		; @@@
; @@@	mov bx,[ds:si+4]	; buffer offset
; @@@	mov es,[ds:si+6]	; buffer segment
; @@@		call copytoxms	; copy data to XMS bin AX
; @@@		pop bx		; @@@
; @@@	pop es

nextlrd:
	inc bp		; COUNT: one more sector read ok
	add word [ds:si+6],0x20 ; 512/16: next buffer position
	inc dword [ds:si+8]	; next sector (LBA)
	loop lrd_lp
		call REGDUMP		; @@@    @@@
		call POOLED_READS	; @@@ do BX actual disk reads
					; @@@    @@@
lrd_done:
	mov [ds:si+2],bp	; return our sector COUNT

	mov [cs:evilword], di	; what is to be returned as AX
				; *** VERY LAZY AND NOT ELEGANT
	pop eax
	popa
	pop dword [ds:si+8]	; restore original sector number
	pop word [ds:si+6]	; restore original buffer segment

	mov ax, [cs:evilword]	; *** STILL LAZY AND NOT ELEGANT

	clc
	or ah,ah
	jz lrd_ok
	stc			; set carry on error
lrd_ok: ret			; done...

lrd_busy:
	mov ax,0x8000	; busy (0x0aa for hard disks?)
	mov word [ds:si+2],0	; no sectors read
	stc
	ret

evilword	dw 0	; lazy!

; ---------------------------------------------------------------

	; @@@ This is used for the new (Jan. 2002) multi
	; @@@ sector read pooling feature. Registers:
	; @@@ DS:SI -> data structure
	; @@@ DL -> drive
	; @@@ DI -> current return value (for AX)
	; @@@ BP -> count of ok sectors (including the sectors
	; @@@  -we- should do, so we must sub them on failure)
	; @@@ (plus CX to do count, EAX sector, ...)

	; @@@ This must handle BX being 0, return CY on
	; @@@ error and do the proper rollback in DS[SI],
	; @@@ containing ptr/[4] to buffer, dword/[8] sector,
	; @@@ and other stuff that we do not use here (like
	; @@@ on word [2]the external given/returned count)...

POOLED_READS:		; no checks here for count or buffer pointer
	clc		; overflows, as those have been done earlier.
	TSTBX
	or bx,bx
	jz near pore_end

pore_real:		; we set DS:SI, DL, CX (count) and get
	push ax		; AX, CX (count), CY/NC back...
	push bx		; unless there is an error, we do ***NOT***
	push cx		; check the count, and if there is, we
			; pretend complete failure. What the heck...
	push word [ds:si+6]	; buffer position
	push dword [ds:si+8]	; sector number

	mov cx,bx
	shl cx,5		; mul (0x200>>4), thus mul 0x20
	sub [ds:si+6],cx	; move BUFFER back
	push eax
	movzx eax,bx
	sub [ds:si+8],eax	; move SECTOR back
	pop eax
	mov cx,bx		; the COUNT

	mov ax,di		; status
	call readfromdisk	; REAL disk read this time
				; CX from [si.8]l.DL to [si+4]p
	mov di,ax		; status

	pop dword [ds:si+8]
	pop word [ds:si+6]  
	pop cx
	pop bx
	pop ax
	jc pore_err

	; next, we do all the rollback AGAIN to copy all to XMS !!!

pore_xms:
	push eax
	push cx
	push word [ds:si+6]	; buffer position
	push dword [ds:si+8]	; sector number

	mov cx,bx
	shl cx,5		; mul (0x200>>4), thus mul 0x20
	sub [ds:si+6],cx	; move BUFFER back
	push eax
	movzx eax,bx
	sub [ds:si+8],eax	; move SECTOR back
	pop eax
	mov cx,bx		; the COUNT

pore_nextxms:
	mov eax,[ds:si+8]	; load sector number
		call newbin	; find space in XMS -> bin AX
	push es
	push bx
	mov bx,[ds:si+4]	; buffer offset
	mov es,[ds:si+6]	; buffer segment
		call copytoxms	; copy data to XMS bin AX
	pop bx
	pop es
	add word [ds:si+6],0x20	; 512/16: next buffer position
	inc dword [ds:si+8]	; next sector (LBA)
	loop pore_nextxms

	pop dword [ds:si+8]
	pop word [ds:si+6]  
	pop cx
	pop eax
	
pore_end:
	xor bx,bx	; we must RESET the COUNT of postponed reads!
	clc
	ret

pore_err:
	sub bp,bx	; correct count: less sectors were really
	xor bx,bx
	stc		; ok than planned while postponing reads!
	ret
	

; ---------------------------------------------------------------

		; Read from real disk - defaults to LBA,
		; but has fallback to CHS.
readfromdisk:	; CX sectors from [si+8]l.DL to [si+6]:[si+4],
		; returns status CF and AX and count CX
		; obviously requires a LBA structure on ds:[si]...
	test dl,0x80		; floppy or harddisk?
	jz readusingchs		; never use LBA for floppies

	; *** NEW 11/2002: use havelba as a bitfield
	push ax			; ***
	push cx			; ***
	mov ax,1		; *** LSB is C:
	mov cl,dl		; *** drive
	and cl,7		; *** mask: max 8 harddisks ***CL***
	shl ax,cl		; *** ***SHL***
	test word [cs:havelba],ax	; *** LBAable disk?
	pop cx			; ***
	pop ax			; ***
	jz readusingchs		; if no LBA, fall back to CHS

	mov ax,0x4200		; LBA read
	push word [ds:si+2]
	push dx
	mov word [ds:si+2],cx	; number of sectors to do
		call callold	; do a real disk read (cache miss)
	pop dx
	mov cx,[ds:si+2]	; return number of sectors read!
	pop word [ds:si+2]
	ret

rddmaover:
	mov ax,0x800		; DMA overrun, 0 sectors
	mov cx,0		; return "0 sectors read" in CX
	stc
	ret

readusingchs:
	test cx,0xff80		; too big for CHS?
	jnz rddmaover
	push dx
	push es
	push eax
	push bx
	push cx
	mov eax,[ds:si+8]	; which sector
	mov bl,cl		; save count from CL (CX)
	call LBAtoCHS		; get CHS from LBA again
	test cl,63		; valid sector number?
	jz rdchswimp
	mov al,bl		; restore count
	les bx,[ds:si+4]	; read buffer pointer
	mov ah,2		; classic CHS read, drive DL
		call callold	; do a real disk read (cache miss)
	mov dx,ax		; save AX
	jnc rdcok1
	cmp ah,0x11		; THIS error for READ (ecc corrected)
	jnz rdccy		; returns a special AL value!
rdcecc:	mov dl,0		; saved AL -> 0 sectors !
rdcok1:	jmp short rdchsok
rdchswimp:
	mov dx,0x400		; read error, 0 sectors - save for AX
rdccy:	stc			; set CARRY again
rdchsok:
	pop cx
	pop bx
	pop eax
	pop es
	mov ax,dx	; restore AX
	mov ch,0
	mov cl,al	; return in CX the number of sectors read!
	pop dx
	ret
