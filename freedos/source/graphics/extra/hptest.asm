	; this prints a test pattern at various resolutions
	; on HP PCL printers

	org 100h
start:
	mov dx,warning
	mov ah,9
	int 0x21

	xor ax,ax
	int 0x16

	cmp al,27
	jz lesserrors
	cmp ah,27
	jz lesserrors

	mov si,initsequence
	call pstring
	mov di,thedpival

nextdpi:
	mov si,description
	call pstring
	mov si,di
	call pstring	; print resolution as text
	mov si,resolution1
	call pstring	; start resolution configuration
	mov si,di
	call pstring	; configure resolution
	mov si,resolution2
	call pstring	; end resolution configuration

	call patty	; 4 pixels high
	call patty

	call pat4x1	; pat4x... are 16 pixels high each
	call pat4x2
	call pat4x1
	call pat4x2
	call pat4x3
	call pat4x3

	call patty
	call patty

	mov si,endgraphics
	call pstring

	add di,4	; go to next resolution
	mov al,[di]
	or al,al	; EOF ?
	jnz nextdpi

	mov si,endsequence
	call pstring

	mov ax,[ds:errors]
	cmp ax,0xff
	jbe lesserrors
	mov ax,0xff	; cannot return errorlevel above 255
lesserrors:
	mov ah,0x4c
	int 0x21

pat4x1:	call pat1
	call pat1
	call pat1
	call pat1
	ret

pat4x2:	call pat2
	call pat2
	call pat2
	call pat2
	ret

pat4x3:	call pat1
	call pat2
	call pat1
	call pat2
	ret

pat1:	mov si,pattern1
	call pstring
	mov si,pattern2
	call pstring
	mov si,pattern3
	call pstring
	mov si,pattern4
	call pstring
	ret

pat2:	mov si,pattern4
	call pstring
	mov si,pattern3
	call pstring
	mov si,pattern2
	call pstring
	mov si,pattern1
	call pstring
	ret

patty:	mov si,pattern1
	call pstring
	mov si,pattern2
	call pstring
	mov si,pattern1
	call pstring
	mov si,pattern2
	call pstring
	ret

pstring:
	mov al,[si]
	or al,al
	jz pstrdone
	xor dx,dx	; LPT1
	mov ah,0	; print char
	int 0x17	; printer
	mov al,ah	; status
	xor al,0x80
	test al,0xa9	; any normal error?
	jz noperror
	inc word [errors]
noperror:
	and al,0x30
	cmp al,0x30
	jz noprinter	; no printer attached
	inc si
	jmp short pstring
pstrdone:
	ret

noprinter:
	mov ax,0x4c01
	int 0x21

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


errors	dw 0	; counting errors here

warning	db "Printing HP PCL 3 (75 to 600 dpi) b/w test patterns now."
	db 13,10
	db "Press ESC now to abort, any other key to go on!"
	db 13,10
	db "$"

initsequence	db 27,"E"		; init sequence (300 dpi)
		db 27,"*o1M"	; -1=300x300 dpi EconoFast mode
				; 0=600x300, 1=600x600 maximum mode
		db 27,"*o0M"	; shingling: 0/1/2=1/2/4 passes.
			; (printer lets ink dry longer that way)
		db 27,"*o3D"	; maximum depletion, no gamma correct.
		db 0


description	db "Test pattern, 112x112 pixels, resolution ",0

resolution1	db "dpi:",13,10,27,"*t",0

thedpival	db "600",0
		db "300",0
		db "200",0
		db "150",0
		db "100",0
		db "75",0,0
		db 0	; EOF


resolution2	db "R",27,"*r1A"		; ___dpi, left
		db 27,"*r1U",27,"*b0M",0	; b/w, uncompressed

		; parts of some checkerboard patterns:
pattern1	db 27,"*b14W"	; send 14 bytes of graphics data
		db 0x55,0x55,0x55,0x55,0x18
		db 0x33,0x33,0x33,0x33,0x18
		db 0x88,0x88,0x88,0x88,0
pattern2	db 27,"*b14W"
		db 0xaa,0xaa,0xaa,0xaa,0x18
		db 0x33,0x33,0x33,0x33,0x18
		db 0x44,0x44,0x44,0x44,0
pattern3	db 27,"*b14W"
		db 0x55,0x55,0x55,0x55,0x18
		db 0xcc,0xcc,0xcc,0xcc,0x18
		db 0x22,0x22,0x22,0x22,0
pattern4	db 27,"*b14W"
		db 0xaa,0xaa,0xaa,0xaa,0x18
		db 0xcc,0xcc,0xcc,0xcc,0x18
		db 0x11,0x11,0x11,0x11,0

endgraphics	db 27,"*rC",13,10,0
		; HP homepage: "ESC*rC" ajp.clara.net: "ESC*rB" ...!
		; HP seems to be right, ajp seems to be wrong.

endsequence	db 13,10,12	; end of print job
		db 27,"E"	; reset printer / end print job
		db 0


	; Newer printers allow medium type and quality setting:
	; ***	db 27,"&l4M"	; glossy/transparency
		; (0=normal 1=bond 2=premium 3=glossy paper 4=transp.)
	; ***	db 27,"*r1Q"	; quality: 1 draft 2 letter
	; Those settings are mapped to shingling and depletion:
	; Shingling 0/1/2 means 1/2/4 passes to give ink time to dry.
	; Depletion (0/1=off) is 2/3=25/50%, 4/5=25/50% with gamma
	; correction -> hardware removes dots to reduce ink bleeding
	; (older printers do not allow gamma corrected depletion).


