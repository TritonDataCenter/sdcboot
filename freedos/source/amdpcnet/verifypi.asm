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
;-----------------------------------------------------------------------
;
					; centralize msg & db/w/d move to
					; tail.asm (07-12-93, begin)
IFDEF	UTILITY
signature	db	'PKT DRVR',0	;
signature_len	equ	$-signature	;

packet_int_msg	db	CR,LF		;
		db	"Error: <packet_int_no> should be in the range 0x60 to 0x80"
		db	'$'		;
ENDIF
					; (07-12-93, end)

verify_packet_int:			; compared error indication passed
;
;----------------------------------------
; enter with no special registers.
; exit with cy,dx-> error message if the packet int was bad,
;  or nc,zr,es:bx -> current interrupt if there is a packet driver there.
;  or nc,nz,es:bx -> current interrupt if there is no packet driver there.
;----------------------------------------
;
	cmp	packet_int_no,60h	; check pkt int # > 60H
	jb	verify_packet_int_bad	; jump, if pkt int # out of range
	cmp	packet_int_no,80h	; check pkt int # < 80H
	jbe	verify_packet_int_ok	; jump, if pkt int # within of range
verify_packet_int_bad:			; pkt int # is bad
	mov	dx,offset packet_int_msg; DX = addr of pkt int message
	stc				; return set carry flag indicate error
	ret				; return to caller
verify_packet_int_ok:			; pkt int # within range
					;
	mov	ah,35h			; AH = get interrupt number
	mov	al,packet_int_no	; AL = packet interrupt #
	int	21h			; get packet int # in ES:BX
					; check if a signature aleardy exist
	lea	di,3[bx]		; DI = addr of our ISR's signature
	mov	si,offset signature	; SI = addr of signature string bytes
	mov	cx,signature_len	; CX = signature length
	repe	cmpsb			; compare the two signatures
	clc				; return clear carry flag indicate ok.
	ret				; return to caller
