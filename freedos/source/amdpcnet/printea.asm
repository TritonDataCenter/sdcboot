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
;  Copyright, 1988-1992, Russell Nelson, Crynwr Software
;-----------------------------------------------------------------------
;
PUBLIC	print_ether_addr
;
;----------------------------------------
;
; print ethernet address
;----------------------------------------
;
print_ether_addr:			;
	mov	cx,EADDR_LEN		; CX = ethernet address length
print_ether_addr_0:			;
	push	cx			; save loop count
	lodsb				; AL = DS:SI
	mov	cl,' '			; CL = char ' ', include leading zero
	call	byteout			; convert/print AL into hex char
	pop	cx			; restore loop count
	cmp	cx,1			; check loop count = last char
	je	print_ether_addr_1	; jump, if loop count reach last one
	mov	al,':'			; AL = char ':'
	call	chrout			; print AL = char
print_ether_addr_1:			;
	loop	print_ether_addr_0	; loop, until counter exhausted
	ret				; return to caller

