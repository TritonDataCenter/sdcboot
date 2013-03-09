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

PUBLIC	dwordout, wordout, byteout, digout
;
;----------------------------------------
; convert binary to hex char for various length
;----------------------------------------
;
dwordout:				; convert/print DX:AX double word
	mov	cl,'0'			; CL = char 0, assume laeding zero 
	xchg	ax,dx			; AX = high word, DX = low word
	call	wordout			; convert/print AX word 
	xchg	ax,dx			; AX = low word, DX = high word
wordout:				; convert/print AX word
	push	ax			; save word to convert/print
	mov	al,ah			; AL = high byte, AH = low byte
	call	byteout			; convert/print AL byte
	pop	ax			; restore word to convert/print
byteout:				; convert/print AL byte
	mov	ah,al			; AH = AL = input byte
	shr	al,1			; shift high nibble to low nibble
	shr	al,1			;
	shr	al,1			;
	shr	al,1			;
	call	digout			; convert/print AL low nibble
	mov	al,ah			; restore AL = input byte
digout:					; convert/print AL low nibble
	and	al,0fh			; mask out high nibble
	add	al,90h			; convert binary to ascii hex digit.
	daa				;
	adc	al,40h			;
	daa				;
	cmp	al,cl			; check AL = '0', leading zero
	je	digout_1		; jump, if leading zero
	mov	cl,-1			; CL = 0xffff indicate no more leading zero
	jmp	chrout			; print AL = char
digout_1:
	ret				; return to caller

