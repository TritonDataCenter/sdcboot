; GRAPHICS tool for FreeDOS - GPL by Eric Auer eric@coli.uni-sb.de 2003/2008
; Please go to www.gnu.org if you have no copy of the GPL license yet.

; those functions set the palette array to appropriate grayscale
; values in 0..255 range and may destroy all non-segment registers.
; weights based on DOSEMU /src/env/video/dosemu.c, adjusted for 0..255.

cgapalette:
	mov bx, 0 | (85 << 8)		; 4 grays
	mov cx, 171 | (255 << 8)	; 4 grays
	cmp byte [ds:gfxmode],5
	jz short cgapalfound
	push ds
	mov ax,0x40
	mov ds,ax
	mov al,[ds:0x66]	; CGA palette (set with int 10.0b...)
		; RBIL ports.txt tells that corresponding port 0x3c9
		; only exists in CGA and CGA compatible EGA/VGA clones.
	pop ds
	; SET BACKGROUND COLOR TO BLACK /
	; TRANSPARENT UNLESS /B FLAG GIVEN:
	mov ah,[cs:cgaback]
	test ah,1
	jz cganoback	; ignore background, assume black
	test al,1	; blue of background set?
	jz cganoblueback
	add bl,28	; add blue to background
cganoblueback:
	test al,2	; green of background set?
	jz cganogreenback
	add bl,150	; add green-1 to background
cganogreenback:
	test al,4	; red of background set?
	jz cganoredback
	add bl,77	; add red to background
cganoredback:
cganoback:
	; bl now contains background color (in CGA mono modes,
	; the FOREGROUND color can be found in this way, but we
	; just assume white on black in CGA mono...!)
	mov bh,151	; gray of green
	mov cl,77	; gray of red
	mov ch,255-28	; yellow, not white
	test al,0x20	; C/M rather than G/R ?
	jz cgapalfound
	mov bh,151+28	; gray of cyan
	mov cl,77+28	; gray of magenta
	mov ch,255	; actually white
cgapalfound:
	mov al,[cs:inverse]
	cmp al,1
	jnz cganoninverse
	not bx		; inverse printing
	not cx		; inverse printing
cganoninverse:
	mov [cs:palette],bx
	mov [cs:palette+2],cx
	jmp economy

; ------------

egapalette:		; this is probably *** broken ***:
			; VGA uses EGA colors only as index into DAC (?),
			; EGA BIOSes do not support this call.
	; direct read: read 3ba/3da to dummy, write 3c0 with index,
	; read value from 3c1 (only possible in VGA)... so EGA has
	; unreadable palette anyway? So we fill the buffer with defaults
	; before calling int 10, to have some data even if this fails.
	  push es
	mov ax,cs
	mov es,ax
	;
	mov di, palette
	mov cx,16
	xor ax,ax	; color 0 is 0.
	; (old: inc by 17 until > 255, no EGA -> grey translation.)
egapalfillloop:
	mov [cs:di],ax
	inc di
	inc ax		; correct
	cmp ax,8	; colors are 0..7, 38..3f, with 14 replacing 6.
	jb egapalfillloop
	or ax,0x38
	cmp ax,0x3f
	jbe egapalfillloop
	mov byte [cs:palette+6],0x14	; adjust brown / yellow
	;
	mov bx, palette
	mov ax,0x1009	; read palette (*** VGA in EGA mode ***)
	int 0x10	; target is es:bx (16 colors plus border color)
	  pop es
	mov bx, palette	; if int 10.1009 has failed, EGA default palette
egaxlate:		; is still in the buffer. In any case, translate.
	mov al,[cs:bx]
	mov ah,0	; black plus something
	mov cx,1	; lowest bit first
	mov si, egapalbits
egabitxlate:
	test al,cl
	jz egaunsetbitxlate
	add ah, [cs:si]	; add the part of one set bit
egaunsetbitxlate:
	shl cx,1
	inc si
	or ch,ch
	jz egabitxlate
	mov al,[cs:inverse]
	cmp al,1
	jnz eganoninverse
	not ah		; inverse printing
eganoninverse:
	mov [cs:bx],ah	; now we have a gray value
	inc bx
	cmp bx, palette+16
	jb egaxlate
	jmp economy

egapalbits:	; for each palette bit, know how "light" it is
	; x x r g b R G B
	; assume weights for 0..255 values: R=77 G=150 B=28
	db 19, 100, 52,  9, 50, 25, 0, 0

; ------------

mcgapalette:
	; could use int 10.1017.bx=start.cx=count.es:dx=buffer
	; to read a the DAC registers in chunks
	mov di, palette
mcgapalloop:
	mov bx, di	; index
	sub bx, palette
	mov ax,0x1015	; read DAC entry BL
	int 0x10	; returns: R=DH G=CH B=CL in 0..63 range
	mov ax,78	; RED intensity (77/256) (*** +1 ***)
	mul dh		; RED
	mov bx,ax	; start of a gray value sum
	mov ax,153	; GREEN intensity (151/256) (*** +2 ***)
	mul ch		; GREEN
	add bx,ax
	mov ax,28	; BLUE intensity (28/256)
	mul cl		; BLUE
	add bx,ax
	add bx,0x80	; round (maximum is now 16384+61)
	cmp bx,16384
	jb mcgainrange
	mov bx,16383	; clip to range
mcgainrange:
	shl bx,1	; scale up
	shl bx,1	; scale up
	mov al,[cs:inverse]
	cmp al,1
	jnz mcganoninverse
	not bh		; inverse printing
mcganoninverse:
	mov [cs:di],bh	; store 0..255 value
	inc di
	cmp di, palette+256
	jb mcgapalloop
	jmp economy

economy:		; make everything 50% brighter if economy mode
	mov al,[cs:economymode]
	cmp al,1
	jnz paldone
	mov di, palette
economyloop:
	mov al,[cs:di]
	mov ah,255
	sub ah,al
	shr ah,1	; half the distance to 255
	mov al,255
	sub al,ah
	mov [cs:di],al	; 255 - ((255 - N) / 2)
	inc di
	cmp di, palette+256
	jb economyloop
paldone:
	ret

