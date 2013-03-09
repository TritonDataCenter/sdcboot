; Copyright (C) 1995,1996 CW Sandmann (sandmann@clio.rice.edu) 1206 Braelinn, Sugarland, TX 77479
;
; This file is distributed under the terms listed in the document
; "copying.cws", available from CW Sandmann at the address above.
; A copy of "copying.cws" should accompany this file; if not, a copy
; should be available from where this file was obtained.  This file
; may not be distributed without a verbatim copy of "copying.cws".

; Enters DPMI protected mode and performs an "exit" with magic to unload TSR

	title	unload
	include	segdefs.inc
	start_data16

dpmi_protsw	dd	0
no_dpmi		db	'CWSDPMI not removed', 13, 10, '$'

	end_data16

	start_code16

	public	_unload_tsr
_unload_tsr:

	mov	ax,1687h
	int	2fh				;DPMI detection
	or	ax,ax
	jz	short dpmi_present
	
exit_error:
	mov	dx,offset DGROUP:no_dpmi
	mov	ah,9
	int	21h

	mov	ax,4c01h
	int	21h

dpmi_present:
	mov	word ptr dpmi_protsw,di
	mov	word ptr dpmi_protsw+2,es
	mov	bx,si

	or	bx,bx				;Allocate memory for DPMI
	jz	short no_para
	mov	ah,48h
	int	21h
	jc	short exit_error
	mov	es,ax

no_para:
	mov	ax,1
	call	dpmi_protsw
	jc	short exit_error

; We are in protected mode

	mov	ebx,69151151h			; Magic for unload
	mov	ax,4c00h
	int	21h

	end_code16
	end
