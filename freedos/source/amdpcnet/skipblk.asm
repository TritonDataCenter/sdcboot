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

PUBLIC	skip_blanks
;
;----------------------------------------
; enter with ds:si --> start of the string 
; return al(1st non-blank char), si(1st non-blank position)
;----------------------------------------
;
skip_blanks:				;
	lodsb				; AL = value at DS:SI
	cmp	al,' '			; check AL = blank
	je	skip_blanks		; loop, if AL = blank
	cmp	al,HT			; check AL = ASCII 9
	je	skip_blanks		; loop, if AL = ASCII 9
IFNDEF	UTILITY
	cmp	al,'='			; check AL = '='
	je	skip_blanks		; loop, if AL = '='
ENDIF
	dec	si			; SI = next address
	ret				; return to caller
