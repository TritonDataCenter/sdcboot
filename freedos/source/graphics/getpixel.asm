; GRAPHICS tool for FreeDOS - GPL by Eric Auer eric@coli.uni-sb.de 2003
; Please go to www.gnu.org if you have no copy of the GPL license yet.

	; pixel readers: based on src/env/video/vgaemu.c of DOSEMU
	; for the CGA and EGA case. DOSEMU ignores the page number!

cgamgetpixel:	; in: X=CX Y=DX out: AX=gray (0..255)
	push bx
	mov ax,dx
	and ax,0xfffe	; INTERLACE!
;	mul byte [ds:xram]/2	; always 80 = (16*4) + 16
;	shl ax,1
	shl ax,1
	shl ax,1
	shl ax,1		; now we have * 16
	mov bx,ax
	shl ax,1
	shl ax,1		; now we have * 64
	add bx,ax		; sum is * 80
	mov ax,cx		; 8 pixels / byte
	shr ax,1
	shr ax,1
	shr ax,1
	add bx,ax		; we know the byte ...
	test dl,1		; find out CGA page ...
	jz nocgainter1
	add bx,0x2000		; INTERLACE!
nocgainter1:
	xor ax,ax
	mov ah,127		; "black" -> 50% only
	cmp byte [cs:economymode],1
	jnz ncgamEconomy
	mov ah,0		; real black
ncgamEconomy:
	push cx
	and cl,7		; bit selector
	mov al,[es:bx]		; get that byte!
	shl al,cl		; MSB is left, LSB right
	or al,al
	mov al,ah		; "black of the day"
	jns cgamblack
	mov al,255		; white
cgamblack:
	pop cx
	pop bx
	ret

cgagetpixel:	; in: X=CX Y=DX out: AX=gray (0..255)
	push bx
	mov ax,dx
	and ax,0xfffe	; INTERLACE!
;	mul byte [ds:xram]/2	; always 80 = (16*4) + 16
;	shl ax,1
	shl ax,1
	shl ax,1
	shl ax,1		; now we have * 16
	mov bx,ax
	shl ax,1
	shl ax,1		; now we have * 64
	add bx,ax		; sum is * 80
	mov ax,cx		; 4 pixels / byte
	shr ax,1
	shr ax,1
	add bx,ax		; we know the byte ...
	test dl,1		; find out CGA page ...
	jz nocgainter2
	add bx,0x2000		; INTERLACE!
nocgainter2:
	push cx
	mov al,3		; MSB are left
	mov ah,cl
	and ah,3		; 2 LSB of X select bits
	sub al,ah
	add al,al		; 2 bits per pixel
	mov cl,al		; shift distance
	mov al,[es:bx]		; get that byte!
	shr al,cl		; MSB is left, LSB right
	and ax,3		; mask to use 2 bits
	pop cx
	; no pop bx, done later
	jmp genericxlategray

egamgetpixel:	; in: X=CX Y=DX out: AX=gray (0..255)
	; would multiply Y by 80, find cgam style bit mask,
	; use I/O and memory to read out pixel.
	; just using BIOS for now (slower)!
	jmp genericgetpixel

egagetpixel:	; in: X=CX Y=DX out: AX=gray (0..255)
	; would multiply Y by 40 or 80, find cgam style bit mask,
	; use I/O and memory to read out pixel.
	; just using BIOS for now (slower)!
	jmp genericgetpixel

mcgagetpixel:	; in: X=CX Y=DX out: AX=gray (0..255)
	push bx
	mov bl,0
	mov bh,dl	; only DL of DX is used
	mov ax,bx
	shr ax,1
	shr ax,1
	add bx,ax	; (Y*256)+(Y*256/4) = Y * 320 ...
	add bx,cx	; add X
	mov ah,0
	mov al,[es:bx]
	; no pop bx, done later
	jmp genericxlategray

genericgetpixel:	; in: X=CX Y=DX out: AX=gray (0..255)
	push bx
	mov ah,0x0d	; get pixel at CX,DX, page BH, to AL
	mov bh,[ds:gfxpage]
	int 0x10	; graphics BIOS
genericxlategray:
	mov bh,0
	mov bl,al	; form index
	mov ah,0
	mov al,[cs:palette+bx]	; read palette
	pop bx
	ret
