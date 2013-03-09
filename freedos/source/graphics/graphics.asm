; GRAPHICS tool for FreeDOS - GPL by Eric Auer eric@coli.uni-sb.de 2003/2008
; Please go to www.gnu.org if you have no copy of the GPL license yet.

; This is a program to print the graphics screen contents for a few
; standard modes (CGA, EGA, VGA, MCGA) on a few standard printer types
; (Epson ESC/P 24 or 9 pin / compatibles, 256gray PostScript, HP PCL).
; Hercules 720x348 mono mode should be supported in future versions.
; EGA, VGA, MCGA can do CGA modes. VGA can do CGA, EGA and MCGA modes.

; %define EPSON 1	; ... to enable "generic" 9/24 pin ESC/P driver
; %define POSTSCRIPT 1	; ... to enable grayscale PostScript driver
; %define HPPCL 1	; ... to enable HP PCL black and white driver

	org 0x100	; it is a COM program
start:	jmp install

; ------------

%include "palette.asm"	; palette setup

; ------------

%include "dispatch.asm"	; int handler, mode / getpixel setup
	; ************	; this is what uses the driver type defines!

; ------------

tty:
	push ax
	push bx
	mov bx,0007
	mov ah,0x0e
	int 0x10
	pop bx
	pop ax
	ret

prtchar:	; print char AL
	test byte [cs:busyflag],4
	jnz short prtskip
	push ax
	push bx
	push dx
	mov dx,[cs:prtnumber]
	mov ah,0	; print char
	int 0x17	; buggy spoolers trash BX here
	; returns status AH: MSB ... LSB are:
	; notBusy ACK noPaper SEL   ERROR x x TIMEOUT
	; and 0x30 equals 0x30 makes DOS assume no printer attached,
	; and we assume the same.
	mov [cs:prterror],ah	; remember status for debugging
;	xor ah,0x80		; 3/2008: BUSY bit is unreliable! (Oleg)
;	test ah, 0xa9		; 3/2008: BUSY bit is unreliable! (Oleg)
	test ah, 0x29
	jz prtokay1
	or byte [cs:busyflag],8	; should probably warn user
	; we might even want to retry until the error goes away!
	; especially annoying if inside data area of raster graphics.
prtokay1:
	mov al,ah
	and al,0x30
	cmp al,0x30
	jnz prtokay2
	or byte [cs:busyflag],4	; abort all further printing
prtokay2:
	pop dx
	pop bx
	pop ax
prtskip:
	ret

; ------------

%include "getpixel.asm"	; optimized pixel readers

; ------------

align 4

	db ">>"	; for debugging: make vars easy to find
oldi5vec	dd 0	; original int 5 vector

prtnumber	dw 0	; which printer to use (0..2)
getpixel	dw genericgetpixel	; pointer to getpixel (CX,DX) -> AL

gfxmode		db 0	; used graphics mode
gfxpage		db 0	; used graphics page (often unused)
busyflag	db 0	; set to 3 while active, 2 abort, 0 idle
			; 4 set means fatal printer error
			; 8 set means any printer error
prterror	db 0	; last printer status

xres	dw 320
xram	dw 320/8	; size of one line in graphics RAM
yres	dw 200
gfxseg	dw 0xa000

inverse	db 0	; set to 1 to print inverse colors
cgaback db 0	; set to 0 to assume that CGA color 0 is black
		; set to 1 or odd to process actual CGA color 0
		; (EPSON has "or 2" for "1 chunk per line" instead of 12/24)
random	db 0	; set to 1 to use random instead of ordered dither
		; does not affect POSTSCRIPT - which uses the
		; dithering engine of the printer instead.
compatmode:
	db 0	; set for HP Laserjet headers / footers in POSTSCRIPT
		; case and 120x72 dpi 8 pin instead of 180x180 dpi
		; 24 pin in EPSON case. Set 150 dpi instead of 300 dpi
		; in HP PCL case. For Epson, compatmode can be ORed with
		; 2 to indicate a real 9 pin printer (120x72 or 60x72)
economymode:
	db 0	; set to use only 50% of the ink / toner / ...

	db "<<"

palette	db 0,85,171,255	; non-CGA palettes are bigger but they are
		; stored at the same place (overwriting "install:")
%define buffer (palette+256)	; print data buffer

; ------------

%ifndef bufsize
%define bufsize 0 ; size of additional buffers, if any
%endif

install:
	mov es,[cs:0x2c]        ; environment segment
	; see Ralf Browns IntList table 1379 (PSP: 1378)
	mov ah,0x49
	int 0x21		; free the environment!
				; (jc "could not free... bla")
	push cs                 ; set segments to sane again
	pop es
	push cs
	pop ds

commandline:
	cld
	mov si,0x81		; command line parsing
clp_loop:
	lodsb
	cmp al,'?'	; help command
	jz help
	cmp al,'/'	; ignore /
	jz clp_loop
	cmp al,'-'	; ignore -
	jz clp_loop
	cmp al,9	; ignore whitespace
	jz clp_loop
	cmp al,' '	; ignore whitespace
	jz clp_loop
	jb parsedone
	cmp al,'a'
	jb clp_caseok
	sub al,'a'-'A'
clp_caseok:
	mov ah,1
	cmp al,'I'	; INVERSE?
	jnz ninverse
	mov [inverse],ah
	jmp clp_loop
ninverse:
	mov ah,1
	cmp al,'E'	; ECONOMY MODE?
	jnz neconomy
	mov [economymode],ah
	jmp clp_loop
neconomy:
	cmp al,'B'	; CGA BACKGROUND COLOR ENABLE?
	jnz ncgaback
	or [cgaback],ah
	jmp clp_loop
ncgaback:
	cmp al,"C"	; COMPATIBILITY MODE?
	jnz ncompat
	or [compatmode],ah
	jmp clp_loop
ncompat:
%ifdef EPSON
	cmp al,"9"	; 9 PIN COMPATIBILITY MODE? (2008 extension)
	jnz ncompat2
	mov al,2
	or [compatmode],al
	jmp clp_loop
ncompat2:
	cmp al,"X"	; X large stripes (whole line chunks) (2008 ext)
	jnz ncompat3
	mov al,2
	or [cgaback],al	; ugly location for (undocumented) X bit :-p
	jmp clp_loop
ncompat3:
%endif
%ifndef POSTSCRIPT	; only useful for dithering drivers
	cmp al,"R"	; RANDOM DITHER (NOT ORDERED DITHER)?
	jnz nrandom
	mov [random],ah
	jmp clp_loop
nrandom:
%endif
	cmp al,'1'	; PRINTER SELECTION?
	jb nselprn
	cmp al,'3'
	ja nselprn
	sub al,'1'
	mov ah,0
	mov [prtnumber],ax
	jmp clp_loop
nselprn:		; unknown?
help:	mov dx,helptext
	mov ah,9
	int 0x21
	mov ax,0x4c01	; do not go TSR
	int 0x21

parsedone:
	mov ax,0x3505	; get print screen interrupt vector
	int 0x21
	mov [ds:oldi5vec],bx
	mov [ds:oldi5vec+2],es

	push cs
	pop es		; re-set ES to CS, better style...

	mov ax,0x2505	; set print screen interrupt vector
	; "mov ds,cs"	; not needed for .com :-)
	mov dx,i5	; our new interrupt handler
	int 0x21

	mov dx,helloworld
	mov ah,9	; print string
	int 0x21

	mov dx,install+15+256+bufsize
	shr dx,1	; convert rounded up end of TSR to
	shr dx,1	; paragraphs for the DOS go TSR call
	shr dx,1	; (divide by 16, or better,
	shr dx,1	; shift right by 4)
	mov ax,0x3100	; go TSR
	int 0x21

; ------------

%include "messages.asm"	; contains help screen / hello screen texts

; ------------

%ifdef EPSON
filler  times 222 db 0  ; make COM look bigger so UPX will compress it :-p
        ; UPX only compresses files if the compressed version is at least
        ; 513 bytes smaller than the original, even though you already save
        ; disk space with less when you cross a cluster size boundary
%endif

; ------------

	; ****** sanity checking for defines follows ******

%ifndef EPSON
%ifndef POSTSCRIPT
%ifndef HPPCL
%error You must compile with either of -DEPSON or -DPOSTSCRIPT or -DHPPCL
%endif
%endif
%endif

%ifdef EPSON
%ifdef POSTSCRIPT
%error define only EPSON -or- POSTSCRIPT
%endif
%ifdef HPPCL
%error define only EPSON -or- HPPCL
%endif
%endif

%ifdef POSTSCRIPT
%ifdef HPPCL
%error define only HPPCL -or- POSTSCRIPT
%endif
%endif

