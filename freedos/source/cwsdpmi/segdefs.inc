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

;	.386p
DGROUP	group	_DATA,_BSS

;------------------------------------------------------------------------

start_code16	macro
;_TEXT	segment	byte public 'code' use16
_TEXT	segment	dword public 'code' use16
	assume	cs:_TEXT,ds:DGROUP,ss:DGROUP
	endm

end_code16	macro
;	align	10h
_TEXT	ends
	endm

start_code16
end_code16

;------------------------------------------------------------------------

start_data16	macro
_DATA	segment word public 'DATA' use16
	endm

end_data16	macro
;	align	10h
_DATA	ends
	endm

start_data16
end_data16

;------------------------------------------------------------------------

start_bss	macro
_BSS	segment	word public 'BSS'
	endm

end_bss	macro
;	align	10h
_BSS	ends
	endm

start_bss
end_bss

	.386p

jmpt	macro	sel
	db	0eah
	dw	0,sel
	endm

