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
;
;   This program is free software; you can redistribute it and/or modify
;   it under the terms of the GNU General Public License as published by
;   the Free Software Foundation, version 1.
;
;   This program is distributed in the hope that it will be useful,
;   but WITHOUT ANY WARRANTY; without even the implied warranty of
;   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
;   GNU General Public License for more details.
;
;   You should have received a copy of the GNU General Public License
;   along with this program; if not, write to the Free Software
;   Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
;-----------------------------------------------------------------------

;
;----------------------------------------
; Driver Revision & Network Interface equates
;----------------------------------------
;
majver		equ	2		; ver. number of the infrastructure.

MAX_ADDR_LEN	equ	16		; maxi. number of bytes in our address.

MAX_HANDLE	equ	10		; maxi. number of handles.

MAX_P_LEN	equ	8		; maxi. type length

MAX_MULTICAST	equ	8		; maxi. number of multicast addresses.

;
;----------------------------------------
; Packet Driver Network Interface Error numbers
;----------------------------------------
;
NO_ERROR	equ	0		; no error at all.
BAD_HANDLE	equ	1		; invalid handle number
NO_CLASS	equ	2		; no interfaces of specified class found
NO_TYPE		equ	3		; no interfaces of specified type found
NO_NUMBER	equ	4		; no interfaces of specified number found
BAD_TYPE	equ	5		; bad packet type specified
NO_MULTICAST	equ	6		; this interface does not support
					; multicast
CANT_TERMINATE	equ	7		; this packet driver cannot terminate
BAD_MODE	equ	8		; an invalid receiver mode was specified
NO_SPACE	equ	9		; operation failed because of
					; insufficient space
TYPE_INUSE	equ	10		; the type had previously been accessed,
					; and not released.
BAD_COMMAND	equ	11		; the command was out of range, or not
					; implemented
CANT_SEND	equ	12		; the packet couldn't be sent (usually
					; hardware error)
CANT_SET	equ	13		; hardware address couldn't be changed
					; (more than 1 handle open)
BAD_ADDRESS	equ	14		; hardware address has bad length or
					; format
CANT_RESET	equ	15		; Couldn't reset interface (more than
					; 1 handle open).
BAD_IOCB	equ	16		; an invalid iocb was specified

;
;----------------------------------------
; Ethernet Definitions.
;----------------------------------------
;
EADDR_LEN	equ	6		; Ethernet address length.
MAXI_DLEN	equ	1500		; maximun ethernet data length bytes
MINI_DLEN	equ	46		; minimun ethernet data length bytes
ETD_LEN		equ	2		; ethernet data length bytes
FCS		equ	4		; frame checksum bytes

ARCADDR_LEN	equ	1		;

					; smallest legal size packet, w/o fcs
RUNT		equ	EADDR_LEN * 2 + ETD_LEN + MINI_DLEN
					; largest legal size packet, w/o fcs
GIANT		equ	EADDR_LEN * 2 + ETD_LEN + MAXI_DLEN

BLUEBOOK	equ	1		; class # of Bluebook ethernet
IEEE8023	equ	11		; class # of 802.3 with 802.2 header

;
;----------------------------------------
; Bits in sys_features
;----------------------------------------
;
MICROCHANNEL	equ	02		; a micro channel computer
TWO_8259	equ	40h		; 2nd 8259 exists

;
;----------------------------------------
; Bits in flagbyte
;----------------------------------------
;
CALLED_ETOPEN	equ	1		; have called etopen
D_OPTION	equ	2		; delayed initialization
N_OPTION	equ	4		; Novell protocol conversion
W_OPTION	equ	8		; Windows upcall checking.

;
;----------------------------------------
; Misc. equates
;----------------------------------------
;
HT		equ	09h		;
CR		equ	0dh		;
LF		equ	0ah		;

CY		equ	0001h		;
EI		equ	0200h		;

DONE		equ	1		; I/O complete flag
CALLME		equ	2		; Please upcall me flag

IFNDEF	UTILITY
;
;----------------------------------------
; Device Driver Error numbers
;----------------------------------------
;
	;
	;----------------------------------------
	; non-init error message equates
	;----------------------------------------
	;
USAGE_HELP	equ	1		; usage error message number
RESET_BAD	equ	2		; reset PCnet error message
INIT_BAD	equ	3		; init PCnet error message
VDS_BAD		equ	4		; VDS memory failure
DRIVER_EXIST	equ	5		; PCnet driver already exist
IRQ_RANGE	equ	6		; IRQ range error
DRIVER_INIT_BAD	equ	7		; PCnet driver init device error
INT_RANGE	equ	8		; INT range error

ENDIF
;
;----------------------------------------
; Structures
;----------------------------------------
;
segmoffs	struc			; defines offs as 0, segm as 2
offs		dw	?		;
segm		dw	?		;
segmoffs	ends			;

iocb		struc			; as_send_pkt structure
buffer		dd	?		; Pointer to the buffer
len		dw	?		; Its length
flags		db	?		; Some flags
ret_code	db	?		; Completion code
upcall		dd	?		; I/O completion upcall
next		dd	?		; Private next pointer (queue)
resv		db	4 dup (?)	; Unused private data
iocb		ends

;
;-----------------------------------------------------------------------
;The following two macros are used to manipulate port addresses.
;Use loadport to initialize dx.  Use setport to set a specific port on
;the board.  setport remembers what the current port number is, but beware!
;setport assumes that code is being executed in the same order as the
;code is presented in the source file.  Whenever this assumption is violated,
;you need to enter another loadport.  Some, but not all examples are:
;in a loop with multiple setports, or a backward jump over a setport, or
;a forward jump over a setport.  If you have any doubt, consult the
;individual driver sources for examples of usage.  If you suspect that
;you have too few loadports, define the symbol "no_confidence" to a
;one.  This will force a loadport before every setport.  If you wish to turn
;it off for some of your code, redefine it to a zero.
;-----------------------------------------------------------------------
;

loadport	macro
	mov	dx,io_addr		;
port_no	=	0			;
	endm				;
;
;----------------------------------------
;change the port number from the current value to the new value.
;----------------------------------------
;
setport	macro	new_port_no
	ifdef	no_confidence		; define if you suspect that you don't
	  if	no_confidence
		loadport		;  have enough loadports, i.e. dx is
	  endif
	endif				;  set to the wrong port.
	if	new_port_no - port_no EQ 1
		inc	dx
	else
		if	new_port_no - port_no EQ -1
			dec	dx
		else
			if	new_port_no - port_no NE 0
				add	dx,new_port_no - port_no
			endif
		endif
	endif
port_no	=	new_port_no
	endm
;
;----------------------------------------
;this macro writes the given character to the given row and column on
;  an CGA.
;----------------------------------------
;
to_scrn	macro	r, c, ch		;
	push	bx			;
	push	es			;
	mov	bx,0b800h		;
	mov	es,bx			;
	mov	bx,es:[r*160+c*2]	;
	inc	bh			;
	and	bh,07h			;
	mov	bl,ch			;
	mov	es:[r*160+c*2],bx	;
	pop	es			;
	pop	bx			;
	endm				;

send_queueempty	macro
;
;----------------------------------------
; Check if send queue is empty.
; Enter with interrupts disabled.
; Exit with zr (zero) if empty, nz (not zero) if not.
; Destroys ax.
;----------------------------------------
;
	mov	ax,word ptr send_head	; Queue empty?
	or	ax,word ptr send_head+2	;
	endm

send_peekqueue	macro
;
;----------------------------------------
; Peek into the queue and get the next entry.
; Enter with interrupts disabled.
; Exit with es:di -> iocb.
;----------------------------------------
;
	les	di,send_head		; Get head segment:offset
	endm				;


