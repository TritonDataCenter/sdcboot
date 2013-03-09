; FreeDOS GRAPHICS tool: GPL by Eric Auer eric@coli.uni-sb.de 2003/2008
; Please go to www.gnu.org if you have no copy of the GPL license yet.

; use prtchar to print char AL. May or busyflag with 4 (abort) or
; 8 (error). User may and busyflag with not-1 (stop) to request
; clean abort of printing process (impossible with PS!?)...

; EPSON driver now with new command line option /R for "RANDOM"
; (otherwise, use "half-ordered" dither)...

; Google search for "matrix printer 180 graphics 24-pin" or similar
; to learn about the ESC codes for other printers. If you want to do
; "9 pin" printing, use 8 pin graphics modes (normally 72 dpi).

; For 24 pin printing, 180 dpi is common (horizontal resolution may
; be configurable from 90..360 dpi) If you need 360 dpi vertical
; resolution, you have to heavily modify this to do 2-pass printing
; By the way, 24 pin printers in 8 pin mode normally have 60 dpi.

; %define OLDRES 1	; select pre 7/2008 canvas sizes
			; new are "rounder" multiples of screen size

	mov si,epsonINIT		; works for all 24 pin modes
	; 180x180 dpi ESC sequence is the default in code elsewhere
	mov al,[cs:compatmode]
	test al,al
	jz short epsonInitDone	; use default sizes / resolutions, 180 dpi
epsonOldModes:			; 8of9 pin 120x72 dpi / 8of24 pin 120x60 dpi
	mov si,epsonINITold		; works for all 8 pin modes
	mov word [cs:epsonPins],   8	; not 9, of course
	mov word [cs:epsonYdpi],  72	; for 9 pin printers (unused)
%ifdef OLDRES
	mov word [cs:epsonYsz],  592	; in 72 dpi: 8.2 inch (9.9 on 24pin!)
%else	; 9 pin mode: size must be multiple of 8
	mov word [cs:epsonYsz],  704	; 9 7/9 inch 72 dpi (9 pin only!)
%endif
	cmp al,3	; /9 /C is 60x72 dpi mode
	jz epsonAncientMode
	test al,2	; /9 flag set? then use 120x72 instead of 120x60
	jnz epsonOldMode9
	mov word [cs:epsonYdpi],  60	; for 24 pin printers (unused)
%ifdef OLDRES
	mov word [cs:epsonYsz],  496	; in 60 dpi: 8.3 inch (6.9 on 9pin!)
%else	; 9 pin mode: size must be multiple of 8
	mov word [cs:epsonYsz],  576	; 9.6 inch 60 dpi
%endif
epsonOldMode9:				; same X setup for 120x72 and 120x60
	mov word [cs:epsonXdpi], 120	; (unused)
%ifdef OLDRES
	mov word [cs:epsonXsz],  744	; X is smaller, as Y is smaller:
					; only 6.2 inch. 31*24 / 186*4
%else
	mov word [cs:epsonXsz],  880	; 120 dpi 7 1/3 inch 4*epsonGC
%endif
	; 120x60/120x72 dpi ESC sequence is selected in code elsewhere
	jmp short epsonInitDone

epsonAncientMode:	; 60x72 dpi (distorted to 60x60 on 24pin printers)
	mov word [cs:epsonXdpi],  60	; (unused)
%ifdef OLDRES
	mov word [cs:epsonXsz],  372	; only 6.2 inch. 31*12 / 186*2
%else
	mov word [cs:epsonXsz],  440	; 60 dpi 7 1/3 inch 2*epsonGC
%endif
	; 60x72 dpi ESC sequence is selected in code elsewhere

epsonInitDone:
	call prtstr			; set up printer
	test byte [cs:busyflag],12	; any errors?
	jz initEPSONworked
	jmp abortEPSONprinter		; skip all rest, init failed

initEPSONworked:	; (1)
	mov al,[cs:cgaback]		; undocumented extra flag (2008 ext)
	test al,2
	jz smallstripes
widestripes:				; set chunk width to page width
	mov ax,[cs:epsonXsz]
	mov [cs:epsonGC],ax
	mov [cs:epsonGCold],ax
	mov [cs:epsonGColder],ax
smallstripes:
	xor cx,cx			; X (printer)
	xor dx,dx			; Y (printer)
	jmp short nextEPSONblock

nextEPSONline:		; (2)
	mov si,epsonCRLF		; advance paper
	call prtstr
	xor cx,cx			; X (printer)
	add dx,[cs:epsonPins]		; Y (printer)

nextEPSONblock:		; (3)
	test byte [cs:busyflag],1	; user wants to abort?
	jnz noEPSONabort
doEPSONabort:
	mov si,epsonAbort
	call prtstr			; confirm abort
	jmp leaveEPSONprinter
noEPSONabort:

	mov al,[cs:compatmode]
	or al,al
	jz epsonFullGFX
	; 2008 extension: compatmode can be 1 120x60 / 2 120x72 / 3 60x72
epsonOldGFX:
	mov si,epsonGFXold		; start graphics bitmap block
	mov bx,[cs:epsonGCold]		; graphics columns / block
	cmp al,3
	jnz epsonGFXcommon		; 120x60 or 120x72 (same escape)
	mov si,epsonGFXolder		; 60x72 ancient resolution /9 /C
	mov bx,[cs:epsonGColder]	; graphics columns / block
	jmp short epsonGFXcommon
epsonFullGFX:	; 180x180 dpi mode
	mov si,epsonGFX			; start graphics bitmap block
	mov bx,[cs:epsonGC]		; graphics columns / block
	jmp short epsonGFXcommon
epsonGFXcommon:
	call prtstr
	or bh,bh			; wider than 256 pixel columns?
	jnz widestripes2
smallstripes2:
	mov al,0			; second byte of columns!
	call prtchar			; (\0 not printed in prtstr)
widestripes2:

epsonBothGFX:
	mov si,[cs:epsonPins]		; graphics rows / block
					; BX SI only valid in (3)..(4)
	mov di,0x8000			; no pixels set, MSB on top

nextEPSONpixel:		; (4)		; CX/DX: printer X/Y
	push cx				; BX/SI: remaining cols/pins
	push dx				; Innermost loop: pins (DX, SI)

	; *** figure out screen CX DX based on printer CX DX ***

	xchg cx,dx	; landscape mode
	mov ax,[cs:epsonXsz]
	sub ax,dx
	mov dx,ax	; landscape mode

	; now coordinate origin is at upper right of paper,
	; where screen coordinate origin (upper left) will be.

	mov ax,dx		; inv. printer X -> screen Y

	cmp ax,[cs:epsonLastRawX]
	jz epsonKnownRawX	; printer X changes every 8th/24th pixel
	mov [cs:epsonLastRawX],ax

	mul word [cs:yres]	; scale up
	; (32 bit intermediate value DX AX)
	div word [cs:epsonXsz]	; scale down

	mov [cs:epsonLastCookedX],ax
epsonKnownRawX:
	mov ax,[cs:epsonLastCookedX]

	mov dx,ax		; resulting screen Y

	push dx
	mov ax,cx		; screen X, printer Y based
	mul word [cs:xres]	; scale up
	; (32 bit intermediate value DX AX)
	div word [cs:epsonYsz]	; scale down
	mov cx,ax		; resulting screen X
	pop dx

	; *** optional if Ysz multiple Pins, Xsz multiple of GC... ->
	mov ax,255		; white border
	cmp cx,[cs:xres]
	jae gotEPSONpixel
	cmp dx,[cs:yres]
	jae gotEPSONpixel
	; <- optional if even multiples ***

	; use xres, yres and call [getpixel] for screen reading.

	cmp cx,[cs:epsonLastX]		; gain speed, remember pixels
	jnz freshEPSONpixel
	cmp dx,[cs:epsonLastY]
	jnz freshEPSONpixel
	mov ax,[cs:epsonLastPix]	; fetch already known pixel
	jmp gotEPSONpixel
freshEPSONpixel:
	mov [cs:epsonLastX],cx
	mov [cs:epsonLastY],dx
	call [cs:getpixel]	; this call is dynamically selected!
	mov [cs:epsonLastPix],ax
gotEPSONpixel:
	; returns AX in 0..255 range, 255 being white

	pop dx
	pop cx

	inc dx				; count up rows
	dec si				; count down pins

	cmp byte [cs:random],1	; random or ordered dither?
	jz epsonRdither
epsonOdither:
	call ditherBWordered	; set CY with probability (AL/255)
	jc epsonWHITE
	jmp short epsonBLACK
epsonRdither:
	call ditherBWrandom
	jc epsonWHITE
	; jmp short epsonBLACK

epsonBLACK:
	mov ax,di
	or al,ah		; OR in that black pixel
	mov di,ax
epsonWHITE:

	test si,7			; done with byte?
	jnz stillInByte
	mov ax,di
	call prtchar			; SEND that byte
	mov di,0x8000			; flush bit bucket
	jmp short freshByte

stillInByte:
	mov ax,di
	shr ah,1			; go to next pixel
	mov di,ax

freshByte:
	test si,255			; pins left?
	jz nextEPSONcolumn
	jmp nextEPSONpixel	; (/4)	; still in pixel column

nextEPSONcolumn:
	sub dx,[cs:epsonPins]		; go back to upper pin
	mov si,[cs:epsonPins]		; COUNT pins AGAIN...

	;
	push bx
	mov bx,cx			; column
	and bx,7
	add [cs:ditherTemp],bl		; yet another try for weaving
; ---	add bx,bx
; ---	mov bl,[cs:weaveTab+bx]		; weaving table
; ---	mov [cs:ditherWeave],bl		; reduce artifacts by weaving
; ---	add [cs:ditherTemp],bl		; reduce artifacts by weaving
	; (most visible patterns are every 2/4/8 values of ditherTemp)
	pop bx
	;

	inc cx				; next pixel column
	dec bx				; count down columns
	jz doneEPSONblock
	jmp nextEPSONpixel	; (/4)	; still in bitmap block

doneEPSONblock:
	cmp cx,[cs:epsonXsz]		; done with row?
	jae doneEPSONline
	jmp nextEPSONblock	; (/3)	; otherwise, send next bitmap

doneEPSONline:
; ***	mov al,7
; ***	call tty			; beep each row
	;
	mov byte [cs:ditherTemp],0	; reset dithering pattern
	;
	cmp dx,[cs:epsonYsz]		; done with printing?
	jae leaveEPSONprinter
	jmp nextEPSONline	; (/2)	; otherwise, do next line
	
leaveEPSONprinter:		; (/1)
	mov si,epsonDONE		; send closing sequence
	call prtstr

abortEPSONprinter:
	jmp i5eof			; *** DONE ***

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

	; Epson escape sequences that we may want to use:
	; > ESC * mode lowcolumns highcolumns data -> graphics mode
	;              (mode 39 is for example 24pin 180x180dpi)
	;   ESC J n -> advance n/180 inch
	;   ESC A n -> line spacing n/60 inch
	; > ESC 3 n -> line spacing n/180 inch
	;   ESC + n -> line spacing n/360 inch
	; > ESC 0   -> 8 lines per inch
	;   ESC 2   -> 6 lines per inch

epsonXdpi	dw 180	; printer X resolution (unused)
epsonYdpi	dw 180	; printer Y resolution (unused)
%ifdef OLDRES	; "big canvas" (for 8.25*11.7 / 8.5*11 inch paper)
epsonXsz	dw 1330	; width of printout in pixels, e.g. 7.4 inch
			; (epsonXsz should be a multiple of epsGC) (19*70)
epsonYsz	dw 1776	; length of printout in pixels, e.g. 9.8 inch
%else		; "rounder" multiples: 2.75*480 (6.6*200) by 2.75*640
epsonXsz	dw 1320	; width of printout in pixels, 7 1/3 inch
			; (epsonXsz should be a multiple of epsGC) (6*220)
epsonYsz	dw 1760	; length of printout in pixels (padded with/in
			; white to multiple of 24 = 1776 = 9 7/9 inch)
%endif
epsonPins	dw 24	; use 24 or 8 pins? (word!)
			; (epsonYsz should be a multiple of epsPins)

epsonAbort	db 13,10,"(aborted)",13,10,0
		; 3/2008 ESC @ resets all settings - avoids bad surprises
		; and is needed for some printers at end of hardcopy (Oleg)
epsonINITold	db 27,"@",27,"A",8	; init sequence (8 pixels per line)
		db 13,10,0	; (60 or 72 Y dpi depending on printer)
epsonINIT	db 27,"@",27,"3",24	; init sequence (24 pixels per line)
		db 13,10,0	; (180 Y dpi)

epsonDONE	db 27,"0"	; closing sequence (8 lines per inch)
		db 12		; do form feed as well
		db 27,"@",0
epsonCRLF	db 13,10,0	; how to adv. paper by epsonPins pixels

epsonGFXold	db 27,"L"	; init sequence for graphics mode, 120x60 (72)
%ifdef OLDRES
epsonGCold	dw 186		; pixel columns of each epsonGFX* (was: 31)
%else
epsonGCold	dw 220		; pixel columns of each 8/9 pin epsonGFX*
%endif
		db 0		; (epsonXsz should be a multiple of epsonGC*)
epsonGFX	db 27,"*",39	; init sequence for graphics mode 180x180
%ifdef OLDRES
epsonGC		dw 70		; pixel columns of each 24 pin epsonGFX*
%else
epsonGC		dw 220		; pixel columns of each 24 pin epsonGFX*
%endif
		db 0		; (epsonXsz should be a multiple of epsonGC*)
epsonGFXolder	db 27,"K"	; init sequence for graphics mode 60x60 (72)
%ifdef OLDRES
epsonGColder	dw 186		; pixel columns of each epsonGFX* (was: 31)
%else
epsonGColder	dw 220		; pixel columns of each 8/24 pin epsonGFX*
%endif
		db 0		; (epsonXsz should be a multiple of epsonGC*)
		; more: ESC Y... fast 120x60 (72), ESC Z... 240x...

		; data: top, middle, low, top, middle, low, ... bytes
		; (3 bytes per column, top bit is 0x80...!)

		; for non-Epson printers, you may have to send for
		; example the data size in bytes rather than using
		; the epsonGC value directly. In that case, you still
		; need to have the epsonGC dw around, just move it
		; out of the scope of the epsonGFX string then.

epsonLastRawX		dw -1	; last seen printer column ->
epsonLastCookedX	dw 0	; -> last seen screen row

epsonLastX	dw -1		; last seen pixel
epsonLastY	dw -1		; last seen pixel
epsonLastPix	dw 0		; last seen pixel


; ------------

	; If you would do pure ordered dither with integer print
	; size for each screen pixel, I recommend those values:

	; Print box is printer pixels per screen pixel if 180 dpi.
	; Aspect ratio on screen and x/y factor and print box:
	; 320x200: 1:1.25 1.60 4x5
	; 640x200: 1:2.50 3.20 2x5
	; 640x350: 1:1.39 1.83 2x3
	; 640x480: 1:1.00 1.33 2x2
	; Hercules would be 720x348: 1:1.56 2.07 2x3 (or 720x360 ...) 
	; Print box of 320x200 is reduced to fit aspect ratio better.
	; 2x3 print box is 1:1.50 but you cannot easily improve that.

	; Note that the CURRENT implementation allows "arbitrary"
	; zoom factors between screen size and used paper area.
