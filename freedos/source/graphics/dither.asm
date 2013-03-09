; GRAPHICS tool for FreeDOS - GPL by Eric Auer eric@coli.uni-sb.de 2003
; Please go to www.gnu.org if you have no copy of the GPL license yet.

; prtstr          - print 0 terminated string starting at CS:SI
; ditherBWordered - set CY with probability proportional to AL
;   (using "mirrored counter" as "randomness" source)
;   (to set seed, set ditherTemp, e.g. from weaveTab)
; ditherBWrandom  - set CY with probability proportional to AL
;   (using a linear congruential pseudo random number generator)
;   (to set seed, set ditherSeed, not really needed)

; CUTOFF is where the dithering should flip over to "completely black"
; or "completely white" respectively (gray levels CUTOFF / 255-CUTOFF)
; 2 seems to be a good value. Minimum is 0, theoretical maximum 127,
; but 127 would practically disable dithering.
%ifndef CUTOFF
%define CUTOFF 2
%endif

; ------------

prtstr:	push ax		; print 0 terminated string starting at CS:SI
prtstrloop:
	mov al,[cs:si]
	inc si
	or al,al
	jz short prtstrdone
	call prtchar		; print one char
	jmp short prtstrloop	; loop
prtstrdone:
	pop ax
	ret

; ------------

	; *** use inverse counter as pseudorandom for dither ***
	; (this is intended to mix ordered and random dither - Eric)
ditherBWordered:			; set CY with probability (AL/255)
	inc byte [cs:ditherTemp]	; simply count... (in any case!)
	cmp al,255-CUTOFF
	jae trueism
	cmp al,CUTOFF
	jbe falseism
	push ax
	push bx
	push cx
	mov ch,al			; remember AL
	mov bl,[cs:ditherTemp]		; a simple counter
; --- 	add bl,[cs:ditherWeave]		; weaving
	mov bh,0
	push bx
	and bl,15			; low nibble ...
	mov al,[cs:ditherTable+bx]	; mirror it
	mov cl,4
	shl al,cl			; ... becomes high nibble
	mov [cs:ditherValue],al
	pop bx
	shr bl,cl			; high nibble ...
	and bl,15
	mov al,[cs:ditherTable+bx]	; mirror it
	or al,[cs:ditherValue]		; ... becomes low nibble
	; mov [cs:ditherValue],al	; (not used)
	cmp al,ch			; compare to user AL
	; CY set if random < user, so higher probability if user FF
	; never set if user value is 00, correct...
	pop cx
	pop bx
	pop ax
	ret

	; this is a standard linear congruential 16bit
	; PseudoRandomNumberGenerator using ditherSeed
ditherBWrandom:			; set CY with probability (AL/255)
	cmp al,255-CUTOFF
	jae trueism
	cmp al,CUTOFF
	jbe falseism
	push ax
	push bx
	push dx
	mov bx,ax			; remember AL
	mov ax,[cs:ditherSeed]
	mov dx,25173
	mul dx
	add ax,13849
	mov [cs:ditherSeed],ax
	xor al,ah			; combine both parts
	cmp al,bl			; compare to user AL
	; CY set if random < user, so higher probability if user FF
	; never set if user value is 00, correct...
	; slight error: only almost always set if user value is FF
	; (so black has a few white spots)
	pop dx
	pop bx
	pop ax
	ret


trueism:	; ensure that black is printed as black
	stc	; by ensuring 100% probability if AL is 255
	ret

falseism:	; ensure that white is white - do we need this?
	clc
	ret

; ------------

ditherValue	db 0		; pseudorandom value for dithering
ditherTemp	db 0		; helper value for pseudorandom
ditherTable	db 0,8,4,12,2,10,6,14,1,9, 5,13, 3,11, 7,15
		;  0 1 2  3 4  5 6  7 8 9 10 11 12 13 14 15
		; table for mirrored counting in a nibble (ordered d.)
; --- ditherWeave	db 0	; offset for weaving
weaveTab	; db 4,6,4,2	; weaving cycle list
		db 15, 7,11, 3,13, 5,9,1,14,6,10,2,12,4,8,0

ditherSeed	dw 1		; for linear congruential PRNG

