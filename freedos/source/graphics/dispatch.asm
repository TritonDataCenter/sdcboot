; FreeDOS GRAPHICS tool: GPL by Eric Auer eric@coli.uni-sb.de 2003/2008
; Please go to www.gnu.org if you have no copy of the GPL license yet.
; (2008: RAX etc are 64bit CPU reserved words now, renamed to svAX etc)

	; initializers for getpixel and palette and resolution

cgamono:
	mov word [ds:getpixel], cgamgetpixel
	mov cx,40	; 40 bytes / line
	mov bx,201	; 640x200
	jmp short genericmode

cgafour:
	mov word [ds:getpixel], cgagetpixel
	call cgapalette
	mov cx,80	; 80 bytes / line
	mov bx,200	; 320x200
	jmp short genericmode

egamono:
	mov word [ds:getpixel], egamgetpixel
	call egapalette	; (if this is wrong, EGAM getpixel is, too)
	mov cl,[ds:palette+15]	; "white of the day"
	mov [ds:palette+1],cl	; is this correct / needed???
	mov cx,80	; 80 bytes / line
	mov bx,351	; 640x350
	cmp al,0x0f
	jz short genericmode	; 640x350 ?
	mov bx,481	; 640x480 is the other one
	jmp short genericmode

egamode:
	mov word [ds:getpixel], egagetpixel
	call egapalette
	mov cx,40	; 40 bytes / line
	mov bx,200	; 320x320
	mov al,[ds:gfxmode]
	cmp al,0x0d	; 320x200 ?
	jz short genericmode
	add cx,cx	; 80 bytes / line
	mov bx,201	; 640x200
	cmp al,0x0e	; 640x200 ?
	jz short genericmode
	mov bx,351	; 640x350
	cmp al,0x10	; 640x350 ? (might be only 4 colors if 64k EGA)
	jz short genericmode
	mov bx,481	; 640x480
	jmp short genericmode

mcgamode:
	mov word [ds:getpixel], mcgagetpixel
	call mcgapalette
	mov bx,200	; Y, 320
	mov cx,320	; 320 bytes / line
	jmp short genericmode

genericmode:
	mov ax,320	; X=320
	test bx,1	; flag set?
	jz cols320
	add ax,ax	; X=640
	and bx,0xfffe	; remove flag
cols320:
	mov [ds:xres],ax	; X screen resolution
	mov [ds:yres],bx	; Y screen resolution
	mov [ds:xram],cx	; X bytes per line
	mov es,[ds:gfxseg]	; load video buffer segment

	sti		; do not freeze things any longer
	; (although only palette, not screen buffer got copied yet)

	; We assume 33 x 25 cm screen size and are careful
	; that even big margins work on both A4 and Letter paper.

%ifdef EPSON
	; The Epson ESC/P driver uses pseudo-random or pseudo-ordered
	; dither and arbitrary scaling to get the output size right.
%include "epson.asm"
%endif

%ifdef HPPCL
	; The HP PCL driver uses pseudo-random or pseudo-ordered
	; dither and arbitrary scaling to get the output size right.
%include "hppcl.asm"
%endif

%ifdef POSTSCRIPT
	; PostScript does not need to know any of this, just give
	; it the sizes of the output area and input bitmap array.
%include "postscr.asm"
%endif

; ------------

i5:	cld		; new print screen handler
	test byte [cs:busyflag],3	; already busy?
	jz notbusy
	and byte [cs:busyflag],0xfe	; abort screen dump if PrintScreen
	push ax
	mov al,7
	call tty			; beep
	pop ax
	jmp short i5done		; key pressed while dumping!
notbusy:
	mov byte [cs:busyflag],3	; assume being busy AND no errors
	call regsave
	mov ah,0x0f	; get current video mode
	int 0x10
	mov [ds:gfxpage],bh	; current video page
	cmp bh,0
	jnz oldi5	; only active page 0 supported
	and al,0x7f	; ignore "blanking" property
	mov [ds:gfxmode],al	
	mov word [ds:gfxseg],0xb800	; assume CGA first
	cmp al,4
	jnz ncga4
cga4a:	jmp cgafour	; 4: 320x200 CGA, color
ncga4:	cmp al,5
	jz cga4a	; 5: 320x200 CGA, 4 gray
	cmp al,6
	jnz ncgam
	jmp cgamono	; 6: 640x200 CGA
ncgam:	; mov word [ds:gfxseg],0xb000]
	; Mode 8 (and 9): several BIOS extensions for Hercules, 720x348
	; ...
	mov word [ds:gfxseg],0xa000
	cmp al,0x0d
	jnz negaa
ega16:	jmp egamode	; 0dh: 320x200 EGA
negaa:	cmp al,0x0e
	jz ega16	; 0eh: 640x200 EGA
	cmp al,0x0f
	jnz negam
egam:	jmp egamono	; 0fh: 640x350 EGA mono
negam:	cmp al,0x10
	jz ega16	; 10h: 640x350 EGA, 4 or (256k gfx. RAM) 16 colors
	cmp al,0x11
	jz egam		; 11h: 640x480 EGA / VGA mono
	cmp al,0x12
	jz ega16	; 12h: 640x480 EGA / VGA 16 (or more) colors
	cmp al,0x13
	jnz nmcga
	jmp mcgamode	; 13h: 320x200 MCGA 256 colors
nmcga:	; add more modes here
	; ...
	mov byte [ds:busyflag],0	; decide not to be / get involved
	jmp short oldi5	; text mode or unsupported mode: let others do it
i5eof:	mov byte [ds:busyflag],0	; no longer busy
i5done:	call regrestore
	iret
oldi5:	call regrestore
	jmp far [cs:oldi5vec]	; jump to original one

; ------------

regsave:
	mov [cs:svDS],ds
	push cs
	pop ds
	mov [ds:svES],es
	mov [ds:svAX],ax
	mov [ds:svBX],bx
	mov [ds:svCX],cx
	mov [ds:svDX],dx
	mov [ds:svSI],si
	mov [ds:svDI],di
	mov [ds:svBP],bp
	ret

regrestore:
	push cs
	pop ds
	mov es,[ds:svES]
	mov ax,[ds:svAX]
	mov bx,[ds:svBX]
	mov cx,[ds:svCX]
	mov dx,[ds:svDX]
	mov si,[ds:svSI]
	mov di,[ds:svDI]
	mov bp,[ds:svBP]
	mov ds,[ds:svDS]
	ret

svAX	dw 0
svBX	dw 0
svCX	dw 0
svDX	dw 0
svBP	dw 0
svSI	dw 0
svDI	dw 0
svES	dw 0
svDS	dw 0

