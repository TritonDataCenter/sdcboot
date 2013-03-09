 ; This file is part of LBAcache, the 386/XMS DOS disk cache by
 ; Eric Auer (eric@coli.uni-sb.de), 2001-2008.

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
; GPL 2 software by Eric Auer <eric@coli.uni-sb.de> 2001-2006




; The main NEWI13 dispatcher which calls
; hdread and hdwrite which now come in two versions,
; one for LBA and one for CHS - {hd,lba}{read,write}
; we can return from those to either enderr, endok, or oldint
; for now, several functions cause the cache to be flushed or shut
; down completely - no support for flushing only one drive yet!
; also contains callold, which calls the original int 0x13


%ifdef DBG
gtmsg	db '>',0
%endif

lbagenerr	db "LBA: secnum past 4G or >127 sectors or...",0
staknesterr	db "LBACACHE: int. STAK nest!? otherss=",0

flopfreakerr	db "LBACACHE flush: floppy wrap AX=",0

fddstat		dw 0x0000	; 1/2 enable a/b caching,
				; 10/20 signal a/b changed
				; 100/200 a/b caching allowed mask

tuneflags	db 0		; new 7/2004: bitmask extra options
		; 1: fully assoc (~1: N-way assoc), in binsel2.asm
		; 2: alloc on write (~2: update on write), write.asm
		; 4: allocate stack in LOW RAM, for SCSI, setup.asm
		; *** other suggestions welcome ***

		; Writes are never delayed, but this option controls
		; whether written data is only updated if already in
		; cache OR if it is even added to cache if not...
		; alloc on write was default - 7/2004

temperature	db 0		; "cache temperature" - 9/2004

busy	db 0	; nonzero while local stack busy
localsp	dw 0	; is nonzero if we have a local stack
localss	dw 0	; local ss (usually equal to our CS)
othersp dw 0
otherss dw 0

enderr:	mov ah,4
	stc
	jmp i13retf	; LEAVE


	; the new INT 0x13 handler follows (only for 80386+XMS),
	; it is reached from the real hook which is as early in
	; CS as possible (int13new)

NEWI13:	cmp eax,'tick'
	jnz notickleinstcheck
	cmp ebx,'temp'
	jz temperature_control	; added 9/2004
	cmp bx,'le'
	jnz notickleinstcheck
	iret	; we BLOCK the TICKLE install check, because TICKLE
		; must be loaded AFTER LBAcache when we want to use
		; use the 8k+ buffer of TICKLE for XMS-to-disk
		; transfers (for delayed writes, not yet implemented)!

temperature_control:	; only useful if MELTFREEZE is enabled in binsel!
	mov [cs:temperature],cl
	cmp dx,4	; no results yet?
	jnz sum_up_temp	; else add our count to count of other instances
	mov dx,4	; start outside used drive number range
sum_up_temp:
	call countfrozen	; add count of frozen elements to DX
	jmp short t_old_	; chain to previous handler

notickleinstcheck:
	or dl,dl	; only cache hard drives for now
	js short to_hard

	cmp dl,1	; FLOPPY: cache floppy B ?
	ja t_old_
	jb to_drv_a
	test word [cs:fddstat],2	; B enabled?
	jnz carefordrive
t_old_:	jmp oldint_pure			; 2..7f or disabled
					; do NOT use after local
					; cache setup!
to_drv_a:
	test word [cs:fddstat],1	; A enabled?
	jz t_old_
	jmp short carefordrive	; yes, A cached

to_hard:		; harddisk...

	cmp dl,0x87	; *** only cache first 8 drives (3 bits...)
	ja short t_old_	; *** was 4 drives, 2 bits

drvcheck:		; check our what-to-cache-mask
	push cx
	push ax
; ***	mov ax,0x1000	; 0x80...
	mov ax,1	; *** LSB = drive 0x80
	mov cl,dl
	and cl,7	; *** 8 drives to be handled (was 4)
; ***	shl cl,2	; 4 bits per drive
; ***	shr ax,cl	; select drive in mask
	shl ax,cl	; *** select drive in mask
	test [cs:drvselmask],ax
	pop ax
	pop cx
	jz short t_old_	; no drive we care for



carefordrive:
	cmp word [cs:localss], byte 0	; local stack existing?
	jz care_nostack
	test byte [cs:busy],0xff	; local stack already busy?
	jz care_stack
		push ax
		mov ax,[cs:otherss]
		push word staknesterr
		call meep
		pop ax
	mov ah,0x80		; BUSY (0xAA for hard disks?)
	push bp
	mov bp,sp		; note: [bp+...] is implicitly [ss:bp+...]
	or byte [bp+6],1	; stack has BP IP, CS, flags: set carry, stc
	pop bp
	iret			; we return really early :-p

care_stack:
	mov [cs:otherss],ss		; prepare to use...
	mov [cs:othersp],sp		; ...our local...
	lss sp,[cs:localsp]		; ...stack!
	inc byte [cs:busy]		; AND mark it as busy!

care_nostack:

	test dl,0x80		; floppy or hard disk?
	jnz short care_hard

care_flop:			; only reached for A and B
	push ax
	clc
	mov ax,0x1600		; disk changed? test always...
		call callold	; call old int 13
	pop ax
	jnc short cared4flop	; NC: disk not changed

	; start of 2008 additions
	; DOSEMU 1.4 / ... returns carry, ah=1 (invalid command)
	; normal BIOS would return cy/ah=6 (changed/unknown) if disk
	; changed, and nc/ah=0 for not changed, cy/ah=80 if not ready
	; (some BIOSes even do proper USB disk change detection :-))
	cmp ah,1		; 2008 for DOSEMU 1.4
	jz cared4flop		; 2008 for DOSEMU 1.4
	test word [cs:havelba],0x8000	; "in DOSEMU" flag set?
	jnz cared4flop		; 2008 for DOSEMU in general
	or dl,dl		; ignore disk changes if A: and large
	jnz flushBchanged	; no such support for B: yet
	cmp byte [cs:ageometry+1],2	; (was) more than 2 heads?
	jae noflushAchanged	; then skip flush but update geometry
flushBchanged:
	; more than 2 heads: usually "USB stick or ZIP booting as A:"
	; end of 2008 additions

%ifdef FLOPCHGMSG
	push ax
	mov al,dl		;     all time for removeable disks!
	push word diskchangedmsg
	call meep		; tell user about disk change
	pop ax
%endif
%ifndef NOFLUSHFDD
		call flushone	; flush cache if disk changed
%endif

noflushAchanged:
	push ax
	mov ax,0x10		; A changed
	cmp dl,0
	jz a_chg
	mov ax,0x20		; B changed
a_chg:	
	call FDGEOSYNC		; synchronize floppy geometry drive DL
	or word [cs:fddstat],ax	; store the disk change info for the user!
	pop ax
cared4flop:
	cmp ah,0		; disk reset? Causes int 1E info re-read
	jnz carenoflopreset
	push ax
	call callold		; do floppy reset BEFORE syncing geometry
				; (also sends DDPT geometry to hardware)
	pop ax
	call FDGEOSYNC		; synchronize floppy geometry drive DL
carenoflopreset:
	cmp ah,5
	jb near rwmaybe	; 0..4 are most normal
	jz near quietfishy	; flush if format request (5)
		; *** FORMAT no longer gives a message - 24may2003 ***
	cmp ah,8
	jb near fishyfishy	; bailout if other odd format requests (6,7)
			; functions 6, 7 are XT harddisk format!

		; int 13.05 and 13.09 (hdd only) can modify the GEOMETRY
		; (from tables on int 1e,41,46) implicitly,
		; int 13.17, 13.18, (13.06 ?) explicitly
		; modify the geometry (but 13.17 never beyond 1.44m)

	jnz short cared_flop
georequest:	; user wants to know geometry, so do we!
	call FDGEOSYNC		; synchronize floppy geometry drive DL
	jmp near UNSTACK_oldint


care_hard:
	cmp ah,5        ; block format requests (5/6/7)
	jb near rwmaybe	; 0..4 are most normal
	cmp ah,8	; another format thing
	jb near enderr		; *** make format things return an error!
	jz near UNSTACK_oldint



cared_flop:
	cmp ah,9
	jz near fishyfishy	; REALLY bail out if hdd geometry is changed!
	cmp ah,0x0b
	jz near fishy	; flush cache if somebody writes long (with ECC)!


	cmp ah,0x16		; floppy change info request
	jnz no_chg_chk
	cmp dl,1
	ja no_chg_chk
	jb a_chg_chk
b_chg_chk:
	test word [cs:fddstat],0x20	; B changed?
	jz no_chg_chk			; bios can say no (or yes!) itself
	and word [cs:fddstat],0xffdf	; not(0x20)
chged:	
	mov ah,6		; this is reported only once per event,
	stc			; even if repeatedly asked for
	jmp i13retf		; LEAVE, tell that the disk
				; has been changed
a_chg_chk:
	test word [cs:fddstat],0x10	; A changed?
	jz no_chg_chk			; bios can say no (or yes!) itself
	and word [cs:fddstat],0xffef	; not(0x10)
	jmp short chged

no_chg_chk:

	cmp ah,0x18		; set media type for format: better bail out?
				; MS FORMAT even uses this for harddisks,
				; but this func cannot set head count, only
				; cyl / sec count and DDPT. Weird.
; ***	jz short fishyfishy	; func 0x18 is also used for harmless stuff
	jnz short no_geo_chg
	cmp dl,0x88		; beyond the range of "our" harddisks?
	jae no_geo_chg		; no geometry change that we are interested in
	cmp dl,0x80		; trying to modify geo of some HARDDISK?
	jae short fishyfishy	; bail out / shut down if harddisk affected
;	cmp dl,1		; A: or B: ? (already checked above anyway!)
;	ja no_geo_chg		; ignore other floppy drives

	; *** have to call real int 13 BEFORE syncing geometry (4/2004) ***
	; (in: AH, DL, CX, out: AH (0 ok, 1 n/a, C reject, 80 no disk), ES:DI)
	call callold		; call actual int 13 handler NOW
	pushf
	call FDGEOSYNC		; synchronize floppy geometry drive DL
	popf
	jmp i13retf		; all done, not calling original int 13 again!
	; *** end of 4/2004 bugfix (callold *before* FDGEOSYNC now) ***

no_geo_chg:
	cmp ah,0x22
	jz short fishy  	; PS/2 write incompatible to NEXTSEC
	cmp ah,0x21
	jz short fishy  	; PS/2 read incompatible to NEXTSEC
				; int 13 extensions: int 13.41.(bx)55aa.dl
				; 1 - func 42 43 44 47 48 (64bit lba)
				; 2 - func 45 46 48 49 (removable media)
				;     (... int 15.52.dl before eject ...)
				; 4 - func 48 4e (edd: set pio/dma mode...)
	cmp ah,0x42			; 64bit LBA read
	jz near lbacheck
	cmp ah,0x43			; 64bit LBA write
	jz near lbacheck
				; 44: verify 45: lock/unlock (counting)
				; 46: eject 47: seek 48: get drive param
				; (to buffer at dssi, init with: W size,
				; W flags=0, ... - size is 1a/1e/42,
				; flag will be OR of ... 4 removable ...)
	cmp ah,0x46
	jz short fishy	; flush cache if media is ejected (int 13 ext)
				; also related to our 64bit LBA stuff!
	cmp ah,0x4a
	jz short fishyfishy	; STOP cache if CD emulates boot drive
	cmp ah,0x4c
	jz short fishyfishy	; STOP cache if CD emulates boot drive

	cmp ah,0xec		; Con->Fmt (TSR floppy format tool) related
	jz confmt_jump		; harmless for caching (format itself trapped)
				; (the above got added 4/2004)

		; Trapped BIOS enhancer (EzDrive, OnTrack Disk Manager, SWBIOS)
		; 13.ef.DL.cx (+cx cyl on next int13),
		; 13.f9.DL (inst check: DX msb set), 13.fe.DL (get cyl-1024 DX)
		; 13.ff.DL (inst check: AX aa55, ES:BX version string)
		; --> Let people use normal CHS or normal LBA only!
	cmp ah,0xee	; "add 1024 to cyl for next call"
	jz blockedfunc
	cmp ah,0xef	; "add CX to cyl for next call"
	jz blockedfunc
	cmp ah,0xf9	; "install check - read feature flags to DX"
	jz blockedfunc2
	cmp ah,0xfe	; "read real_cyl-1024 to DX"
	jz blockedfunc2
	cmp ah,0xff	; "install check - return AX 0xaa55, string ES:BX"
	jz blockedfunc

	cmp ah,0x50		; <- AH
	jae short fishyfishy	; stop cache completely any undocumented stuff
	jmp short confmt_jump	; *** was missing, hint 13jul2004 J. Seifert

blockedfunc2:
	xor dx,dx
blockedfunc:
	stc
	jmp i13retf

confmt_jump:
	jmp UNSTACK_oldint	; other mixed stuff is not even worth flushing
	; *** this "dangerous function enumeration" is potentially dangerous

; --------------

fishyfishy:	; UNINSTALL because resetting the drive params may bend geom
		; or because of other weird things happening
	mov word [cs:running],2	; SHUT DOWN cache completely
				; (no need to modify any enable masks)
		push word fish2err
		call meep	; complain a bit...
	jmp UNSTACK_oldint	; [running] may be reset with a debugger,
				; so XMS and DOS RAM are not released

; --------------

fishy:		; FLUSH because a call with maybe unpredictable side effects
		; was detected!
	push ax
	mov al,dl	; show drive (and function AH) in error message
		push word fisherr
		call meep	; complain again!
	pop ax
quietfishy:	; *** FLUSH but do not give a message about it ***
		call flushone	; only flush ONE drive
	jmp UNSTACK_oldint

; --------------

rwmaybe:
	cmp ah,2
	jb near UNSTACK_oldint	; functions 0 (reset) / 1 (status) handled elsewhere
	cmp ah,3		; verify (function 4)?
	ja near UNSTACK_oldint
	cmp dl,2		; floppy A: or B: (2..7f ignored above anyway)?
	ja norwfloppy

				; *** Floppy freak check added 4/2004
	push ax			; functions 2/3 read/write: AL=sectors
	push bx			; ES:BX=buffer DL=drive DH=head CX=cyl-and-sec
	; ( DMA wraps are trapped when calling oldint from hdwrite / hdread )
	or dl,dl		; is it A: ? Assume that not...
	mov bx,[cs:bgeometry]	; lo: sec/track hi: max head number
	jnz norw_a		; it is indeed B:
	mov bx,[cs:ageometry]	; no, it really is A:!
norw_a:	cmp dh,bh		; off-range head?
	ja flop_blast
	cmp al,127		; reading too many sectors?
	ja flop_blast
	mov ah,cl
	and ah,63		; AH = starting sector number
	add ah,al		; last touched sector number + 1
	dec ah
	cmp ah,bl		; would it wrap past end of floppy track?
	ja flop_blast
	stc			; indicate that everything looks okay
flop_blast:			; if error found, this is reached by JA
				; (which means that neither CF nor ZF set)
	pop bx
	pop ax
	jc norwfloppy		; no floppy freak treatment needed
		push word flopfreakerr	; ** warn about freak situation
		call meep		; ** code is 02xx or 03xx

	push ax
	mov ax,[cs:ageometry]
	xor ax,[cs:bgeometry]	; just store a checksum
	call FDGEOSYNC		; ** update geometry from DDPT / 13.8
	xor ax,[cs:ageometry]
	xor ax,[cs:bgeometry]	; checksum the same? (a ^ b ^ a' ^ b' zero?)
	jnz flop_isflushed	; if not, FDGEOSYNC already flushed for us
	call flushone		; flush cache for freaky floppy
flop_isflushed:
	pop ax

	jmp UNSTACK_oldint	; let access happen without caching!!!
	; ( would be enough to invalidate only those cache entries for... )
	; ( ...which the freaky access could have caused inconsistencies. )

norwfloppy:			; *** End of floppy freak check.
			; add call meep here if you are REALLY curious
%ifdef DBG
		push word gtmsg	; DBG *offset*
		call meep	; DBG *** REALLY CURIOUS ***
%endif
	cmp ah,3
	jz near hdwrite	; copy to cache if write
	; reached only if AH is 2:
	jmp near hdread	; read, maybe from cache

; --------------

lbacheck:		; check if we can handle this kind of LBA call
	cmp dword [ds:si+4],-1
	jz short fishyLBA	; the buffer pointer is a 64bit flat
			; pointer rather than a DOS pointer -> FLUSH
	cmp dword [ds:si+12],0
	jnz short fishyLBA	; the sector number is above 4 G,
			; our 32bit LBA code cannot handle it: FLUSH
	test word [ds:si+2],0xff80
	jnz fishyLBA		; do not allow more than 0x7f sectors
			; in one call, at least not yet!
	push eax
	movzx eax,word [ds:si+2]	; number of blocks
	add eax,[ds:si+8]		; sector number (low part)
	pop eax
	jc short fishyLBA	; will hit the 4 G 32bit boundary
	jz short fishyLBA	; ... exactly, so we FLUSH
%ifdef DBG
		push word gtmsg	; DBG *offset*
		call meep	; DBG *** REALLY CURIOUS ***
%endif
	cmp ah,0x42
	jz near lbaread		; 64bit LBA read
	; cmp ah,0x43
	; jz near lbawrite	; 64bit LBA write
	; jmp UNSTACK_oldint
	jmp lbawrite		; if it is no read, it is a write!

fishyLBA:
		push word lbagenerr
		call meep	; warn user...
	jmp fishy		; go to FLUSH

; --------------

FDGEOSYNC:	; check for geometry change of drive DL (A: and B: only)
	cmp dl,1
	ja near fdgeonocheck	; only check if A: or B:
	push ax
	push bx
	push cx
	push di
	push es
	mov ah,8	; read geometry
	push dx		; save DL!
	call callold
	mov ch,dh	; number of heads (ignore cylinder count)
	pop dx		; ... restores DL!

	xor ax,ax
	mov es,ax
	les di,[es:0x78]	; *** load with int 1e vector (5/2004)
	; buggy BIOSes may set ES:DI to f000:???? or even not at all!!!
	mov al,[es:di+3]       ; get sector size code

	and cl,0x3f	; ignore cylinder count (for now)
	mov ah,0xe2		; "error message": wrong sector size
	cmp al,2		; 512 bytes per sector?
	jnz short fdgeofailure	; else stop caching floppies!
fdgeosecsizeokay:
%if 0				; 2008
	mov ah,0xe4		; "error message": too many heads
	cmp ch,1		; head count at most 2?
	ja short fdgeofailure	; else stop caching floppies!
%endif				; 2008
	mov ah,0xe6		; "error message": sect / track mismatch
	mov al,cl		; also for the error message
	cmp al,[es:di+4]	; are sectors / track in range?
	ja short fdgeofailure	; (block "actual value > DDPT allowed value")
	mov al,[es:di+4]	; *** use DDPT sectors / track ***
	; (not drive maximum but value set by DOS from boot sect. data!)

fdgeobiosbug:
	mov ah,0xe8		; "error message": invalid sect / track
	cmp al,6		; less than 120k in 1 sided 40 track case?
	jb short fdgeofailure
%if 0				; 2008
	cmp al,42		; more than 3.5M in 2 sided 80 track case?
	ja short fdgeofailure	; note that SETUP.ASM skips if above 1.44M!
%endif				; 2008
	mov ah,ch		; *** max head number, usually 2 ***
				; (taken from DH)
	cmp dl,1		; which drive?
	jz short fdgeoB		; beyond B: is skipped above already

fdgeoA:
	cmp [cs:ageometry],ax
	jz short fdgeoAkept
	call flushone		;  FLUSH, geometry of A: changed
	push word fdgeoupdateAmsg	; *** DEBUG GEOMETRY ***
	call meep			; *** DEBUG GEOMETRY ***
fdgeoAkept:
	mov [cs:ageometry],ax	; update geometry data for A:
	jmp short fdgeodone

fdgeoB:
	cmp [cs:bgeometry],ax
	jz short fdgeoBkept
	call flushone		;  FLUSH, geometry of B: changed
	push word fdgeoupdateBmsg	; *** DEBUG GEOMETRY ***
	call meep			; *** DEBUG GEOMETRY ***
fdgeoBkept:
	mov [cs:bgeometry],ax	; update geometry data for B:
	jmp short fdgeodone

fdgeofailure:
	or ah,dl	; add drive number to error code
	push word fdgeobugmsg
	call meep	; give error message
	cmp dl,1
	jb short fdgeoblockA
fdgeoblockB:
	and byte [cs:fddstat],0xfd	; SHUT DOWN caching of B:
	jmp short fdgeodone
fdgeoblockA:
	and byte [cs:fddstat],0xfe	; SHUT DOWN caching of A:

fdgeodone:
	pop es
	pop di
	pop cx
	pop bx
	pop ax
fdgeonocheck:
	ret

fdgeobugmsg:		db " STOP a floppy cache. AX=",0
fdgeoupdateAmsg:	db " A: flush, geometry=",0 ; *** DEBUG GEOMETRY ***
fdgeoupdateBmsg:	db " B: flush, geometry=",0 ; *** DEBUG GEOMETRY ***

; --------------

%ifdef FLOPCHGMSG
diskchangedmsg:		db " Flush: disk change. AH:DL=",0
%endif

; --------------

oldint_pure:	jmp far [cs:oldvec]

; --------------

callold:
        pushf	; calls the original int 0x13
	call far [cs:oldvec]
	ret

; --------------

i13retf:	; must use THIS to do the local stack handling properly!
		; new 8/2006: manipulate caller flags on stack, no RETF+2
	pushf
	cmp byte [cs:busy],0		; are we using the local stack?
	jz short i13r_nostack
	popf
	lss sp,[cs:othersp]		; continue with caller stack
	mov byte [cs:busy],0		; mark local stack as free
	pushf
i13r_nostack:				; stack: ourflags IP CS callerflags
%if 0
	push bp				; BP f1 IP CS f2
	mov bp,sp
	push ax
	and byte [bp+8],0xfe		; reset caller carry flag
	mov al,[bp+2]			; read our local flags
	and al,1			; AL is now 0 or 1: our carry flag
	add [bp+8],al			; copy carry to caller flags
	pop ax
	pop bp
	popf
%else
	popf				; optimized code from Arkady :-)
	push bp
	mov bp,sp			; BP IP CS callerflags
	rcr byte [bp+6],1		; carry->msb, callercarry->carry, ...
	rol byte [bp+6],1		; ourcarry->lsb, ...
	pop bp				; callercarry set to ourcarry :-)
%endif
	iret				; the safest way to return from int!

UNSTACK_oldint:				; close the local stack before
	pushf				; continuing with oldint!!!
	cmp byte [cs:busy],0		; are we using the local stack?
	jz short oldi_nostack
	popf
	lss sp,[cs:othersp]		; continue with caller stack
	mov byte [cs:busy],0		; mark local stack as free
	jmp short oldint_pure

oldi_nostack:
	popf
	jmp short oldint_pure

; --------------

	;   a (read long), c (seek), d (recalib+reset), 10 (ready?), 
	;   11 (recalib), 12 (ram diag), 13 (disk diag), 14 (ctrl diag)
	; e/f are r/w XT sector buffer, used for diagnostic and format...
	; 19: park - would be a good time to flush if we had a write cache
	; 1a: esdi format, but also harmless stuff. 1b...: mixed
	; 1f: syquest lock door... 20...: atapi stuff and mixed, very mixed!
	; 21/22: read/write multiple (ps/2): dh lsb are 2 more cyl bits!

	; 41... are the next REAL (generic) functions: int 13 ext
	; 42 - reading with 64bit LBA - is kind of harmless for us, ignored!
	; (43 write, 44 verify, 45 lock/unlock, 46 eject, 47 seek, 48 info,
	;  49 changed?, 4a..4d bootable cdrom, 4e set dma/pio)
	; above 4f is again very mixed

	; Trapped BIOS enhancer (EzDrive, OnTrack Disk Manager, SWBIOS...)
	; calls (now return CY) are: 13.ee.DL (+1024 cyl on next int13),
	; 13.ef.DL.cx (+cx cyl on next int13), 13.fe.DL (get cyl-1024 DX)
	; 13.f9.DL (inst check: DX msb set)
	; 13.ff.DL (inst check: AX aa55, ES:BX version string)
