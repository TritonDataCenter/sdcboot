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

%define XMSRANGECHK 1   ; XMS driver will fail with error A7 for off-
                        ; range copy, but THIS debug message is better.

	; XMS helper functions
	; copytoxms copies one sector from es:bx to XMS slot AX
	; copytodos copies one sector from XMS slot AX to es:bx

%ifdef DBGx
toxmsmsg	db 13,10,'esbx->xms.',0
todosmsg	db 13,10,'esbx<-xms.',0
%endif

%ifdef XMSRANGECHK
xmscopybugmsg	db " Bug! DOS"
xmsbugdirmsg	db "->"
		db "XMS.",0
%endif


			; new Jan 2002 - why did nobody abandon
			; the possibility to turn the A20 off
			; as soon as the 386 came up?????

A20KLUDGE:		; hangs if nothing reads the waiting data!
%ifndef SANE_A20
	pushf		; yes, that crappy thing finally got me:
	push ax		; the BOCHS(.com) ROMBIOS fails to check,
	sti		; but in 64 does not only need test 2/Z
a20k:	in al,0x64	; (can accept commands) BUT(!) also has to be
	test al,3	; test 1/Z (has -no- -other- data waiting on
	jnz a20k	; port 60) for command 0xd0 (read A20/...
	pop ax		; state), so iodev/keyboard.cc and
	popf		; bios/rombios.c are at war (in BOCHS) !
			; (which may crash, because int 15.87, the
			; copy function, uses set_enable_a20 ...)
%endif
	ret


	; XMS copy structure (0x10 in size):
	; D size (512), W source handle (0 for DOS), D source (linear or
	; pointer for DOS), W dest handle (...), D dest

copytoxms:	; copy 1 sector from es:bx to xms bin AX
	pushf

%ifdef DBGx
		push word toxmsmsg	; DBG
		call meep		; DBG
%endif

	pusha	; do not trust XMS
	push eax

%ifdef XMSRANGECHK
	cmp ax,[cs:sectors]
	jb toxms_inrange
	mov word [cs:xmsbugdirmsg],"->"
xmscopybug:
		push word xmscopybugmsg
		call meep
xmscpx:		jmp short wrxmsok	; well, NOT ok actually
toxms_inrange:
%endif

	movzx eax,ax		; clear high 16 bits
	shl eax,9		; assume 512 byte sectors!
	push eax		; C bin nr -> dest offs
	push word [cs:xmshandle]	; A dest: xms
	push es
	push bx			; 6 esbx source pointer
	push word 0		; 4 source: dos
	push dword 512		; 0 size: 512 (1 sector)
	mov si,sp		; our XMS copy structure on stack
	push ds		; (save)
	push ss
	pop ds		; ... stack again ...

		call A20KLUDGE	; Jan 2002
	mov ah,0x0b	; XMS copy command
		call far [cs:xmsvec]

	pop ds		; (restore)
	mov bp,ax		; (save)
	pop eax		; remove...
	pop eax		;   XMS copy...
	pop eax		;     structure from...
	pop eax		;       stack!
	mov ax,bp		; (restore)

	or ax,ax
	jnz wrxmsok

	; *** whoops: XMS copy returned an error in BL, BL trashed !
		mov al,bl	; for debugging
		mov ah,0x0b	; same
		push word xmserr
		call meep

wrxmsok:		; common tail, also used by copytodos
	pop eax
	popa
	popf
	ret

; ---------------------------------------------------------------

copytodos:	; copy 1 sector from xms bin AX to es:bx
	pushf

%ifdef DBGx
		push word todosmsg	; DBG
		call meep		; DBG
%endif

	pusha	; do not trust XMS
	push eax

%ifdef XMSRANGECHK
	cmp ax,[cs:sectors]             ; sector inside cache?
	jb todos_inrange
	mov word [cs:xmsbugdirmsg],"<-"
	mov byte [cs:xmscpx+1],0xfe	; create ENDLESS LOOP
	; (otherwise, undetected "disk" read error would be let through)
		jmp short xmscopybug	; abort copy attempt
todos_inrange:
%endif

	push es
	push bx			; C esbx dest pointer
	push word 0		; A dest: dos
	movzx eax,ax		; clear high 16 bits
	shl eax,9		; assume 512 byte sectors!
	push eax		; 6 bin nr -> source offs
	push word [cs:xmshandle]	; 4 source: xms
	push dword 512		; 0 size: 512 (1 sector)
	mov si,sp		; our XMS copy structure on stack
	push ds		; (save)
	push ss
	pop ds		; ... stack again ...

		call A20KLUDGE	; Jan 2002
	mov ah,0x0b	; XMS copy command
		call far [cs:xmsvec]

	pop ds		; (restore)
	mov bp,ax		; (save)
	pop eax		; remove...
	pop eax		;   XMS copy...
	pop eax		;     structure from...
	pop eax		;       stack!
	mov ax,bp		; (restore)

	or ax,ax
	jnz wrxmsok            ; jump to common tail

	; *** whoops: XMS copy returned an error in BL, BL trashed !
		mov al,bl	; for debugging
		mov ah,0x0b	; same
		push word xmserr
		call meep

	jmp short wrxmsok	; jump to common tail

