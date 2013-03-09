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

PUBLIC	print_number			;
;
;----------------------------------------
; enter with dx -> dollar terminated name of number,
; di ->dword.
; exit with the number printed and the cursor advanced to the next line.
;----------------------------------------
;
print_number:				;
					; removed, in order to centralize
					; print message
IFDEF	UTILITY
	mov	ah,9			; print the name of the number.
	int	21h
ENDIF
	mov	al,'0'			; AL = ascii 0
	call	chrout			; print character
	mov	al,'x'			; AL = ascii x
	call	chrout			; print character
	mov	ax,[di]			; AX = value at DS:DI
	mov	dx,[di+2]		; DX = value at DS:(DI+2)
	call	dwordout		; print double word in hex format

	mov	al,' '			; AL = ascii blank
	call	chrout			; print character
	mov	al,'('			; AL = ascii left parentheses
	call	chrout			; print character
	mov	ax,[di]			; AX = value at DS:DI
	mov	dx,[di+2]		; DX = value at DS:(DI+2)
	call	decout			; print double word in decimal format

	mov	al,')'			; AL = ascii right parentheses
	call	chrout			; print character
	mov	al,CR			; AL = ascii CR
	call	chrout			; print character
	mov	al,LF			; AL = ascii LF
	call	chrout			; print character
	ret				; return to caller

