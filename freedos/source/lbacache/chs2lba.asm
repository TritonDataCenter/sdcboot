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




	; chs2lba.asm:
	; Uses the [geometry] table to offer both
	; CHStoLBA: which converts CXDH to EAX in context DL and
	; LBAtoCHS: which converts EAX to CXDH in context DL.

%ifdef DBG2
lbadbg          db 13,10,'CHStoLBA: cxdx->eax ',0
chsdbg          db 13,10,'LBAtoCHS: eaxdl->cxdx ',0
%endif

chsovermsg      db 13,10,'LBAtoCHS: impossible cylinder, cache OFF ',7,0
lbaovermsg      db 13,10,'CHStoLBA: impossible geometry, cache OFF ',7,0

; ---------------------------------------------------------------

ageoFLOPPY:             ; *** NEW 11/2002
	mov bx,0x0112   ; 2 heads, 18 sectors: typical 1440k floppy
	cmp dl,1        ; IMPORTANT bugfix 4/2003!
	jb ageoA
	jnz ageoknown   ; assume 1440k if neither A: nor B: !
ageoB:  mov bx,[cs:bgeometry]   ; B:
	jmp short ageoknown
ageoA:  mov bx,[cs:ageometry]   ; A:
	jmp short ageoknown

CHStoLBA:               ; convert CX DH in context DL to EAX
	pushf           ; flags and everything else are preserved!

%ifdef DBG2
		push ax ; DBG
		mov ax,cx               ; DBG
		push word lbadbg        ; DBG *offset*
		call meep               ; DBG
		mov ax,dx               ; DBG
		push word colonmsg      ; DBG
		call meep               ; DBG
		pop ax  ; DBG
%endif

	push bx
	push ecx
	push edx        ; because we use mul
	cmp dl,0x80
	jb ageoFLOPPY
	mov bl,dl       ; use drive number ...
	and bx,7        ; *** ignore all other bits of drive number!
	add bx,bx
	mov bx,[cs:geometry+bx] ; bl is now max sector number,
			; bh max head number
ageoknown:
	cmp dh,bh       ; head number too high?
	ja lbainvalid   ; SPOOK
	push cx
	and cl,63       ; sector number...
	cmp cl,bl       ; ...too high?
	pop cx
	ja lbainvalid   ; SPOOK
	cmp bl,7
	jb lbainvalid   ; do not believe in less than SEVEN sec/cyl
	cmp bh,2-1
	jb lbainvalid   ; do not believe in less than TWO heads
	
	  push cx       ; sector number for later
	xor eax,eax
	mov ax,cx       ; start extracting the cylinder...
	xchg al,ah
	shr ah,6        ; now ax is the cylinder number
	movzx ecx,bh
	inc ecx         ; now ecx is numheads (max 0x100)
	push dx ; YUCK, forgot to save this over mul...
	mul ecx         ; cyl * numheads
	pop dx  ; UN-YUCK... :-) (EDXhi no longer needed)
	movzx ecx,dh    ; now ecx is selected head
	add eax,ecx     ; (cyl * numheads) + head
	mov cl,bl       ; now ecx is sectors per track
	mul ecx         ; ((cyl * numheads) + head) * sectorspertrack
	  pop cx        ; stored sector number
	and cx,63       ; the sector... (ecx hi still 0, ok)
	dec ecx         ; make it 0 based! (now in ecx...)
	add eax,ecx     ; now we have the LBA sector number in EAX !
	jmp short lbavalid      ; everything ok

lbainvalid:             ; [geometry] did not fit the given CHS,
			; we are in big trouble (H/S overflow!)
		mov ax,bx       ; the geometry
				; (too lazy to display used H/S)
		push word lbaovermsg
		call meep       ; complain
		mov word [cs:running],2 ; SHUT DOWN!
	mov eax,-2      ; sector number -2: error (-1 is empty bin)

lbavalid:

%ifdef DBG2
		push eax                ; DBG
		shr eax,16              ; DBG
		push word colonmsg      ; DBG
		call meep               ; DBG
		pop eax                 ; DBG
		push word colonmsg      ; DBG
		call meep               ; DBG
%endif

	pop edx
	pop ecx
	pop bx
	popf
	ret

; ---------------------------------------------------------------

bgeoFLOPPY:             ; *** NEW 11/2002
	mov bx,0x0112   ; 2 heads, 18 sectors: typical 1440k floppy
	cmp dl,1        ; IMPORTANT bugfix 4/2003
	jb bgeoA
	jnz bgeoknown   ; assume 1440k if neither A: nor B: !
bgeoB:  mov bx,[cs:bgeometry]   ; B:
	jmp short bgeoknown
bgeoA:  mov bx,[cs:ageometry]   ; A:
	jmp short bgeoknown


LBAtoCHS:               ; convert EAX in context DL to CX DH
	pushf           ; flags and everything else are preserved!
	push eax

%ifdef DBG2
		push eax        ; DBG
		push ax         ; DBG
		shr eax,16              ; DBX
		push word chsdbg        ; DBG *offset*
		call meep               ; DBG EAXhi
		pop ax          ; DBG
		push word colonmsg      ; DBG
		call meep               ; DBG EAXlo
		mov ax,dx               ; DBG
		mov ah,00               ; DBG
		push word colonmsg      ; DBG
		call meep               ; DBG DL
		pop eax         ; DBG
%endif

	push bx
	push ecx
	push edx        ; because we use mul
	cmp dl,0x80
	jb bgeoFLOPPY
	mov bl,dl       ; use drive number ...
	and bx,7        ; *** ignore all other bits of drive number!
	add bx,bx
	mov bx,[cs:geometry+bx] ; bl is now max sector number,
			; bh max head number
bgeoknown:
	movzx ecx,bl    ; sectors per track
	xor edx,edx
	div ecx         ; value (/ and %) sectors -> sector, value
	mov bl,dl       ; store SECTOR number, trash bl
	inc bl          ; ... value is now (cyl*numheads)+head
	movzx ecx,bh    ; max head number
	inc ecx         ; numheads
	xor edx,edx
	div ecx         ; value (/ and %) numheads -> head, cylinder
	mov bh,dl       ; store the HEAD, trash bh
	cmp eax,1024
	jb chscylok     ; else we are in big trouble (CHS overflow!)

chscylinvalid:
		push ax
		shr eax,16
		push word chsovermsg
		call meep               ; EAXhi
		pop ax
		push word colonmsg      ; ':' 
		call meep               ; EAXlo
		mov word [cs:running],2 ; SHUT DOWN !

	mov ax,1023     ; max cylinder
	mov bx,0        ; BH: head 0, BL: impossible sector 0

chscylok:
	xchg al,ah      ; now AH has the lower 8 bits
	shl al,6        ; now AL has the high 2 bits on top
	or al,bl        ; insert sector -> cx prepared
	shl eax,8       ; see below
	mov al,bh       ; insert head -> for dh

	pop edx
	pop ecx
	pop bx
	mov dh,al       ; get HEAD
	shr eax,8
	mov cx,ax       ; get CYLINDER and SECTOR

%ifdef DBG2
		push ax         ; DBG
		mov ax,cx               ; DBG
		push word colonmsg      ; DBG
		call meep               ; DBG CX
		mov ax,dx               ; DBG
		push word colonmsg      ; DBG
		call meep               ; DBG DX
		pop ax          ; DBG
%endif

	pop eax
	popf
	ret

