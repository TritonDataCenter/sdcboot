; Copyright (C) 1995,1996 CW Sandmann (sandmann@clio.rice.edu) 1206 Braelinn, Sugarland, TX 77479
; Copyright (C) 1993 DJ Delorie, 24 Kirsten Ave, Rochester NH 03867-2954
;
; This file is distributed under the terms listed in the document
; "copying.cws", available from CW Sandmann at the address above.
; A copy of "copying.cws" should accompany this file; if not, a copy
; should be available from where this file was obtained.  This file
; may not be distributed without a verbatim copy of "copying.cws".
;
; This file is distributed WITHOUT ANY WARRANTY; without even the implied
; warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

tss_s		struc
tss_back_link	dw	?
		dw	?
tss_esp0	dd	?
tss_ss0		dw	?
		dw	?
tss_esp1	dd	?	;Not really, see tss.h
tss_ss1		dw	?
		dw	?
tss_esp2	dd	?	;Not really, see tss.h
tss_ss2		dw	?
		dw	?
tss_cr3		dd	?
tss_eip		dd	?
tss_eflags	dd	?
tss_eax		dd	?
tss_ecx		dd	?
tss_edx		dd	?
tss_ebx		dd	?
tss_esp		dd	?
tss_ebp		dd	?
tss_esi		dd	?
tss_edi		dd	?
tss_es		dw	?
		dw	?
tss_cs		dw	?
		dw	?
tss_ss		dw	?
		dw	?
tss_ds		dw	?
		dw	?
tss_fs		dw	?
		dw	?
tss_gs		dw	?
		dw	?
tss_ldt		dw	?
		dw	?
tss_trap	dw	?
tss_iomap	db	?
tss_irqn	db	?
tss_cr2		dd	?
tss_error	dd	?
		dw	64 dup (?)
tss_stack	dw	?
		ends
