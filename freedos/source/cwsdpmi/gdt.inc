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

gdt_s	struc
lim0	dw	?
base0	dw	?
base1	db	?
stype	db	?	; type, DT, DPL, present
lim1	db	?	; limit, granularity
base2	db	?
gdt_s	ends

IFNDEF run_ring
run_ring equ	3	; also in gdt.h, must be consistent
ENDIF

g_zero  equ	0*8
g_gdt	equ	1*8
g_idt	equ	2*8
g_rcode equ	3*8
g_rdata equ	4*8
g_pcode	equ	5*8+run_ring
g_pdata	equ	6*8+run_ring
g_core  equ	7*8+run_ring
g_BIOSdata equ  8*8

g_vcpicode	equ	9*8	;for VCPI Call Selctor in Protect Mode
g_vcpireserve0  equ	10*8
g_vcpireserve1  equ	11*8

g_atss  equ	12*8
g_ctss  equ	13*8
g_itss  equ	14*8
g_ldt	equ	15*8
g_iret	equ	16*8

g_num	equ	17

spare_stack	equ	128	;bytes for HW interrupts, generic interrupts
