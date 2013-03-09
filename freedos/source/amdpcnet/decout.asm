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
;put into the public domain by Russell Nelson, nelson@crynwr.com
;-----------------------------------------------------------------------

PUBLIC	decout				;
;
;----------------------------------------
; print decimal valuein DX:AX
;----------------------------------------
;
decout:					;
	mov	si,ax			; SI = AX = low wrod
	mov	di,dx			; DI = DX = high word
	or	ax,dx			; check DX:AX = zero
	jne	decout_nonzero		; jump, if DX:AX != 0
	mov	al,'0'			; AL = char '0'
	jmp	chrout			; print AL char
decout_nonzero:				;

	xor	ax,ax			; AX = 0, high storage
	mov	bx,ax			; BX = 0, middle storage
	mov	bp,ax			; BP = 0, low storage

	mov	cx,32			; CX = double word length(DX:AX)
decout_1:				;
	shl	si,1			; SI=low word shift MSB to carry
	rcl	di,1			; DI=high word rotate MSB to carry
	xchg	bp,ax			; BP = high , AX = low
	call	addbit			; use carry adjust low storage
	xchg	bp,ax			; AX = high, BP = low
	xchg	bx,ax			; AX = middle, BX = high 
	call	addbit			; use carry adjust middle storage 
	xchg	bx,ax			; AX = high, BX = middle 
	adc	al,al			; AL = AL + AL + Carry
	daa				; decimal adjust AL after addition
	loop	decout_1		; loop, until counter exhaust

	mov	cl,'0'			; CL='0',eliminate leading zeroes flag
	call	byteout			; print AL = high byte
	mov	ax,bx			; AX = middle word
	call	wordout			; print AX = middle word
	mov	ax,bp			; AX = low word
	jmp	wordout			; print AX = low word

addbit:	
	adc	al,al			; AL = AL + AL + Carry
	daa				; decimal adjust AL after addition
	xchg	al,ah			; AL = high byte, AH = low byte
	adc	al,al			; AL = AL + AL + Carry
	daa				; decimal adjust AL after addition
	xchg	al,ah			; AL = low byte, AH = high byte
	ret				; return to caller


