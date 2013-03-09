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
;
;
;we read the timer chip's counter zero.  It runs freely, counting down
;from 65535 to zero.  Whenever the count is above the old count, the
;timer must have wrapped around.  When that happens, we count down a tick.
;-----------------------------------------------------------------------
;

timeout		dw	?		; number of ticks to wait.
timeout_counter	dw	?		; old counter zero value.

;
;-----------------------------------------
;enter with ax = number of ticks (36.4 ticks per second).
;-----------------------------------------
;
set_timeout:
	inc	ax			; the first times out immediately.
	mov	cs:timeout,ax		;
	mov	cs:timeout_counter,0	;
	ret

;
;-----------------------------------------
;call periodically when checking for timeout.  Returns nz if we haven't
;timed out yet.
;-----------------------------------------
;
do_timeout:
	mov	al,0			; latch counter zero.
	out	43h,al			;
	in	al,40h			; read counter zero.
	mov	ah,al			;
	in	al,40h			;
	xchg	ah,al			;
	cmp	ax,cs:timeout_counter	; is the count higher?
	mov	cs:timeout_counter,ax	;
	jbe	do_timeout_1		; no.
	dec	cs:timeout		; Did we hit the timeout value yet?
	ret				;
do_timeout_1:
	or	sp,sp			; ensure nz.
	ret				;

