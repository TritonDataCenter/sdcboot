;-----------------------------------------------------------------------
;Copyright (c) 1993 ADVANCED MICRO DEVICES, INC. All Rights Reserved.
;This software is unpblished and contains the trade secrets and 
;confidential proprietary information of AMD. Unless otherwise provided
;in the Software Agreement associated herewith, it is licensed in confidence
;"AS IS" and is not to be reproduced in whole or part by any means except
;for backup. Use, duplication, or disclosure by the Government is subject
;to the restrictions in paragraph (b) (3) (B) of the Rights in Technical
;Data and Computer Software clause in DFAR 52.227-7013 (a) (Oct 1988).
;Software owned by Advanced Micro Devices, Inc., 901 Thompson Place,
;Sunnyvale, CA 94088.
;-----------------------------------------------------------------------
;  Copyright 1988-1992 Russell Nelson
;
; put into the public domain by Russell Nelson, nelson@crynwr.com
;-----------------------------------------------------------------------

PUBLIC	get_number			;
;
;----------------------------------------
;
;----------------------------------------
;
get_number:				;
	mov	bp,10			; BP = 10 (default decimal).
	jmp	short get_number_0	; jump to get number

PUBLIC	get_hex				;

get_hex:				;
	mov	bp,16			; BP = 16 (heximal)
;
;----------------------------------------
;get a hex number, skipping leading blanks.
;enter with si->string of digits,
;	    di -> dword to store the number in.
;	    [di] is not modified if no digits are given, so it acts as the default.
;return cy if there are no digits at all.
;return nc, bx:cx = number, and store bx:cx at [di].
;----------------------------------------
;
get_number_0:				;
	call	skip_blanks		; skip blank & ASCII 9
	call	get_digit		; check a decimal or heximal number
	jc	get_number_3		; jump, if carry flag set
	xor	ah,ah			; AH = 0
	cmp	ax,bp			; check AX with BP
	jae	get_number_3		; jump, if AX >= BP, number base diff
	or	al,al			; AL = AL OR AL
	jne	get_number_4		; jump, if AL != 0
	mov	bp,8			; BP = 8 (octal)
get_number_4:				;

	xor	cx,cx			; CX=0(low word,ascii2binary conversion)
	xor	bx,bx			; BX=0(high word,ascii2binary conversion)
get_number_1:				;
	lodsb				; AX = read a char byte from DS:SI
	cmp	al,'x'			; check AL = char x
	je	get_number_5		; jump, if AL = char x(heximal)
	cmp	al,'X'			; check AL = char X
	je	get_number_5		; jump, if AL = char X(heximal)
	call	get_digit		; AL = converted from char to binary
	jc	get_number_2		; jump, if carry flag set, error
	xor	ah,ah			; AH = 0
	cmp	ax,bp			; check digit(AX) >  # base(BP)
	jae	get_number_2		; jump, if digit > # base.
					; a leagal # is found
	push	ax			; save the new digit(AX).
					; multiply the low word by # base.
	mov	ax,bp			; AX = BP (# base)
	mul	cx			; CX = previous low word
	mov	cx,ax			; CX = low word, mult. CX(pre-low-word)
	push	dx			; save DX = high word, mult. CX(pre-low-word)
	mov	ax,bp			; AX = # base
	mul	bx			; BX = previous high word
	mov	bx,ax			; BX = low word, mult. BX(pre-high-word)
	pop	dx			; restore DX = high word, mult CX(pre-low-word)
	add	bx,dx			; BX = low(pre-BX) + high(pre-CX)word

	pop	ax			; restore AX = new digit
	add	cx,ax			; CX(low word) = low word + new digit
	adc	bx,0			; increment BX(high word) if carry
	jmp	get_number_1		; loop, for other digits
get_number_5:				; if user select hex number
	mov	bp,16			; BP = hex # base
	jmp	get_number_1		; loop, for other digits
get_number_2:				; if error or complete number parse
	dec	si			; back to last digit
	mov	[di],cx			; store low word's number
	mov	[di+2],bx		; store high word's number
	clc				; return clear carry flag indicate ok.
	jmp	short get_number_6	; jump to exit
get_number_3:				; if input format error
	cmp	al,'?'			; check user help option(default)
	stc				; return set carry flag indicate error
	jne	get_number_6		; jump, if not user help option
	add	si,2			; SI =SI+2,skip the help option char
	mov	cx,-1			; CX = 0xFFFF
	mov	bx,-1			; BX = 0xFFFF
	jmp	get_number_2		; return both high,low words as 0xFFFF
get_number_6:				;
	ret				; return to caller

