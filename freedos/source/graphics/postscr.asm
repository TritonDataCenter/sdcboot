; GRAPHICS tool for FreeDOS - GPL by Eric Auer eric@coli.uni-sb.de 2003
; Please go to www.gnu.org if you have no copy of the GPL license yet.

	; PostScript does not need to know any of this, just give
	; it the sizes of the output area and input bitmap array.

; use prtchar to print char AL. May or busyflag with 4 (abort) or
; 8 (error). User may and busyflag with not-1 (stop) to request
; clean abort of printing process (impossible with PS!?)...

; We could use ImageMagick style 24 bit color output, but for now,
; only 8 bit grayscale output is supported.

; example of a function definition:
; "/inch { 72 mul } def" allows you to write "3 inch" for "216".
; Hint: check http://www.mdwconsulting.com/postscript/postscript-operators/
; For examples: http://atrey.karlin.mff.cuni.cz/~milanek/PostScript/

; first, convert resolution to decimal. Only works for 100..999:
	mov bl,10
	mov ax,[ds:xres]
	div bl
	add ah,'0'	; ones
	mov [ds:xresstr+2],ah
	mov ah,0
	div bl
	add ax,'00'	; ah: tens al: hundreths
	mov [ds:xresstr],ax
	mov bl,10
	mov ax,[ds:yres]
	div bl
	add ah,'0'	; ones
	mov [ds:yresstr+2],ah
	mov ah,0
	div bl
	add ax,'00'	; ah: tens al: hundreths
	mov [ds:yresstr],ax

	mov al,[cs:compatmode]
	test al,al
	jz psstandardheader

	mov si,pscompatheader	; HP Laserjet: Enter PostScript mode
	call prtstr

	test byte [cs:busyflag],12	; any errors?
	jnz psSkipALL

psstandardheader:
	mov si,psheader
	call prtstr	; print the header and geometry
	test byte [cs:busyflag],12	; any errors?
	jz printedPSheader
psSkipALL:
	jmp leavePSprinter	; header not accepted, skip everything
printedPSheader:

; ------------

	xor cx,cx	; X
	xor dx,dx	; Y

nextPSpixel:
	test cx,3
	jnz noPSspace
	mov al,' '
	call prtchar	; space before every 4th dumped byte (readability)
noPSspace:

	mov ax,255	; white
	test byte [cs:busyflag],1	; user wants to abort printing?
	jz abortingPSprint
;	cmp word [cs:getpixel],0
;	jz sbortingPSprint
	call [cs:getpixel]	; this call is dynamically selected!
	; returns AX in 0..255 range, 255 being white.
; *** IF /R FLAG GIVEN, SET  AL  TO  255 - AL  (INVERSE PRINTING) ***

abortingPSprint:	; we can only skip getpixel, as the "image"
			; command already wants ALL pixel data from us.
			; (would end as soon as readhexstring runs empty).

	push ax
	shr ax,1	; high nibble first
	shr ax,1
	shr ax,1
	shr ax,1
	and ax,0x000f
	mov bx,hexdigits
	add bx,ax	; select digit
	mov ax,[cs:bx]
	call prtchar	; high nibble as hex
	pop ax
	mov bx,hexdigits
	and ax,0x000f
	add bx,ax
	mov ax,[cs:bx]
	call prtchar	; low nibble as hex

	mov ax,cx
	and ax,31
	cmp ax,31
	jnz noPSlinebreak
	call prtcrlf	; add line break after every 32 bytes to
			; make PS date more human readable
noPSlinebreak:
	inc cx	; next column
	cmp cx,[cs:xres]
	jb nextPSpixel	

	xor cx,cx
	call prtcrlf
	inc dx
	cmp dx,[cs:yres]
	jb nextPSpixel

; ------------

	mov si,psfooter
	call prtstr	; print the footer

	mov al,[cs:compatmode]
	test al,al
	jz leavePSprinter

	mov si,pscompatfooter	; HP Laserjet: Leave PostScript mode
	call prtstr

leavePSprinter:
	jmp i5eof	; we are done!

; ------------

prtcrlf:
	mov al,13
	call prtchar
	mov al,10
	call prtchar
	ret

prtstr:
	mov al,[cs:si]
	inc si
	or al,al
	jz short prtstrdone
	call prtchar		; print one char
	jmp short prtstr	; loop
prtstrdone:
	ret

; ------------

hexdigits:
	db "0123456789ABCDEF"

psheader:
	db "%!PS-Adobe-2.0",13,10	; for EPS, add " EPSF-2.0"
	db "%%Title: FreeDOS graphics screen shot",13,10
	db "%%Creator: FreeDOS graphics",13,10
	; db "%%Pages: 1",13,10		; must be 1 for EPS
	; if you use %%Pages:, each page must also have proper DSC...
	; for EPS, add %%BoundingBox: x1 y1 x2 y2
	db "%%EndComments",13,10,13,10	; no CreationDate

	db "/bufstr 500 string def",13,10,13,10	; buffer for reading
	; (in 8 bit / pixel case, one line size would be as X resolution)
	; db "%%EndProlog",13,10
	; db "%%Page: 1 1",13,10,13,10

	db "/xres "
xresstr	db "000 def",13,10
	db "/yres "
yresstr	db "000 def",13,10,13,10

	db "gsave",13,10
	db " 24 54 translate",13,10	; big margins (18 18 is okay, too)
	db " 90 rotate",13,10		; counter clock wise: top of DOS
	; screen ends up on left of page (else use 24 54 535 add translate)
	db "713 535 scale",13,10	; target size, 1:1.33 aspect ratio
	; you can translate to 45 750 and scale by 528 -528 if
	; you want "to mirror the image INTO the paper".

	db "xres yres 8",13,10		; 1. width, height, bit per pixel
	db "[xres 0 0 0 yres sub 0 0]",13,10	; 2. matrix
	; By EA: the "0 yres sub" together just means "-yres".
	; without extra matrix, lower left corner is 0,0.
	; image data then is lower left ... right, next line, etc.
	; matrix arg look like: 1/scaleX yForX xForY
	; 1/scaleY subFromX subFromY
	db "{currentfile bufstr readhexstring pop} bind",13,10	; 3. data
	; read from file into buffer, pop status to void, return string
	; (readhexstring takes file and bufstr and returns status and string)
	; bind causes evaluation of the {...} part. Data source is a
	; procedure here (PS level 1 allows no other) which means that while
	; it returns a nonempty string, the image will continue to consume
	; string data.
	db "image",13,10		; 4. take 1..3, show raster image.
	; alternative syntax:ÿ"dict image -"ÿ(PS level 2) allows color.
	; the alternative syntax takes a dictionary as input, with key/
	; value pairs for the various input variables of image.
	; PS level 2 also has colorimage, which has 1, 3 or 4 data sources.
	; For inlined data in PS level 2, use as 3.: {< hexdump >} ...
	; Now, hexdumped data follows: whitespace is ignored.
	; To use an image several times: /thename... { ... image } def ...
	db 0
	
psfooter:
	db 13,10
	db "grestore",13,10,13,10
	db "showpage",13,10,13,10,12,0

pscompatheader:
	db 27,"%-12345X@PJL JOB",13,10
	db "@PJL ENTER LANGUAGE = POSTSCRIPT",13,10,0

pscompatfooter:
	db "%%EOF",4,27,"%-12345X@PJL EOJ",13,10
	db 27,"%-12345X",13,10,0

