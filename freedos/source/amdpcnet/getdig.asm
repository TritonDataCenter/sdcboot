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

PUBLIC	get_digit
;
;----------------------------------------
;enter with al = character
;return nc, al=digit, or cy if not a digit.
;----------------------------------------
;
get_digit:				;
	cmp	al,'0'			; check AL with char 0
	jb	get_digit_1		; jump, if AL less than char 0
	cmp	al,'9'			; check AL with char 9
	ja	get_digit_2		; jump, if AL greater than char 9
	sub	al,'0'			; AL = converted to binary
	clc				; clear carry flag
	ret				; return to caller
get_digit_2:
	cmp	al,'a'			; check AL with char a
	jb	get_digit_3		; jump, if AL less than char a
	cmp	al,'f'			; check AL with char f
	ja	get_digit_3		; jump, if AL greater than char f
	sub	al,'a'-10		; AL = converted to binary
	clc				; clear carry flag
	ret				; return to caller
get_digit_3:
	cmp	al,'A'			; check AL with char A
	jb	get_digit_1		; jump, if AL less than char A
	cmp	al,'F'			; check AL with char F
	ja	get_digit_1		; jump, if AL greater than char F
	sub	al,'A'-10		; AL = converted to binary
	clc				; clear carry flagclear carry flag
	ret				; return to callerreturn to caller
get_digit_1:				; error case, not a digit
	stc				; set carry flag
	ret				; return to caller



