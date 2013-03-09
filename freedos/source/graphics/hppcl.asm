; FreeDOS GRAPHICS tool: GPL by Eric Auer eric@coli.uni-sb.de 2003/2008
; Please go to www.gnu.org if you have no copy of the GPL license yet.

; use prtchar to print char AL. May or busyflag with 4 (abort) or
; 8 (error). User may and busyflag with not-1 (stop) to request
; clean abort of printing process (impossible with PS!?)...

; HP PCL driver uses command line option /R for "RANDOM" dither
; (otherwise, use "half-ordered" dither)...

; This is the HP PCL driver, based on the EPSON driver. HP PCL sends
; data as msb-at-left, 1 row at a time, while EPSON sends data as
; msb-on-top, 8 or 24 rows at a time.

%define TESTING 1	; define to force bitmap data to be
			; free of ESC, LF, FF chars, to be safe...
%define HPPCL3PLUS 1	; define to select HP printer mode
			;  "unshingled depleted econofast" if 300 dpi
; %define OLDRES 1	; use old (pre 7/2008) canvas sizes
			; new sizes are "rounder" screen size multiples

	mov si,hppclINIT	; 600x600 dpi
	mov al,[cs:compatmode]
	test al,al
	jz hppclFullMode
hppclOldMode:		; only 300x300 instead of 600x600 dpi
	cmp word [cs:hppclXdpi],300	; already shifted?
	jbe hppclOldShiftAlreadyDone
	shr word [cs:hppclXdpi], 1	; (not used)
	shr word [cs:hppclYdpi], 1	; (not used)
	shr word [cs:hppclXsz], 1
	shr word [cs:hppclYsz], 1
hppclOldShiftAlreadyDone:
	mov si,hppclINITold	; 300x300 dpi
	; rest of changes is handled in-line

hppclFullMode:
	call prtstr			; set up printer
	test byte [cs:busyflag],12	; any errors?
	jz inithppclworked
	jmp aborthppclprinter		; skip all rest, init failed

inithppclworked:	; (1)
	xor dx,dx			; Y (printer)

	; ***

nexthppclline:		; (2)
	xor cx,cx			; X (printer)

	test dx,15
	jnz nohppclabort		; only check every 16th pixel row
	test byte [cs:busyflag],1	; user wants to abort?
	jnz nohppclabort
dohppclabort:
	mov si,hppclAbort
	call prtstr			; leave gfx mode, confirm abort
	mov si,hppclDONE2		; because gfx mode already left
	jmp leavehppclprinter2
nohppclabort:

	mov si,hppclGFX			; start graphics bitmap line, HIRES
	mov al,[cs:compatmode]
	or al,al
	jz hppclBothGFX
hppclOldGFX:
	mov si,hppclGFXold		; start graphics bitmap line, LORES
hppclBothGFX:
	call prtstr
	mov di,0x8000			; no pixels set, MSB on left

	; ***

nexthppclpixel:		; (3)		; CX/DX: printer X/Y
					; DI: bitmask, pixels
			; Innermost loop: CX (printer columns)

	push cx
	push dx

	; *** figure out screen CX DX based on printer CX DX ***

	xchg cx,dx	; landscape mode
	mov ax,[cs:hppclXsz]
	sub ax,dx
	mov dx,ax	; landscape mode

	; now coordinate origin is at upper right of paper,
	; where screen coordinate origin (upper left) will be.

	mov ax,dx		; calc screen Y from inv. printer X
	mul word [cs:yres]	; scale up
	; (32 bit intermediate value DX AX)
	div word [cs:hppclXsz]	; scale down
	mov dx,ax		; resulting screen Y

	mov ax,cx		; calculate screen X from printer Y
	cmp ax,[cs:hppclLastRawX]	; raw X does not change often
	jz hppclKnownRawX	; outer loop changes raw X / printer Y
	mov [cs:hppclLastRawX],ax

	push dx
	mul word [cs:xres]	; scale up
	; (32 bit intermediate value DX AX)
	div word [cs:hppclYsz]	; scale down
	mov [cs:hppclLastCookedX],ax
	pop dx

hppclKnownRawX:
	mov ax,[cs:hppclLastCookedX]	; current screen X
	mov cx,ax		; screen X

	; *** optional ->
	mov ax,255		; white border
	cmp cx,[cs:xres]
	jae gothppclpixel
	cmp dx,[cs:yres]
	jae gothppclpixel
	; <- optional ***

	; use xres, yres and call [getpixel] for screen reading.

	cmp cx,[cs:hppclLastX]		; gain speed, remember pixels
	jnz freshhppclpixel
	cmp dx,[cs:hppclLastY]
	jnz freshhppclpixel
	mov ax,[cs:hppclLastPix]	; fetch already known pixel
	jmp gothppclpixel
freshhppclpixel:
	mov [cs:hppclLastX],cx
	mov [cs:hppclLastY],dx
	call [cs:getpixel]	; this call is dynamically selected!
	mov [cs:hppclLastPix],ax
gothppclpixel:
	; returns AX in 0..255 range, 255 being white

	pop dx
	pop cx



	cmp byte [cs:random],1	; random or ordered dither?
	jz hppclRdither
hppclOdither:
	call ditherBWordered	; set CY with probability (AL/255)
	jc hppclWHITE
	jmp short hppclBLACK
hppclRdither:
	call ditherBWrandom
	jc hppclWHITE
	; jmp short hppclBLACK

hppclBLACK:
	mov ax,di
	or al,ah		; OR in that black pixel
	mov di,ax
hppclWHITE:

	mov ax,di			; AH: mask AL: bits
	test ah,1			; done with byte?
	jz stillInByte			; last bit not reached

%ifdef TESTING
	call extracharcare		; avoid escapes
%endif

	call prtchar			; SEND that byte
	mov di,0x8000			; flush bit bucket
	jmp short freshByte

stillInByte:
	shr ah,1			; go to next pixel
	mov di,ax			; store new mask / bits

freshByte:
	inc cx				; *** next pixel column
	cmp cx,[cs:hppclXsz]		; done with all?
	jae donehppclline
	jmp nexthppclpixel	; (/3)	; still in bitmap line

donehppclline:

	push bx
	;
%ifdef OLDWEAVE
	mov ax,dx			; row
	and ax,15
; ---	mov [cs:ditherTemp],al		; yet another try for weaving
	mov bx,ax			; "resets" dithering pattern
	add bx,bx
	mov bl,[cs:weaveTab+bx]		; weaving table
; ---	mov [cs:ditherWeave],bl		; reduce artifacts by weaving
	; (most visible patterns are every 2/4/8 values of ditherTemp)
%else
	mov ax,127
	call ditherBWrandom		; make seed change
	mov bx,[cs:ditherSeed]
	xor bl,bh
	xor bl,dl			; include row in seed
%endif
	mov [cs:ditherTemp],bl		; reduce artifacts by random offset
	;
	mov bx,[cs:ditherSeed]		; this provides some new
	xchg bl,bh			; "extra randomness" for
	xor bx,dx			; include row in seed
	mov [cs:ditherSeed],bx		; the "random" dither...
	pop bx


; *	mov al,7
; *	call tty			; beep each row

	inc dx				; *** next pixel row

	cmp dx,[cs:hppclYsz]		; done with printing?
	jae leavehppclprinter
	jmp nexthppclline	; (/2)	; otherwise, do next line
	
leavehppclprinter:		; (/1)
	mov si,hppclDONE		; send closing sequence
leavehppclprinter2:
	call prtstr

	mov al,7			; beep ...
	call tty			; ... to tell that we are done

aborthppclprinter:
	jmp i5eof			; *** DONE ***

; ------------

%ifdef TESTING
extracharcare:	; replace "escape like" AL values (for testing)
	cmp al,27	; ESC (00011011)
	jz ecc1
	cmp al,10	; LF  (00001010)
	jz ecc2
	cmp al,12	; FF  (00001100)
	jz ecc3
	ret
ecc1:	mov al,0x33	;     (00110011)
	ret
ecc2:	mov al,0x14	;     (00010100)
	ret
ecc3:	mov al,0x18	;     (00011000)
	ret
%endif

; ------------

; prtstr          - print 0 terminated string starting at CS:SI
; ditherBWordered - set CY with probability proportional to AL
;   (using "mirrored counter" as "randomness" source)
;   (to set seed, set ditherTemp, e.g. from weaveTab)
; ditherBWrandom  - set CY with probability proportional to AL
;   (using a linear congruential pseudo random number generator)
;   (to set seed, set ditherSeed, not really needed)

%include "dither.asm"	; random/ordered dithering, string output

; ------------

	; HP PCL escape sequences that we may want to use:
	; (enter your values as human-readable number at ___!)
; 1	; ESCE      reset printer (recommended before/after job)
	;           (also a job start/end marker, ejects IF non-empty)
; (2)	; ESC&l___E set top margin to ___ lines
; 3	; ESC*t___R set graphics resolution in dpi (maximum/integer)
; 4	; ESC*r_A   position graphics: 0 left margin, 1 *current pos.*

; 5	; ESC*r___U color depth: 1 (s/w), 3 (RGB), -4 (KCMY), -3 (CMY)
; 6	; ESC*b_M   select compression type _ (0 uncompressed)
	; (1=repetitions/data, 2=(size/data)-or-(repetitions/data)
	; where repetitions/size are distinguished by sign. Newer
	; printers have other compression modes 3..9 as well. For DOS
	; GRAPHICS, no compression is used to avoid the need to buffer
	; line before knowing how many bytes it will be compressed...

; *	; ESC*b___V send ___ bytes of graphics data after this
;  *	;           (for COLOR: for all planes EXCEPT the last one)
;   *	; ESC*b___W send ___ bytes of graphics data after this
	;           (causes appropriate vertical movement after it)
; 98	; ESC*rB    leave graphics mode
; 99	; FF        (hex 0c, ASCII 12): formfeed, eject page

	; *** the next 4 values are divided by 2 for 300 dpi mode: ***
hppclXdpi	dw 600	; printer X resolution (unused)
hppclYdpi	dw 600	; printer Y resolution (unused)
%ifdef OLDRES	; 600dpi canvas size should be multiple of 16*16
hppclXsz	dw 4448	; width of printout in pixels, e.g. 7.43 inch
hppclYsz	dw 5940	; length of printout in pixels, e.g. 9.9 inch
%else		; "rounder" factors 9*480, 21.6*200, 12 12/35 * 350:
hppclXsz	dw 4320	; width of printout in pixels, e.g. 7.2 inch
hppclYsz	dw 5760	; length of printout in pixels, e.g. 9.6 inch
%endif

hppclINITold	db 27,"E",13,10		; init sequence (300 dpi)
%ifdef HPPCL3PLUS
		db 27,"*o-1M"	; 300x300 dpi EconoFast mode
			; 0=300x300, 1=600x600 density print head mode
		db 27,"*o0M"	; shingling: 0/1/2=1/2/4 passes.
			; (printer lets ink dry longer that way)
		db 27,"*o3D"	; maximum depletion, no gamma correct.
%endif
		db 27,"*t300R",27,"*r1A"	; 300dpi, left
		db 27,"*r1U",27,"*b0M",0	; b/w, uncompressed

hppclINIT	db 27,"E",13,10		; init sequence (600 dpi)
%ifdef HPPCL3PLUS
		db 27,"*o1M"	; 600x600 dpi mode
		db 27,"*o0M"	; shingling: 0/1/2=1/2/4 passes.
			; (printer lets ink dry longer that way)
		db 27,"*o3D"	; maximum depletion, no gamma correct.
%endif
		db 27,"*t600R",27,"*r1A"	; 600dpi, left
		db 27,"*r1U",27,"*b0M",0	; b/w, uncompressed

	; HP PCL 3 compatible printers usually support shingling and
	; depletion in hardware. Not sure about EconoFast mode.
	; At least, raster graphics data stays constant in all cases.

hppclAbort	db 27,"*rC"
		db 13,10
		db "(aborted)",13,10,0

hppclDONE	db 27,"*rC"	; closing sequence
hppclDONE2	db 13,10,12	; do form feed as well
		db 27,"E"	; reset printer / end print job
		db 0

hppclGFXold	db 27,"*b"	; init sequence for graphics mode
%ifdef OLDRES
hppclResStrold	db "278"	; 2224/8
%else
hppclResStrold	db "270"	; 2160/8
%endif
		db "W",0

hppclGFX	db 27,"*b"	; init sequence for gfx. data block
%ifdef OLDRES
hppclResStr	db "556"	; 4448/8
%else
hppclResStr	db "540"	; 4320/8
%endif
		db "W",0

	; unused:
	; ESC&k___H set line spacing in 1/120 inch units
	; ESC&l___C set line spacing in 1/48 inch units
	; Newer printers allow medium type and quality setting:
	; ***	db 27,"&l4M"	; glossy/transparency
		; (0=normal 1=bond 2=premium 3=glossy paper 4=transp.)
	; ***	db 27,"*r1Q"	; quality: 1 draft 2 letter
	; Those settings are mapped to shingling and depletion:
	; Shingling 0/1/2 means 1/2/4 passes to give ink time to dry.
	; Depletion (0/1=off) is 2/3=25/50%, 4/5=25/50% with gamma
	; correction -> hardware removes dots to reduce ink bleeding
	; (older printers do not allow gamma corrected depletion).

hppclLastRawX		dw -1	; last seen printer row ->
hppclLastCookedX	dw 0	; -> last seen screen column

hppclLastX	dw -1		; last seen pixel ->
hppclLastY	dw -1		; last seen pixel ->
hppclLastPix	dw 0		; -> last seen pixel

