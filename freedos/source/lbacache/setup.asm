 ; This file is part of LBAcache, the 386/XMS DOS disk cache by
 ; Eric Auer (eric@coli.uni-sb.de), 2001-2005, 2008

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
 ; Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA
 ; (or try http://www.gnu.org/licenses/licenses.html at www.gnu.org).

; LBAcache - a hard disk cache based on XMS, 386 only,
; and aware of the 64bit LBA BIOS Int 13 Extensions.
; GPL 2 software by Eric Auer <eric@coli.uni-sb.de> 2001-2005




	; .8086 (Warning: nasm does not have this directive,)
	; (so we must simply be careful "by hand"... It does)
	; (have BITS 16 and BITS 32 to tell CS type, though.)

	; main install and setup routine follows (starts with
	; 8086 compatible code to detect 386, then does a check
	; for the existence of XMS, allocates the cache there,
	; and stores the drive geometry information for drives
	; 0x80 .. 0x87 for CHS <-> LBA conversion
	; install: is the entry point.
	; jumps to resinst: at the end.
	; FLOPPY: added change line detection -> fddstat word
	; *** NEW 11/2002: lba bit for each drive, 8 drives ..0x87
	; *** NEW 11/2002: honor amount of available RAM if DOS 5+
	; (should also honor it if < DOS 5 in .com mode...)

%define INTSTACKSIZE 384	; 300 or 320 before 4/2008

install:			; * Main bootstrapping code *
;	cli			; cli not really needed...
	mov word [tsrsize],0	; default: do not stay resident

	mov es,[cs:0x2c]	; environment segment
		; see Ralf Browns IntList table 1379 (PSP: 1378)
	mov ah,0x49
		int 0x21	; free the environment (ignore errors)

	push cs                 ; set segments to sane values (again)
	pop es
	push cs
	pop ds
	push cs
	pop word [cs:localss]	; initialize local stack segment (7/2004)

	mov bx,0x1000		; try to resize to 64k, we never need more.
	mov ah,0x4a		; resize DOS memory block ES (ourselves)
	int 0x21		; shrink ourselves, ignore errors for
				; the "grow ourselves" case. (7/2004)

; -------------

; test if we have 386 or better: 
	pushf   ; save flags
		xor ax,ax
		push ax
		popf	; try to clear all bits
		pushf
	        pop ax
	and ax,0f000h
	cmp ax,0f000h
	jz noinst1	; 4 msb stuck to 1: 808x or 80186
		mov ax,0f000h
		push ax
		popf	; try to set 4 msb
		pushf
		pop ax
	test ax,0f000h
	jz noinst1	; 4 msb stuck to 0: 80286
	jmp short okinst1

noinst1:	; failed, no 386 found
	push bx
	push si
	cld			; flags are saved :-)
	mov si,err386
		call strtty	; complain: no 386 found
	mov si,hello
		call strtty	; show banner
	pop si
	pop bx

	popf
	jmp quitinst

	; .386 ( Warning: nasm does not have this directive, )
	; (but remember that you may use 80386 code below :-))

okinst1:
	popf	; good, it is a 386, now restore flags

; -------------

	pusha			; <<< after this point, we must use
	push ds			; <<< quitpop, not only quitinst...
	cld			; flags are saved :-)

	mov si,hello
		call strtty	; show banner

; -------------

	call parsecommandline	; the COMMAND LINE should be parsed as
				; soon as possible, so we do it now.


; -------------

	call uncache		; new 01/2002: call the part that
				; implements the formerly separate
				; UNCACHE functionality (sync, info,
				; stop) (must be -before- we get an
				; XMS handle)
	mov ax,[ds:args]	; bit mask of found command line args
	test ax,255		; low byte is for non-TSR args
	jz go_tsr		; exit here if any uncache thing was
				; our mission - else do cache thing!
	jmp quitpop		; leave this here! <<< includes popa...!

go_tsr:

; -------------

; test if we have XMS. allocate XMS. fail if none/not enough found.

	mov ax,0x4300
		push ax	; for debugging: put "why" code on stack
	mov bl,-1		; clear errorcode value
		int 0x2f	; xms installation check
	cmp al,0x80
	jnz short noxms		; xms not present
	mov ax,0x4310
	push es
		int 0x2f	; get xms call vector (may clobber regs)
	mov [xmsvec],bx
	mov [xmsvec+2],es
	pop es
		pop ax	; for debugging: remove "why" code
	mov ah,8
		push ax	; for debugging: new "why" code
		call far [xmsvec]	; check amount of free XMS
	or ax,ax
	jz short noxms		; errorcode in bl if ax zero


	; *** round up sector count / XMS size FIRST ***
	; smalloc can shrink table later, but table must be bigger
	; bigger than actually available XMS handle can handle!
	mov ax,[ds:sectors]
	or al,al		; round value?
	jz roundedsectvalid	; then leave as is
	mov al,255		; else round up to the next
	inc ax			; multiple of 128k (256 sectors)
	jnz roundedsectvalid	; no overflow? Okay then!
	mov ax,0xff00		; Limit value otherwise.
roundedsectvalid:
	mov [sectors],ax

	mov bx,[ds:sectors]	; ds: prefix needed, see above
	inc bx			; round up
	shr bx,1	; amount of XMS we need, kbyte / use 512 by sectors
	cmp dx,bx		; DX total, AX biggest chunk (kbyte)
	jb short noxms
	cmp ax,bx
		pop ax	; for debugging: remove "why" code
		push word 0xfe00	; for debugging: new "why" code
	jb short noxms		; not enough XMS free
		pop ax	; for debugging: remove "why" code
	mov ah,9
		push ax	; for debugging: new "why" code
	mov dx,bx
		call far [xmsvec]	; alloc DX kbyte for us
	or ax,ax
	jz short noxms		; errorcode in bl if ax zero
		pop ax	; for debugging: remove "why" code
	mov [xmshandle],dx	; our handle

	mov ah,0x0c		; LOCK handle (NEW 6/2005)
		push ax	; for debugging: new "why" code
		call far [xmsvec]	; lock that XMS handle
	or ax,ax		; if AX=1, success: locked at linear DX:BX
	jz short noxms		; errorcode in bl if ax zero
		pop ax	; for debugging: remove "why" code
				; end of NEW 6/2005 part

	; to free the memory, use function 0x0a and handle in DX
	; only function used by the TSR is copy, function 0x0b.
	jmp short alloclowstack

noxms:	
		pop ax		; meep displays errorcode from bl...
		mov al,bl	; ...and the "why" code from stack.hi
		push word xmserr
		call meep	; give feedback
				; *** meep is now redirectable
	mov si,xmserr2		; no or not enough XMS found
		call strtty	; minimalistic error message
				; (*** can be redirected)
; --- instfailed:
	jmp quitpop		; <<< includes popa...!

; now we have the XMS we need, after flushing the status table and
; finding the geometry we can hook int 0x13 and return...

; -------------

alloclowstack:
	; Stack allocation new 7/2004
	; (no need to preserve AX BX CX ... here)

	test byte [cs:tuneflags],4	; using separate LOW RAM stack?
	jz nosepstackcheck
	mov ax,cs		; are we in LOW RAM anyway?
	cmp ax,0x9000		; 9fff would be safe unless > 640k base
	jna nosepstack

; --- yessepstack:
	mov ax,0x5800		; get malloc strategy
	int 0x21
	mov cx,ax		; save
	mov ax,0x5801		; set malloc strategy
	mov bx,0		; "low memory, first fit"
	int 0x21

	mov ah,0x48		; allocate DOS memory
	mov bx,INTSTACKSIZE/16	; size of our "low RAM" stack
	int 0x21		; (not interested in free space / bx)
				; returns segment in AX
	pushf
	push ax
	mov ax,0x5801		; (re)set malloc strategy
	mov bx,cx		; restore
	int 0x21
	pop ax
	popf
	jc sepstackfail
	cmp ax,0x9f80		; low enough?
	ja sepstackfail
; --- sepstackok
	mov [cs:localss],ax		; new segment
	mov word [cs:localsp],INTSTACKSIZE

	push ds
	mov ds,ax			; our stack segment
	xor bx,bx
	mov ax, "  "			; fill area (nice for debugging)
zapstack:
	mov [ds:bx],ax
	inc bx
	inc bx
	cmp bx,INTSTACKSIZE		; size of our "low RAM" stack
	jb zapstack
	pop ds

	mov [cs:otherss],ss	; *** NEW 7/2004: use "low RAM" stack for
	mov [cs:othersp],sp	; **   drive detction stuff - otherwise
	lss sp,[cs:localsp]	; *     SCSI BIOS might fail int 13.8...!

	jmp short nosepstackcheck

sepstackfail:
	mov si,nolowmemmsg
		call strtty	; tell that TUNS request failed
nosepstack:			; no separate stack MCB, reset flag!
	and byte [cs:tuneflags], 0xfb	; not(4)
nosepstackcheck:

; -------------

findgeom:			; *** NEW 11/2002: loop, havelba BITS

	mov si,GEOmsgstart	; %
	call strtty		; % tell user that drive detection starts

	mov word [cs:havelba],0	; start with assuming no LBA
		; drvselmask value comes from parsecommandline here
	mov di,geometry		; *** start creating table HERE
	cld
	mov si,0x0180		; *** start bitmask / drive C: 0x80

findnextgeom:

				; *** Lazy checking is DISABLED by
				; *** default, because the user may
				; *** modify the drvselmask with a
				; *** debugger later!
%ifdef LAZYDISKCHECKING
	mov dx,si		; *** load drive DL
	test [cs:drvselmask],dh	; *** did the user WANT us to cache?
	jnz fglearngeom		; *** otherwise SKIP geometry check!
	jmp fgdrivedone
fglearngeom:
%endif

	mov ax,0x4100
	mov bx,0x55aa
	mov dx,si		; *** load drive DL
		int 0x13	; check if BIOS int 13 extension is
				; present for this drive (DL)
				; *** modifies AX BX CX DH
	cmp bx,0xaa55		; (version info in AH and DH ignored)
	jnz nolbabios		; install check failed
	test cx,1		; 1 LBA 2 removable 3 flatmem/edd
	jz nolbabios
	mov dx,si		; ***
	or [cs:havelba],dh	; *** note that LBA was found
	jmp short findgeom2	; ok, LBA BIOS present

nolbabios:
	test si,0x0000		; ************************
	jz findgeom2		; NEW: error message for ... only
	push si
	mov si,errnolba		; error message...
		call strtty
	pop si
	; *** jmp instfailed	; having no LBA is no longer fatal :-)
	; *** plus we must not leave here without cleaning up stack!

; -------------

findgeom2:
	mov ah,08		; read geometry and number of hard disks
	push es
	push di
	mov dx,si		; *** load drive DL

	push dx			; %
	and dl,15		; % low 4 bits of BIOS disk number
	add dl,'0'		; % convert to ASCII
	mov [cs:GEOmsgdrv],dl	; % prepare disk description message
	pop dx			; %

		int 0x13	; find geometry (bl type, dl drives, dhcx geo)
	pop di			; (esdi -> drive param tab, only for floppies)
	pop es			; (also modifies ax)
	jc nodrive
	or dl,dl
	jz nodrive
	and cl,63
	mov al,cl		; save max sector num
	mov ah,dh		; save max head num
	mov [cs:di],ax		; save to geometry list
	call print_geo
	inc di
	inc di
	jmp short fgdrivedone	; *** removed check for "beyond DL"

print_geo:			; Show geometry: AH heads, AL sectors
				; (BIOS drive number string: GEOmsgdrv-1)
	push ax			; %
	mov al,ah		; % fetch max head number
	mov ah,0		; %
	inc ax			; % turn into head count
	or ah,ah		; % 256 heads?
	jnz geo256		; %
geo100:	cmp al,100		; % more than 2 digits?
	jb geo99h		; % otherwise we are done
	sub al,100		; % get closer to 2 digits case
	inc ah			; % increments 100s digit
	jmp short geo100	; %
geo256:	mov ax,0x200+56		; % case max head number 0xff...
geo99h: mov cl,ah		; % save 100s digit
	aam			; % ah=al div 10, al=al mod 10
	shl ah,4		; % prepare merge (286+ opcode)
	or al,ah		; % merge BCD
	mov ah,cl		; % restore 100s digit
	push word GEOmsgH	; %
geo_string_1:
	call meep		; % show drive number and head count
	pop ax			; %
	;
	push ax			; %
	mov cl,4		; %
	and ax,0x3f		; % get sectors per track count
	aam			; % ah=al div 10, al=al mod 10
	shl ah,cl		; % prepare merge
	or al,ah		; % merge BCD
	mov ah,0		; % clear high part
	push word GEOmsgS	; %
geo_string_2:
	call meep		; % show sectors per track
	pop ax			; %
	ret


nodrive:			; drive not cacheable
	inc di			; do not write geometry
	inc di			; do not write geometry
	mov dx,si		; *** load bitmask (and drive)
	not dx
	and [cs:havelba],dh	; *** remove LBA flag again!
	and [cs:drvselmask],dh	; *** disable cache for this drive

fgdrivedone:
	mov dx,si		; *** load drive and bitmask
	inc dl			; next drive
	add dh,dh		; next bitmask
	mov si,dx		; *** write back drive and bitmask!

	cmp dl,0x87
	ja alldrives		; done for all hard disks
	jmp findnextgeom	; *** loop for drives 0x80..0x87

alldrives:			; done with all HARD DISK drives
	mov si,GEOmsgend
		call strtty	; tell that we checked all drives

; -------------

				; NEXT, check FLOPPY drives A: and B:
findgeom3:			; A:
	push es			; esdi: drive param table (ignored)
	push di			; cx dx: geometry
	push bx			; bx: drive type (floppies only)
	; matching POP: none_fdd_used

	or word [cs:fddstat],0x0300	; potential drives: A: and B:

	mov ax,0x0800		; get drive params (geometry...)
	xor dx,dx		; A:
	xor bx,bx
	xor cx,cx
		int 0x13	; modifies AX BX CX DX ES DI
	jc short no_drv_a	; no A installed
%if 0				; 2008: allow exotic geometries
	cmp dh,1
	ja no_drv_a		; > 2 head floppies are never cached
	and cl,63		; number of cyls does not matter
	cmp cl,42		; never cache if > 42 sectors / cyl, which
				; is "2.88 MB + X" (was: 18, for 1.44 MB)
	ja no_drv_a		; this excludes some xlarge formats
	cmp bl,4		; 2.88 M is 36 sec/cyl, but to be sure
	ja no_drv_a		; 1 360k 2 1200k 3 720k 4 1440k (16 atapi)
%endif				; 2008
	mov ch,dh		; *** NEW 11/2002
	mov [cs:ageometry],cx	; *** NEW 11/2002 LO: sectors HI: heads
	mov ax,0x1500		; get disk type
	xor dx,dx		; A:
		int 0x13	; for floppies, only AX modified (not CX DX)
	jc no_drv_a
	cmp ah,2		; we allow only "removable with change line"

	jz findgeom4		; "useable diskette drive" :-)

	; start 2008 changes
	cmp byte [cs:ageometry+1],2	; at most 2 heads?
	jb no_drv_a0		; then abort if no change detection
	mov byte [cs:flopAused+2],'+'	; hint user: "huge A:" (ZIP, USB)
	jmp short findgeom4

no_drv_a0:
	push ds
	xor ax,ax
	mov ds,ax
	mov ax,0xf000
	cmp ax,[ds:0x398+2]
	jnz noDosemu
	mov ds,ax
	cmp word [ds:0xfff5+6],0x3339	; "02/25/93" for all DOSEMU versions
	jnz noDosemu
	mov ah,0
	int 0xe6	; DOSEMU install check: returns aa55
noDosemu:
	pop ds
	cmp ax,0xaa55	; also BX, CX = dosemu version
	jnz no_drv_a
	mov byte [cs:flopAused+2],'v'	; hint user: virtual A: (DOSEMU)
	or word [cs:havelba],0x8000	; set flag: we are in DOSEMU
	jmp short findgeom4
	; end 2008 changes

no_drv_a:
	and word [cs:fddstat],0xfefe	; disable caching of A:

	; note that B: is probably never USB / ZIP / ... - kept 2006 code
findgeom4:			; B:
	mov ax,0x0800		; get drive params (geometry...)
	xor cx,cx
	xor dx,dx
	inc dx			; B:
	xor bx,bx
		int 0x13	; see above: only allow floppy sized CHS
	jc no_drv_b
	cmp dh,1
	ja no_drv_b
	and cl,63
	cmp cl,42		; was 18 ... see above. [5/2004]
	ja no_drv_b
	cmp bl,4
	ja no_drv_b
	mov ch,dh		; *** NEW 11/2002
	mov [cs:bgeometry],cx	; *** NEW 11/2002 LO: sectors HI: heads
	mov ax,0x1500		; get disk type
	xor dx,dx
	inc dx			; B:
		int 0x13	; for floppies, only AX modified (not CX DX)
	jc no_drv_b
	cmp ah,2		; we allow only "removable with change line"
	; start 2008 changes
	jz findgeom5
	test word [cs:havelba],0x8000	; A: already set DOSEMU flag?
	jz no_drv_b		; else accept no drives w/o change line
	
	mov byte [cs:flopBused+2],'v'	; hint user: virtual B: (DOSEMU)

		; we could say "useable drive B: found" here

	; end 2008 changes

	jmp short findgeom5

no_drv_b:
	and word [cs:fddstat],0xfdfd	; disable caching of B:

findgeom5:				; A: B: done, opt. show messages now
	test word [cs:fddstat],0x0300	; anything useable?
	jnz some_fdd_found		; (0x0003 would test: ...enabled?)
	mov si,errnofdd	
		call strtty	; warn user: no cacheable floppies

some_fdd_found:

%ifdef FORCEFDD
		or word [cs:fddstat],0x0003
		; FORCEFDD caches floppies even if they have
		; no change line and even w/o the FLOP argument!
	mov si,errforcefdd
		call strtty	; warn user: special DEBUG VERSION
	jmp short forcedfdd
errforcefdd:	db 13,10,13,10
	db "*** DEBUG VERSION: Always caches floppies, even",13,10
	db "*** unchangeable, nonexisting, wrong size ones!",13,10,13,10,0
forcedfdd:
%endif

	mov ax,[cs:fddstat]		; floppy caching enable flags
	test ax,0x0003			; anything really used?
	jz none_fdd_used

; //	push ax
		mov si,flopusedmsg
		call strtty		; announce floppy list
; //	pop ax
; //	push ax
	test al,1			; A: cached?
	jz none_a_used
	mov word [cs:geo_string_1-2],flopAused		; "A: ["
	mov word [cs:geo_string_2-2],flopgeomsg2	; "x"
	push ax
	mov ax,[cs:ageometry]
	call print_geo
	pop ax
	mov si,flopgeomsg3				; "] "
	call strtty

none_a_used:
; //	pop ax
	test al,2			; B: cached?
	jz none_b_used
	mov word [cs:geo_string_1-2],flopBused		; "B: ["
	mov word [cs:geo_string_2-2],flopgeomsg2	; "x"
	push ax
	mov ax,[cs:bgeometry]
	call print_geo
	pop ax
	mov si,flopgeomsg3				; "] "
	call strtty

none_b_used:
		mov si,crlfmsg
		call strtty		; CRLF

none_fdd_used:

	pop bx
	pop di		; * swap fixed 8/2003 (match findgeom3)
	pop es		; * swap fixed 8/2003 (match findgeom3)
			; * (no problem as we re-load es:bx from cs:pb anyway)

; -------------

	test byte [cs:tuneflags],4	; *
	jz nolsfindgeoend		; **
	lss sp,[cs:othersp]		; *** NEW 7/2004: back to normal stack
	mov dword [cs:xmsSZmsg4],0 + ": "	; chop "...and stack" message
nolsfindgeoend:

	jmp short malloc

smallocgiveup:
	mov si,mempanicmsg	; not enough RAM even for minimal size.
		call strtty	; give up and show a message
	jmp quitpop		; <<< includes popa...!

smalloc:
	sub word [sectors],256	; use quite a bit less memory,
				; ... and then TRY AGAIN !
	cmp word [sectors],256	; less than 128k is not okay.
	jb smallocgiveup
	mov si,dotmsg		; string with a single dot
		call strtty	; print "." each time you shrunk

malloc:				; allocate memory for us and the table
	mov ax,[ds:sectors]		; check size requested...
		call telltabsize	; calculate tab size
	jc smalloc			; was far too big
	add ax,table+15			; *offset*
	jbe smalloc			; reduce + TRY again (jc/jz)

mallocstack:
	test byte [cs:tuneflags],4	; using separate "low RAM" stack?
	jnz nomallocstack		; new case distinction: 7/2004
	; IFZ (if no separate stack), allocate stack in our own CS instead:

	add ax,INTSTACKSIZE		; add some bytes for stack
	pushf
	push ax
	and ax,0xfffc			; dword align
	mov [localsp],ax		; local stack will be there
	pop ax
	popf
	jbe smalloc			; reduce + TRY AGAIN ! (jc/jz)

nomallocstack:
	push ax				; SP could have been 0x(1)0000
	cmp ax, sp			; SP marks the RAM limit for .com!
	pop ax
	jae smalloc			; *** Too big for here?

	mov [tsrsize],ax		; store size of resident LBAcache

	mov ax,[ds:sectors]
	push cx				; %
	mov cl,1+10			; % 2 sectors/kB, 1<<10 kB/MB
					; % (sector size fixed at 512!)
	shr ax,cl			; % cache can be up to 32 MB big
	aam				; % AAM: ah=al div 10, al=al mod 10
	add ax,'00'			; %
	xchg al,ah			; % make 10s display left of 1s
	cmp al,'0'			; % leading 0 ?
	jnz nosuppxmszero		; % if yes, suppress
	mov al,' '			; %
nosuppxmszero:				; %
	mov [xmsSZmsg2],ax		; % megabyte part (high=1s, low=10s)
	mov ax,[ds:sectors]		; % now figure out the decimals
	mov cl,1+7			; * 2 sectors/kB, units of 128k
	shr ax,cl			; %
	and ax,7			; * mask out multiples of 1024k
	pop cx				; %
	push bx				; %
	add ax,ax			; %
	mov bx,ax			; %
	mov ax,[octolist+bx]		; % translate to '00' '12' '25'...
	mov [xmsSZmsg3],ax		; % decimals part (2-3 digits)
	mov al,' '			; * if multiple of 1/4 MB, 3rd decimal
	test bx,2			; * digit is 0, else it is 5.
	jz xms2decimals			; *
	mov al,'5'			; *
xms2decimals:				; *
	mov [xmsSZmsg3+2],al		; store 3rd decimal digit
	pop bx				; %

	mov si,xmsSZmsg
		call strtty		; % show size of XMS alloc, announce
					; % showing of CS (DOS RAM) alloc size
	mov ax,[ds:tsrsize]		; end of used part of our CS

	push dx				; %
	push bx				; %
	mov si,drvSZmsgend+5		; % at most 5 digits
drvszhex2decloop:			; %
	dec si				; % move cursor left for next digit
	xor dx,dx			; % 
	mov bx,10			; % convert to decimal
	div bx				; %
	add dl,"0"			; % remainder becomes low digit
	mov [cs:si],dl			; %
	test ax,ax			; % any digits left?
	jnz short drvszhex2decloop	; %

	pop bx				; %
	pop dx				; %

	; % si pointing to start of decimal number now!
		call strtty		; "????? bytes" + CRLF

; -------------

hookint13:				; hook int 0x13 - we can
	push eax			; ONLY do this already here
	push es 			; because [running] is not set
	xor ax,ax			; yet! Table still unflushed!
	mov es,ax
	mov eax,[es:0x13+0x13+0x13+0x13]        ; read old vector
	mov [oldvec],eax        ; save it for chain and call and uninstall
	mov ax,cs
	shl eax,16                              ; new vector: segment
	mov ax,int13new                         ; * new vector: offset
%ifdef PRETENDER
	; leave original int alone and use a separate api on int 0xea !
        mov [es:0xea+0xea+0xea+0xea],eax        ; hook (no cli, atomic)
%else
        mov [es:0x13+0x13+0x13+0x13],eax        ; hook (no cli, atomic)
%endif
        pop es
        pop eax


; -------------

	jmp resinst	; jump out of the way, rest of inst will be
			; overwritten with the status table!
	; will do: call flush, pop ds, popa, mov word [cs:running],1
	; and jmp nix (to return to the device handler...).

nolowmemmsg	db "No low DOS RAM free - using normal stack.",13,10,0
	; new 7/2004

flopusedmsg	db " Caching floppy drive(s): ",0
flopAused	db "A: [",0
flopBused	db "B: [",0
flopgeomsg2	db "x",0
flopgeomsg3	db "] ",0

dotmsg		db ".",0	; indicator for "reducing memory"

xmserr2		db ' Not enough free XMS memory.'
		db 13,10,0
err386		db ' This software needs at least an 80386 CPU'
		db 13,10,'Check PCmag DCACHE for a free PC XT cache.'
		db 13,10,0
errdrv  	db 'No hard disks installed.'
        	db 13,10,0
errnolba        db 'No LBA BIOS found, using CHS.'	; "translating"!
		db 13,10,0
errnofdd        db ' [No floppy cache: no change lines]'
		db 13,10,0

; spismsg	db " SP=",0

GEOmsgstart	db "Detecting harddisks: ",0
GEOmsgH		db 13,10,"  disk 0x8"
GEOmsgdrv	db "0 heads=",0
GEOmsgS		db " sectors=",0
; GEOmsgend	db " Harddisks checked. Checking floppy now.",13,10,0
GEOmsgend	db " [done]",13,10,0

xmsSZmsg	db "XMS allocated: "
xmsSZmsg2	db "00."
xmsSZmsg3	db "000 MB, driver size with tables"
xmsSZmsg4	db " and stack: ",0	; <-- or ": ",0 if no stack...
		; *** xmsSZmsg and drvSZmsg are one long string together
octolist	db "0012253750627587"
		; 000 125 250 375 ... (1/8ths of a megabyte)
drvSZmsgend	db "_____ bytes.",13,10,0

mempanicmsg	db "Not enough free DOS RAM!",13,10,0
