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
; 06-13-94 D.T.	specify the segment registers for clarify purpose
;
;-----------------------------------------------------------------------
.286

INCLUDE	defs.asm

SHARE_IRQ	equ	1		; share IRQ enable
;
;----------------------------------------
; CPU flag equates
;----------------------------------------
;
CY	equ	0001h			; carry flag bit position
EI	equ	0200h			; interrupt enable flag bit position

;
;----------------------------------------
; increment statistics list records macro 
;----------------------------------------
;
linc	macro	n			; inc a 32 bit integer
	local	a			; local variable
	inc	n			; increment the low word
	jne	a			; go if not overflow
	inc	n+2			; increment the high word
a:					;
	endm				;

;
;----------------------------------------
; handle structure
;----------------------------------------
;
per_handle	struc			;
in_use		db	0		; non-zero if this handle is in use.
packet_type	db	MAX_P_LEN dup(0); associated packet type.
packet_type_len	dw	0		; associated packet type length.
receiver	dd	0		; receiver handler.
receiver_sig	db	8 dup(?)	; signature at the receiver handler.
class		db	?		; interface class
per_handle	ends			;

;
;----------------------------------------
; registers(words) structure for stack
;----------------------------------------
;
regs		struc			; stack offsets of incoming regs
_ES		dw	?		;
_DS		dw	?		;
_BP		dw	?		;
_DI		dw	?		;
_SI		dw	?		;
_DX		dw	?		;
_CX		dw	?		;
_BX		dw	?		;
_AX		dw	?		;
_IP		dw	?		;
_CS		dw	?		;
_F		dw	?		; flags, Carry flag is bit 0
regs		ends			;

;
;----------------------------------------
; registers(words:ES,DS,BP,DI,SI/bytes:A/B/C/DL & H) structure for stack
;----------------------------------------
;
bytes		struc			; stack offsets of incoming regs
		dw	?		; es, ds, bp, di, si are 16 bits
		dw	?		;
		dw	?		;
		dw	?		;
		dw	?		;
_DL		db	?		;
_DH		db	?		;
_CL		db	?		;
_CH		db	?		;
_BL		db	?		;
_BH		db	?		;
_AL		db	?		;
_AH		db	?		;
bytes		ends			;

;
;----------------------------------------
;
;----------------------------------------
;
code	segment word public		;
	assume	cs:code, ds:code	;

EXTRN	int_no: byte			; IRQ,I/O addr,DMA info. struct ptr

EXTRN	driver_class: byte		; driver class info. pointer
EXTRN	driver_type: byte		; driver type info. pointer
EXTRN	driver_name: byte		; driver name info. pointer
EXTRN	driver_function: byte		; driver function info. pointer
EXTRN	parameter_list: byte		; driver param. list struct info. ptr
EXTRN	rcv_modes: word			; count of modes followed by mode handles.

EXTRN	start_1: near			; 
EXTRN	start_x: near			; PCnet-ISA H/S reset handle
EXTRN	send_pkt: near			; drv xmit packet routine
EXTRN	as_send_pkt: near		; drv async. xmit packet routine
EXTRN	drop_pkt: near			; drv drop a packet from queue routine
EXTRN	get_address: near		; drv get interface address routine
EXTRN	set_address: near		; drv set interface address routine
EXTRN	terminate: near			; drv stop rcv pkt & host DMA cascade
EXTRN	reset_interface: near		; drv reset interface
EXTRN	xmit: near			; drv process xmit interrupt
EXTRN	recv: near			; drv receive ISR
EXTRN	recv_exiting: near		; drv receive ISR portion after IACK
EXTRN	etopen: near			; drv put host DMA into cascade mode
EXTRN	set_multicast_list: near	; drv multicast address list
IFDEF	SHARE_IRQ
EXTRN	share_isr: near			; check our interrupt condition
ENDIF

PUBLIC	phd_environ			; ptr to DOS environ var addr word
PUBLIC	phd_dioa			; ptr to DOS cmd tail & buffer bytes
PUBLIC	packet_int_no			; ptr to pkt drv int # bytes(4)
PUBLIC	is_at				; ptr to PC AT flag byte
PUBLIC	sys_features			; ptr to sys feature byte(MC/2nd 8259)
PUBLIC	flagbyte			; ptr to user options/card init. flagbyte
PUBLIC	multicast_count			; ptr to multicast addresses count word.
PUBLIC	multicast_addrs			; ptr to multicast address bytes
PUBLIC	multicast_broad			; ptr to broadcast entry bytes
PUBLIC	send_head			; ptr to head of xmit queue
PUBLIC	send_tail			; ptr to end of xmit queue
PUBLIC	our_isr				; ptr to our pkt drv ISR routine
PUBLIC	their_isr			; ptr to original pkt drv ISR stoe address(dword)

PUBLIC	re_enable_interrupts		; ptr to re-enable interrupt routine
PUBLIC	set_recv_isr			; ptr to set rcv ISR routine
PUBLIC	count_in_err			; ptr to count rcv error routine
PUBLIC	count_out_err			; ptr to count xmit error routine
PUBLIC	maskint				; ptr to mask 8259 int mask reg routine
PUBLIC	unmaskint			; ptr to unmask 8259 intmask reg routine
PUBLIC	recv_find			; ptr to process rcv pkt with user
					; options and infom appl routine
PUBLIC	recv_copy			; ptr to copy data from drv to appl
					; buffer routine
PUBLIC	send_queue			; ptr to queue an io cntl blk routine
PUBLIC	send_dequeue			; ptr to dequeue an io cntl blk routine
;
;----------------------------------------
; ORG positions for DOS environment and variables & driver starting point
;----------------------------------------
;
	ORG	2ch			;
phd_environ	dw	?		; DOS environ var addr byte


	ORG	80h			;
phd_dioa	LABEL	byte		; DOS cmd tail & def disk buffer bytes


	ORG	100h			; Device Driver starting point
;
;----------------------------------------
; we use our dioa for a stack space.  Very hard usage has shown that only
;  27 bytes were being used, so 128 should be sufficient.
;----------------------------------------
;
our_stack	LABEL	byte		; stack top
start:
	jmp	start_1			; jump to initialize routines

EVEN					; put data area on a word boundary.
;our_stack	LABEL	byte		; WRONG !!! move up
;
;----------------------------------------
; PCnet-ISA card and system information data area
;----------------------------------------
;
packet_int_no	db	?,?,?,?		; interrupt to communicate.

is_at		db	0		; AT = 1, otherwise = 0

sys_features	db	0		; 2h = MC, 40h = 2nd 8259

flagbyte	db	0		; general purpose flag
					; CALLED_ETOPEN (init. card)
					; D_OPTION (delay init. not implement)
					; N_OPTION (802.3->8137 or 8137->8138)
					; W_OPTION (window option)

EVEN
;
;----------------------------------------
; function dispatch table
;----------------------------------------
;
functions	LABEL	word
		dw	f_not_implmt	;  0 (not supported)
		dw	f_driver_info	;  1
		dw	f_access_type	;  2
		dw	f_release_type	;  3
		dw	f_send_pkt	;  4
		dw	f_terminate	;  5
		dw	f_get_address	;  6
		dw	f_rst_interface	;  7 (not completed) ???
		dw	f_stop		;  8 
		dw	f_not_implmt	;  9 (not supported)
		dw	f_get_parameters; 10
		dw	f_not_implmt	; 11 (not supported)
		dw	f_as_send_pkt	; 12 (not completed)
		dw	f_drop_pkt	; 13 (not completed)
		dw	f_not_implmt	; 14 (not supported)
		dw	f_not_implmt	; 15 (not supported)
		dw	f_not_implmt	; 16 (not supported)
		dw	f_not_implmt	; 17 (not supported)
		dw	f_not_implmt	; 18 (not supported)
		dw	f_not_implmt	; 19 (not supported)
		dw	f_set_rcv_mode	; 20
		dw	f_get_rcv_mode	; 21
		dw	f_set_mltcst_lst; 22
		dw	f_get_mltcst_lst; 23
		dw	f_get_statistics; 24
		dw	f_set_address	; 25

;
;----------------------------------------
; handle table area
;----------------------------------------
;
handles		per_handle MAX_HANDLE dup(<>); maximun handles of packet drv
end_handles	LABEL	byte		; end of handle data area

;
;----------------------------------------
; packet driver data area
;----------------------------------------
;
multicast_count	dw	0		; count of stored multicast addresses.
multicast_broad	db	0ffh,0ffh,0ffh,0ffh,0ffh,0ffh; entry for broadcast
multicast_addrs	db	MAX_MULTICAST*EADDR_LEN dup(?);


have_my_address	db	0		; nonzero if our address has been set.
my_address	db	MAX_ADDR_LEN dup(?);
my_address_len	dw	?		;

rcv_mode_num	dw	3		; default receive mode number

free_handle	dw	0		; temp, a handle not in use
found_handle	dw	0		; temp, handle for our packet

receive_ptr	dd	0		; the pkt rcv service routine address


send_head	dd	0		; head of transmit queue
send_tail	dd	0		; tail of transmit queue

;
;----------------------------------------
; packet driver statistic data area
;----------------------------------------
;
statistics_list	LABEL	dword		;

packets_in	dw	?,?		; # of packet in across all handles
packets_out	dw	?,?		; # of packet out across all handles
bytes_in	dw	?,?		; # of bytes in, include MAC header
bytes_out	dw	?,?		; # of bytes out, include MAC header
errors_in	dw	?,?		; # of errors accross all error types
errors_out	dw	?,?		; # of errors accross all error types
packets_dropped	dw	?,?		; dropped due to no type handler.

;
;----------------------------------------
; stack & ISR data area
;----------------------------------------
;
savess		dw	?		; SS: saved during the stack swap.
savesp		dw	?		; SP: saved during the stack swap.

their_isr	dd	0		; original owner of pkt driver int

;
;----------------------------------------
;
;----------------------------------------
;
our_isr:				;
	jmp	our_isr_0		; the required signature.
		db	'PKT DRVR',0	; PKT ISR identifier(ASCII string).
our_isr_0:
	assume	ds:nothing		; assembler directive
	push	ax			; save registers
	push	bx			;
	push	cx			;
	push	dx			;
	push	si			;
	push	di			;
	push	bp			;
	push	ds			;
	push	es			;
	cld				; set string operation direction
	mov	bx,cs			; set up ds.
	mov	ds,bx			; DS = CS
	assume	ds:code			; assembler directive
	mov	bp,sp			; use bp to access the original regs.
	and	_F[bp],not CY		; clear the carry flag.

	test	flagbyte,CALLED_ETOPEN	; the card initialized?
	jnz	our_isr_cont		; yes, jump
	push	ax			; no, save registers
	push	bx			;
	push	cx			;
	push	dx			;
	push	si			;
	push	di			;
	push	bp			;
	push	ds			;
	push	es			;
					;
	call	etopen			; initialize the PCnet-ISA card

	pop	es			; restore registers
	pop	ds			;
	pop	bp			;
	pop	di			;
	pop	si			;
	pop	dx			;
	pop	cx			;
	pop	bx			;
	pop	ax			;
	mov	dh,CANT_RESET		; assume initializztion error
	jc	our_isr_error		; jump, if initialization error.
	or	flagbyte,CALLED_ETOPEN	; set PCnet-ISA init. in flagbyte
our_isr_cont:
	mov	bl,ah			; BL = correct function number.
	mov	bh,0			; BH = 0
	cmp	bx,25			; check current maximun function #
	mov	dh,BAD_COMMAND		; assume a bad function number.
	ja	our_isr_error		; jump, if function number error
	add	bx,bx			; adjust to word size
	call	functions[bx]		; index to execute specific function
	jnc	our_isr_return		; jump, if no error occur
our_isr_error:				; initialzation error
	mov	_DH[bp],dh		; save init. error in stack
	or	_F[bp],CY		; stack carry flag set indicate error
our_isr_return:
	pop	es			; restore registers
	pop	ds			;
	pop	bp			;
	pop	di			;
	pop	si			;
	pop	dx			;
	pop	cx			;
	pop	bx			;
	pop	ax			;
	iret				; return to original process

;
;----------------------------------------
; Possibly re-enable interrupts.  We put this here so that other routines
; don't need to know how we put things on the stack.
;----------------------------------------
;
re_enable_interrupts:
	test	_F[bp], EI		; chk int. enabled bit on drv entry.
	je	re_enable_interrupts_1	; jump, if not.
	sti				; Yes, re-enable interrupts now.
re_enable_interrupts_1:			;
	ret


f_not_implmt:				; function # 0,9,14-19
	mov	dh,BAD_COMMAND		; set return value = bad function 
	stc				; set carry flag indicate error occur
	ret				;


f_driver_info:				; function # 1
;
;----------------------------------------
;
; driver_info()
;
; driver_info(handle)	AH == 1, AL == 255
; 	int handle;	BX /* Optional */
;
; error return:
;	carry flag set
;	error code	DH
;	possible errors:BAD_HANDLE /* older drivers only */
;
; non-error return:
;	carry flag clear
;	version         BX
;	class		CH
;	type		DX
;	number		CL
;	name		DS:SI
;	functionality	AL ==   1, basic functions present.
;			   ==   2, basic and extended present.
;			   ==   5, basic and high-performance.
;			   ==   6, basic, high-performance, extended.
;			   == 255, not installed.
;
;----------------------------------------
;
					; As of 1.08, the handle is optional,
;	call	verify_handle		; so we no longer verify it.
	cmp	_AL[bp],0ffh		; check correct calling convention
	jne	f_driver_info_1		; jump, if fail(incorrect)
					; For enhanced PD, if they call
					; with a handle, give them the
					; class they think it is
	cmp	_BX[bp],offset handles	; check with first handle in group
	jb	default_handle		; jump default, if less 
	cmp	_BX[bp],offset end_handles;check with last handle in group
	jae	default_handle		; jump default, if greater
	mov	bx, _BX[bp]		; BX = input handle
	cmp	[bx].in_use,0		; check in use flag
	je	default_handle		; jump default, if not in use
	mov	al, [bx].class		; AL = input handle class
	mov	_CH[bp], al		; return CH = handle class
	jmp	short got_handle	;

default_handle:				; default handle class
	mov	al,driver_class		; AL = BLUEBOOK driver class
	mov	_CH[bp],al		; return class = BLUEBOOK
got_handle:				;
	mov	_BX[bp],majver		; return version = major version #
	mov	al,driver_type		; default = 99 (unused)
	cbw				; convert value in AL to AX
	mov	_DX[bp],ax		; return type = unused
	mov	_CL[bp],0		; return number = zero.
	mov	_DS[bp],ds		; return DS:SI point to our name
	mov	_SI[bp],offset driver_name; return name = NE2100
	mov	al,driver_function	; AL = 2(basic & extended)
	mov	_AL[bp],al		; return function = 2(basic & extended)
	clc				; clear acrry flag, indicate o.k.
	ret				;
f_driver_info_1:
	stc				; set carry flag, indicate error.
	ret				;

;
;----------------------------------------
; set_rcv_mode() (extended driver function)
;
; Sets  the  receive  mode  on  the  interface  associated with  handle.
; 
; set_rcv_mode(handle, mode)	AH == 20
;	int handle;         	BX
;	int mode;		CX
;
;	mode	  meaning
;
;	1	  turn off receiver
;	2	  receive only packets sent to this interface
;	3	  mode 2 plus broadcast packets
;	4	  mode 3 plus limited multicast packets
;	5	  mode 3 plus all multicast packets
;	6	  all packets
;
; error return:
;	carry flag set
;	error code		DH
;	possible errors:
;		BAD_HANDLE
;		BAD_MODE
;
; non-error return:
;	carry flag clear
;
;----------------------------------------
;

f_set_rcv_mode:				; function # 20
	call	verify_handle		; verify input handle, continue if ok.
					; otherwise, return to caller through
					; verify_handle with err code, C flag
	cmp	cx,rcv_mode_num		; check CX=input mode vs. current mode
	je	f_set_rcv_mode_4	; jump, if equal & don't check others.

	mov	dx,bx			; DX = input handle.
	mov	bx,offset handles	; check all other handles are free
f_set_rcv_mode_2:			; from 1st to last handles
	cmp	bx,dx			; check current equal input handle
	je	f_set_rcv_mode_3	; jmp, if yes, it's not free.
	cmp	[bx].in_use,0		; check current handle free?
	jne	f_set_rcv_mode_1	; jump, if no, can't change
f_set_rcv_mode_3:			; 
	add	bx,(size per_handle)	; index to next handle
	cmp	bx,offset end_handles	; check last handles
	jb	f_set_rcv_mode_2	; loop, if no, continue examination
					; if all other handles are free
	mov	cx,_CX[bp]		; CX = input handle's receive mode.
	cmp	cx,rcv_modes		; check maximun + 1 modes(from spec.)
	jae	f_set_rcv_mode_1	; jump, if out of range, a bad mode.
	mov	bx,cx			; BX = input handle's receive mode
	add	bx,bx			; BX = word index to rcv_modes table.
	mov	ax,rcv_modes[bx]+2	; AX = the mode for this handler
	or	ax,ax			; check the mdoe exist
	je	f_set_rcv_mode_1	; jump, if no, must be a bad mode.
	mov	rcv_mode_num,cx		; yes, save the mode number and
	call	ax			; adjust the handle mode accordingly
f_set_rcv_mode_4:
	clc				; return clear carry flag indicate o.k.
	ret				; return to caller
f_set_rcv_mode_1:
	mov	dh,BAD_MODE		; return DH = BAD_MODE error code
	stc				; return set carry flag indicate error 
	ret				; return to caller 

;
;----------------------------------------
;
; get_rcv_mode() (extended driver function)
;
; get_rcv_mode(handle, mode)	AH == 21
; 	int handle;         	BX
;
; error return:
;	carry flag set
;	error code		DH
;	possible errors:
;		BAD_HANDLE
;
; non-error return:
;	carry flag clear
;	mode			AX
;
;----------------------------------------
;
f_get_rcv_mode:				; function # 21
	call	verify_handle		; verify input handle, continue if ok.
					; otherwise, return to caller through
					; verify_handle with err code, C flag
	mov	ax,rcv_mode_num		; AX = the current receive mode.
	mov	_AX[bp],ax		; return AX = the current rcv mode
	clc				; return clear carry flag indicate o.k.
	ret				; return to caller

;
;----------------------------------------
; set_multicast_list() (extended driver function)
;
; set_multicast_list(addrlst, len)	AH == 22
;	char far *addrlst;		ES:DI
;	int	len;			CX
;
;error return:
;	carry flag set
;	error code			DH
;	possible errors:
;		NO_MULTICAST
;		NO_SPACE
;		BAD_ADDRESS
;
;non-error return:
;	carry flag clear
;
;----------------------------------------
;
f_set_mltcst_lst:			; function # 22
	mov	cx,_CX[bp]		; CX = multicast addr list length
;
;----------------------------------------
;verify that they supplied an integer number of EADDR's.
;----------------------------------------
;
	mov	ax,cx			; AX = multicast addr list length
	xor	dx,dx			; clear DX
	mov	bx,EADDR_LEN		; BX = ethernet address length
	div	bx			; DX:AX/BX -> AX = quo , DX = rem
	or	dx,dx			; check DX = 0
	jne	f_set_mltcst_lst_2	; jmp, if no integer number of addr

	cmp	ax,MAX_MULTICAST	; check maximun multicast address
	ja	f_set_mltcst_lst_3	; jump, if greater, return NO_SPACE
f_set_mltcst_lst_1:
	mov	multicast_count,ax	; save the multicast addr number
	push	cs			; save code segment
	pop	es			; ES = CS
	mov	di,offset multicast_addrs; DI = point to multicast addr start
	push	ds			; save data segment
					; get ds:si -> new list.
	mov	ds,_ES[bp]		; DS = input ES
	mov	si,_DI[bp]		; SI = input DI
	push	cx			; save multicast addr list length
;	cld				; set direction flag
	rep	movsb			; load input multicast addr list
	pop	cx			; restore multicast addr list length
	pop	ds			; restore data segment

	mov	si,offset multicast_addrs; DI = pt to new multicast addr start
	call	set_multicast_list	; 
	ret				; return to caller
f_set_mltcst_lst_2:
	mov	dh,BAD_ADDRESS		; return DH = BAD_ADDRESS error code
	stc				; return set carry flag indicate error
	ret				; return to caller
f_set_mltcst_lst_3:
	mov	dh,NO_SPACE		; return DH = NO_SPACE error code
	stc				; return set carry flag indicate error
	ret				; return to caller
;
;----------------------------------------
; get_multicast_list() (extended driver function)
;
; get_multicast_list()		AH == 23
;
;error return:
;	carry flag set
;	error code		DH
;	possible errors:
;		NO_MULTICAST
;		NO_SPACE
;
;non-error return:
;	carry flag clear
;	char far *addrlst;	ES:DI
;	int	len;		CX
;
;----------------------------------------
;
f_get_mltcst_lst:			; function # 23
	mov	_ES[bp],ds		; return ES = DS
	mov	_DI[bp],offset multicast_addrs; return DI = pt to multicast addr start
	mov	ax,EADDR_LEN		; AX = ethernet address length
	mul	multicast_count		; multiple with multicast count
	mov	_CX[bp],ax		; return CX = multicast addr list len
	clc				; return clear carry flag indicate ok.
	ret				; return to caller

;
;----------------------------------------
; get_statistics() (extended driver function)
;
; get_statistics(handle)	AH == 24
;	int handle;		BX
;
; error return:
;	carry flag set
;	error code		DH
;	possible errors:
;		BAD_HANDLE
;
; non-error return:
;	carry flag clear
;	char far *stats;	DS:SI
;
;    struct statistics {
;	unsigned long	packets_in;	/* Totals across all handles */
;	unsigned long	packets_out;
;	unsigned long	bytes_in;	/* Including MAC headers */
;	unsigned long	bytes_out;
;	unsigned long	errors_in;	/* Totals across all error types */
;	unsigned long	errors_out;
;	unsigned long	packets_lost;	/* No buffer from receiver(), card */
;					/*  out of resources, etc. */
;    };
;
;----------------------------------------
;
f_get_statistics:			; function # 24
	call	verify_handle		; verify input handle, continue if ok.
					; otherwise, return to caller through
					; verify_handle with err code, C flag
	mov	_DS[bp],ds		; return DS = DS
	mov	_SI[bp],offset statistics_list; return SI = pt to statistics list
	clc				; return clear carry flag indicate ok.
	ret				; return to caller


access_type_class:
	mov	dh,NO_CLASS		; return DH = NO_CLASS
	stc				; return set carry flag indicate error
	ret				; return to caller

access_type_type:
	mov	dh,NO_TYPE		; return DH = NO_TYPE
	stc				; return set carry flag indicate error
	ret				; return to caller

access_type_number:
	mov	dh,NO_NUMBER		; return DH = NO_NUMBER
	stc				; return set carry flag indicate error
	ret				; return to caller

access_type_bad:
	mov	dh,BAD_TYPE		; return DH = BAD_TYPE
	stc				; return set carry flag indicate error
	ret				; return to caller

;
;----------------------------------------
; access_type()
;
; int access_type(if_class, if_type, 
;		  if_number, type, 
;		  typelen, receiver)	AH == 2
;
;	int	if_class;		AL
;	int	if_type;		BX
;	int	if_number;		DL
;	char far *type;                 DS:SI
;	unsigned typelen;		CX
;	int	(far *receiver)();	ES:DI
;
; error return:
;	carry flag set
;	error code			DH
;	possible errors:
;		NO_CLASS
;		NO_TYPE
;		NO_NUMBER
;		BAD_TYPE
;		NO_SPACE
;		TYPE_INUSE
;
; non-error return:
;	carry flag clear
;	handle				AX
;
; receiver call:
;	(*receiver)(handle, flag, len [, buffer])
;	int	handle;         	BX
;	int	flag;			AX
;	unsigned len;			CX
;
;	if AX == 1,
;		char far *buffer;	DS:SI
;
;----------------------------------------
;
f_access_type:				; function # 2
	mov	bx, offset driver_class	; BX = driver class bytes 1st address
access_type_9:
	mov	al, [bx]		; AL = class supported
	inc	bx			; BX = next class byte address
	or	al,al			; check end of the list
	je	access_type_class	; jump, if yes, end of list
	cmp	_AL[bp],al		; check input class = list class
	jne	access_type_9		; loop, if not the same
access_type_1:				; class supported
	cmp	_BX[bp],-1		; check BX = 0xFFFF(generic type)
	je	access_type_2		; jump, if yes.
	mov	al,driver_type		; AL = driver type(99/unused pkt drv)
	cbw				; convert AL to AX
	cmp	_BX[bp],ax		; check input type = driver type
	jne	access_type_type	; jump, if not, error
access_type_2:				; class, type supported
	cmp	_DL[bp],0		; input DL = 0(generic number)
	je	access_type_3		; jump, if yes
	cmp	_DL[bp],1		; check input DL = drive type number
	jne	access_type_number	; jump, if not, error
access_type_3:				; 1st interface of class/type
	cmp	_CX[bp],MAX_P_LEN	; check type length with maxi. type len 
	ja	access_type_bad		; jump, if greater, error
;	cld				; set direction flag
;
;----------------------------------------
; 1. look for an open handle
; 2. check the existing handles to see if they're replicating a packet type.
;----------------------------------------
;
	mov	free_handle,0		; set free handle = 0
	mov	bx,offset handles	; BX = address to 1st handle
access_type_4:
	cmp	[bx].in_use,0		; check current handle in use
	je	access_type_5		; jump, if no, don't check the type.
	mov	al, _AL[bp]		; AL = input class
	cmp	al, [bx].class		; check input class vs. current handle
	jne	short access_type_6	; jump, if not equal
					; get a pointer to their type
					; from their ds:si to our es:di
	mov	es,_DS[bp]		; ES = input DS
	mov	di,_SI[bp]		; DI = input SI 
;
;----------------------------------------
; get the minimum of their length and our length.
; As currentlyimplemented, only one receiver gets the packets,
; so we have toensure that the shortest prefix is unique.
;----------------------------------------
;
	mov	cx,_CX[bp]		; CX = input type length
	cmp	cx,[bx].packet_type_len	; check current handle's type length
					; vs. input type length
	jb	access_type_8		; jump, if input type length less
	mov	cx,[bx].packet_type_len	; CX = current handle's type length
access_type_8:
	lea	si,[bx].packet_type	; DS:SI pt to current handle pkt type
	or	cx,cx			; check pass-all TYPE(zero TYPE len)
	jne	access_type_7		; jump, if no
					; BX = start address last handle
	mov	bx,offset handles+(MAX_HANDLE-1)*(size per_handle);
	jmp	short access_type_5	; put pass-all type at last handle
access_type_7:
	repe	cmpsb			; compare type length
	jne	short access_type_6	; jump, if not equal, go look next one.
access_type_inuse:			; a handle has been assigned for TYPE
					; and we can't assign another
	mov	dh,TYPE_INUSE		; DH = TYPE_INUSE(same handle/type)
	stc				; return set carry flag indicate error
	ret				; return to caller
access_type_5:				; handle(first avail./last) not in use
	cmp	free_handle,0		; check a free handle exist
	jne	access_type_6		; jump, if yes.
	mov	free_handle,bx		; free handle = not in use handle
access_type_6:
	add	bx,(size per_handle)	; go to the next handle.
	cmp	bx,offset end_handles	; check end of last handles
	jb	access_type_4		; loop, if not

	mov	bx,free_handle		; BX = free handle
	or	bx,bx			; check free handle != 0
	je	access_type_space	; jump, if free handle zero, error

	mov	[bx].in_use,1		; set free(current) handle in use.

	mov	ax,_DI[bp]		; AX = input receiver type offset.
	mov	[bx].receiver.offs,ax	; set free handle receive type offset
	mov	ax,_ES[bp]		; AX = input receiver type segment.
	mov	[bx].receiver.segm,ax	; set free handle receive type segment

	push	ds			; save DS segment
	mov	ax,ds			; AX = DS
	mov	es,ax			; ES = DS
	mov	ds,_DS[bp]		; DS = input type segment
	mov	si,_SI[bp]		; SI = input type offset
	mov	cx,_CX[bp]		; CX = input type length
	mov	es:[bx].packet_type_len,cx; handle type len = input type len
	lea	di,[bx].packet_type	; ES:DI = point to handle type 
	rep	movsb			; copy input type into handle type
					; copy the first 8 bytes
					; to the receiver signature.
	lds	si,es:[bx].receiver	; DS:SI = input ES:DI
	lea	di,[bx].receiver_sig	; ES:DI = point to handle rcv signature
	mov	cx,8/2			; CX = 4 words
	rep	movsw			; copy first 4 words

	pop	ds			; restore DS segment

	mov	al, _AL[bp]		; AL = input class
	mov	[bx].class, al		; handle class = input class

	mov	_AX[bp],bx		; return the handle to them.

	clc				; return clear carry flag indicate ok.
	ret				; return to caller


access_type_space:
	mov	dh,NO_SPACE		; return DH = NO_SPACE
	stc				; return set carry flag indicate error
	ret				; return to caller
;
;----------------------------------------
; release_type()
;
; int release_type(handle)	AH == 3
;	int	handle;         BX
;
; error return:
;	carry flag set
;	error code		DH
;	possible errors:
;		BAD_HANDLE
;
; non-error return:
;	carry flag clear
;
;----------------------------------------
;
f_release_type:				; function # 3
	call	verify_handle		; verify input handle, continue if ok.
					; otherwise, return to caller through
					; verify_handle with err code, C flag
					; BX = input BX after verify handle
	mov	[bx].in_use,0		; mark this handle as being unused.
	clc				; return clear carry flag indicate ok.
	ret				; return to caller


;
;----------------------------------------
; send_pkt()
;
; int send_pkt(buffer, length)	AH == 4
;	char far *buffer;	DS:SI
;	unsigned length;	CX
;
; error return:
;	carry flag set
;	error code		DH
;	possible errors:
;		CANT_SEND
;
; non-error return:
;	carry flag clear
;
;----------------------------------------
;
f_send_pkt:				; function # 4
;
;----------------------------------------
; ds:si -> buffer, cx = length
; XXX Should re-enable interrupts here, but some drivers are broken.
; Possibly re-enable interrupts.
;----------------------------------------
;
;	test 	_F[bp], EI		; check interrupts enabled on pkt drv entry?
;	je	f_send_pkt_1		; jump, if not.
;	sti				; Yes, re-enable interrupts now.
;f_send_pkt_1:				;

;
;----------------------------------------
;following two instructions not needed because si and cx haven't been changed.
;----------------------------------------
;					; uncomment these two lines ?????
;	mov	si,_SI[bp]		; SI = input buffer offset
;	mov	cx,_CX[bp]		; CX = input packet bytes.
;
	linc	packets_out		; increment packet out statistic
	add	bytes_out.offs,cx	; add up the transmit bytes statistic
	adc	bytes_out.segm,0	; in double word format

	push	ds			; save DS segment
	mov	ds,_DS[bp]		; DS = input DS of caller's buffer
	assume	ds:nothing, es:nothing	;

;
;----------------------------------------
; If -n option take Ethernet encapsulated Novell IPX packets
; (from BYU's PDSHELL) and change them to be IEEE 802.3 encapsulated.
;
; ITEM			LENGTH
; ------------------------------------
; Preamble/SFD 		8 bytes
; Destination address 	6 bytes
; Source address 	6 bytes
; Length		2 bytes(on ETHERNET_II, Type x8137 bytes for Netware)
; Data			4 - 1500 bytes
;	IPX header
;	Checksum (FF,FF)		2 bytes
;	Length				2 bytes(including IPX header & data)
;	Transport control		1 byte
;	Packet type			1 byte
;	Destination Node addr		6 bytes
;	Destination Network addr	4 bytes
;	Destination Socket		2 bytes
;	Source Node addr		6 bytes
;	Source Network addr		4 bytes
;	Source Socket			2 bytes
;	....
;
; FCS			4 bytes
;
;----------------------------------------
;
EPROT_OFF	equ	EADDR_LEN*2	; source & destination address len
					; Bluebook 16bit ethertype at offs 12
					; IEEE8023 type at offs 14
	test	cs:flagbyte,N_OPTION	; check user "-n" option
	jz	f_send_pkt_2		; jump, if not selected
	cmp	ds:[si].EPROT_OFF,3781h ; check byte 12,13 = Novell(prot 8137)
	jne	f_send_pkt_2		; jump, if not equal
	push	ax			; save scratch reg
	mov	ax,ds:[si].EPROT_OFF+4	; AX = IPX header's len(even/odd bytes)
	xchg	ah,al			; AX = (high,low)
	inc	ax			; AL = rounding up
	and	al,0feh			; AL = even byte number
	xchg	ah,al			; AX = (low,high)
	mov	ds:[si].EPROT_OFF,ax	; IEEE8023 length = IPX length
	pop	ax			; restore old contents
f_send_pkt_2:
	call	send_pkt		; send packet through PCnet-ISA 
	pop	ds			; resume DS segment
	assume	ds:code			;
	ret				; return to caller


;
;----------------------------------------
; as_send_pkt() (high-performance driver function, ie. function 5 & 6 only)
;
; int as_send_pkt(buffer,length,upcall) AH == 11
; 	char far *buffer;		DS:SI
; 	unsigned length;		CX
; 	int	(far *upcall)();	ES:DI
;
;error return:
;	carry flag set
;	error code			DH
;	possible errors:
;		CANT_SEND		/* transmit error, re-entered, etc. */
;		BAD_COMMAND		/* Level 0 or 1 driver */
;
;non-error return:
;	carry flag clear
;
;buffer available upcall:
;	(*upcall)(buffer, result)
;		int	result;         AX	/* 0 for copy ok */
;		char far *buffer;	ES:DI	/* from as_send_pkt() call */
;
;----------------------------------------
;
f_as_send_pkt:				; function # 12 (NOT SUPPORTED)
;
;----------------------------------------
; es:di -> iocb.
;----------------------------------------
;
;	test	driver_function,4	; check high-performance drv ?????
	test	driver_function,4	; check high-performance drv = 5 or 6
	je	f_as_send_pkt_2		; jump, if not.
;
;----------------------------------------
; Possibly re-enable interrupts.
;----------------------------------------
;
	test _F[bp], EI			; check pkt drv int enabled 
	je	f_as_send_pkt_1		; jump, if disabled.
	sti				; Yes, re-enable interrupts now.
f_as_send_pkt_1:
;
;----------------------------------------
; MISSING LOTS OF CODE HERE
; for example: 
; 1. DS:SI from input stack
; 2. CX from input stack
; 3. ES:DI from input stack
; 4. buffer available upcall
;----------------------------------------
;
	push	ds			; save DS segment
	lds	si,es:[di].buffer	; DS:SI = point to buffer
	assume	ds:nothing		;
	mov	cx,es:[di].len		; CX = length
	linc	packets_out		; increment packet out statistics
	add	bytes_out.offs,cx	; increment statistic of xmit bytes.
	adc	bytes_out.segm,0	; in double word format
;
;----------------------------------------
; ds:si -> buffer, cx = length, es:di -> iocb.
;----------------------------------------
;					; asynchronize send pkt
	call	as_send_pkt		; (not supported)
	pop	ds			; restore DS segment
	assume	ds:code			;
	ret				; return to caller
f_as_send_pkt_2:
	mov dh,	BAD_COMMAND		; return DH = BAD_COMMAND
	stc				; return set carry flag indicate error
	ret				; return to caller


;
;----------------------------------------
; DEBUG : Any spec for this ????
;----------------------------------------
;
f_drop_pkt:				; function # 13	(NOT SUPPORTED)
;
;----------------------------------------
; es:di -> iocb.
;----------------------------------------
;
;	test	driver_function,4	; check high-performance drv ?????
	test	driver_function,4	; check high-performance drv = 5 or 6
	je	f_as_send_pkt_1		; jump, if no high-performance drive

	push	ds			; save DS segment
	mov	si,offset send_head	; SI = offset of xmit queue head addr
dp_loop:
	mov	ax,ds:[si]		; AX = xmit queue head offset
	mov	dx,ds:[si+2]		; DX = xmit queue head segment
	mov	bx,ax			; BX = xmit queue head offset
					; what is next inst try to say 
					; ???????
	or	bx,dx			; End of list?
	je	dp_endlist		; jump, if yes
	cmp	ax,di			; check xmit queue head offs=iocb offs 
	jne	dp_getnext		; jump, if not equal
	mov	bx,es			; BX = iocb segment
	cmp	dx,bx			; check xmit queue head segm=iocb segm
	jne	dp_getnext		; jump, if not equal
	call	drop_pkt		; drop the pkt through card driver
	les	di,es:[di].next		; ES:DI =  next segment:offset
	mov	ds:[si],di		; xmit queue send head = next offset
	mov	ds:[si+2],es		; xmit queue send head = next segment
	pop	ds			; restore DS segment
	clc				; return clear carry flag indicate ok
	ret				; return to caller
dp_getnext:
	mov	ds,dx			; DS = xmit queue head segment
	mov	si,ax			; SI = xmit queue head offset
	lea	si,ds:[si].next		; SI = next iocb offset
	jmp	dp_loop			; Try again
dp_endlist:
	pop	ds			; restore DS segment
	mov	dh,BAD_IOCB		; return DH = BAD_IOCB
	stc				; return set carry flag indicate error
	ret				; return to caller

;
;----------------------------------------
;terminate()
;
;terminate(handle)		AH == 5
;	int	handle;         BX
;
; error return:
;	carry flag set
;	error code		DH
;	possible errors:
;		BAD_HANDLE
;		CANT_TERMINATE
;
; non-error return:
;	carry flag clear
;
;----------------------------------------
;

f_terminate:				; function # 5
	call	verify_handle		; verify input handle, continue if ok.
					; otherwise, return to caller through
					; verify_handle with err code, C flag
f_terminate_1:				; mark handle as free
	mov	[bx].in_use,0		; clear handle in use byte
					; check that all handles are free
	mov	bx,offset handles	; BX = address of first handle
f_terminate_2:				; 
	cmp	[bx].in_use,0		; check current handle free
	jne	f_terminate_4		; jump, if not, so can't exit completely
	add	bx,(size per_handle)	; BX = address of next handle
	cmp	bx,offset end_handles	; check current addr=end of last handle
	jb	f_terminate_2		; loop, if not, continue examination
;
;----------------------------------------
; Now disable interrupts
;----------------------------------------
;
	mov	al,int_no		; AL = 1st IRQ byte
	or	al,al			; check IRQ existence
	je	f_terminate_no_irq	; jump, if not existence
	call	maskint			; mask IRQ through 8259 mask reg
;
;----------------------------------------
; Now return the interrupt to their handler.
;----------------------------------------
;
	mov	ah,25h			; AH = DOS int 21H:25H
	mov	al,int_no		; AL = 1st IRQ value
	add	al,8			; AL = 1st IRQ byte map to master
					; 8259 DOS int #
					; check 1st IRQ byte with master
	cmp	al,8+8			; 8259 DOS int #
	jb	f_terminate_3		; jump, if less, master 8259 IRQ
	add	al,70h - (8+8)		; slave 8259 IRQ map to DOS int #.
f_terminate_3:
	push	ds			; save DS segment
	lds	dx,their_recv_isr	; DS:DX = original receive ISR
	int	21h			; restore original receive ISR vector
	pop	ds			; restore DS segment

f_terminate_no_irq:
	call	terminate		; terminate the hardware.

	mov	al,packet_int_no	; release our_isr.
	mov	ah,25h			; AH = DOS int 21H:25H
	push	ds			; save DS segment
	lds	dx,their_isr		; DS:DX = original pkt ISR
	int	21h			; restore original pkt ISR vector
	pop	ds			; restore DS segment
;
;----------------------------------------
; Now free our memory
;----------------------------------------
;
	push	cs			; save CS on stack
	pop	es			; ES = CS
	mov	ah,49h			; AH = DOS int 21H:49H
	int	21h			; release memory block ES
	clc				; return clear carry flag indicate ok.
	ret				; return to caller
f_terminate_4:				; not all handles are free(some in use)
	mov	dh, CANT_TERMINATE	; return DH = CANT_TERMINATE
	stc				; return set carry flag indicate error
	ret				; return to caller

;
;----------------------------------------
; get_address()
;
; get_address(handle, buf, len)	AH == 6
; 	int	handle;         BX
; 	char far *buf;		ES:DI
; 	int	len;		CX
;
; error return:
;	carry flag set
;	error code		DH
;	possible errors:
;		BAD_HANDLE
;		NO_SPACE
;
; non-error return:
;	carry flag clear
;	length			CX
;
;----------------------------------------
;
f_get_address:				; function # 6
					; uncomment next three lines ?????
;	call	verify_handle		; verify input handle, continue if ok.
					; otherwise, return to caller through
					; verify_handle with err code, C flag
;	mov	es,_ES[bp]		; ES = input segment ES of buffer
;	mov	di,_DI[bp]		; DI = input offset DI of buffer
	mov	cx,_CX[bp]		; CX = input length CX
	cmp	have_my_address,0	; check have_my_address = clear
	jne	get_address_set		; jump, if set
	call	get_address		; try to get the address
	jc	get_address_space	; jump, if carry set(not enough space)
	mov	_CX[bp],cx		; return length
	clc				; return clear carry flag indicate ok.
	ret				; return to caller
get_address_set:
	cmp	cx,my_address_len	; check input len with current addr len
	jb	get_address_space	; jump, if input len smaller
	mov	cx,my_address_len	; CX = current address length.
	mov	_CX[bp],cx		; return CX with current address length
	mov	si,offset my_address	; SI = address of current net address
	rep	movsb			; copy it into input buffer.
	clc				; return clear carry flag indicate ok.
	ret				; return to caller

get_address_space:
	mov	dh,NO_SPACE		; return DH = NO_SPACE
	stc				; return set carry flag indicate error
	ret				; return to caller


;
;----------------------------------------
; set_address()
;
; extended driver function
; set_address(addr, len)	AH == 25
; 	char far *addr;         ES:DI
; 	int len;		CX
;
; error return:
;	carry flag set
;	error code		DH
;	possible errors:
;		CANT_SET
;		BAD_ADDRESS
;
; non-error return:
;	carry flag clear
;	length			CX
;
;----------------------------------------
;
f_set_address:				; function # 25
	mov	bx,offset handles	; BX = address of 1st handle
	mov	cl,0			; CL = number of handles in use = 0
f_set_address_1:			;
	add	cl,[bx].in_use		; CL = number of handles in use
	add	bx,(size per_handle)	; BX = address of next handle
	cmp	bx,offset end_handles	; check BX = end addr of last handle
	jb	f_set_address_1		; loop, if not reach last handle end
					;
	cmp	cl,1			; check number of handle in use > 1
					; should be greater or equal ?????
	ja	f_set_address_inuse	; jump, if greater than 1

	mov	ds,_ES[bp]		; DS = input address segment ES
	assume	ds:nothing		;
	mov	si,_DI[bp]		; SI = input address offser DI
	mov	cx,_CX[bp]		; CX = input address length
	call	set_address		; set lan address into PCnet-ISA
;
;----------------------------------------
; set_address restores ds.		;
;----------------------------------------
;
	jc	f_set_address_exit	; jump, if set address fail
	mov	_CX[bp],cx		; return CX = lan address length.

	cmp	cx,MAX_ADDR_LEN		; check addr len with maximun len
	ja	f_set_address_too_long	; jump, if greater than maximun len

	mov	ds,_ES[bp]		; DS = input addr segment ES
	mov	si,_DI[bp]		; SI = input addr offset DI
	mov	ax,cs			; AX = CS
	mov	es,ax			; ES = CS
	mov	my_address_len,cx	; may address len = lan addr len
	mov	di,offset my_address	; DI = address to my address
	rep	movsb			; copy input address into my address
	mov	have_my_address,1	; have_my_address=1,indicate addr set
	mov	ds,ax			; restoer DS.
	assume	ds:code			;
	clc				; return clear carry flag indicate ok.
	ret				; return to caller
f_set_address_inuse:
	mov	dh,CANT_SET		; return DH = CANT_SET
	stc				; return set carry flag indicate error
	ret				; return to caller
f_set_address_too_long:
	mov	dh,NO_SPACE		; return DH = NO_SPACE
	stc				; return set carry flag indicate error
f_set_address_exit:
	ret				; return to caller


;
;----------------------------------------
; reset_interface()
;
; reset_interface(handle)	AH == 7
;
; 	int	handle;         BX
;
; error return:
;	carry flag set
;	error code		DH
;	possible errors:
;		BAD_HANDLE
;		CANT_RESET
;
; non-error return:
;	carry flag clear
;
;----------------------------------------
;
f_rst_interface:			; function # 7
	call	verify_handle		; verify input handle, continue if ok.
					; otherwise, return to caller through
					; verify_handle with err code, C flag
	call	reset_interface		; reset interface(DO NOTHING ????)
	clc				; return clear carry flag indicate ok.
	ret				; return to caller


;
;----------------------------------------
; Stop the packet driver doing upcalls. Also a following terminate will
; always succed (no in use handles any longer).
;----------------------------------------
;
;
;----------------------------------------
;----------------------------------------
;
f_stop:					; function # 9
	mov	bx,offset handles	; BX = address of 1st handle
f_stop_2:				; clear all handle's in use
	mov	[bx].in_use,0		; clear in use of current handle
	add	bx,(size per_handle)	; BX = address of next handle
	cmp	bx,offset end_handles	; check BX reach end of last handle
	jb	f_stop_2		; loop, if not end of last handle
	clc				; return clear carry flag indicate ok.
	ret				; return to caller


;
;----------------------------------------
; get_parameters() (high-performance driver function, ie. function 5 & 6 only)
;
; get_parameters()		AH = 10
;
; error return:
;	carry flag set
;	error code		DH
;	possible errors:
;		BAD_COMMAND		/* No high-performance support */
;
; non error return:
;	carry flag clear
;	struct param far *;	ES:DI
;
;    struct param {
;	unsigned char	major_rev;	/* Revision of Packet Driver spec */
;	unsigned char	minor_rev;	/*  this driver conforms to. */
;	unsigned char	length;         /* Length of structure in bytes */
;	unsigned char	addr_len;	/* Length of a MAC-layer address */
;	unsigned short	mtu;		/* MTU, including MAC headers */
;	unsigned short	multicast_aval; /* Buffer size for multicast addr */
;	unsigned short	rcv_bufs;	/* (# of back-to-back MTU rcvs) - 1 */
;	unsigned short	xmt_bufs;	/* (# of successive xmits) - 1 */
;	unsigned short	int_num;	/* Interrupt # to hook for post-EOI
;					   processing, 0 == none */
;    };
;
;----------------------------------------
;
f_get_parameters:			; function # 10
;
;----------------------------------------
;strictly speaking, this function only works for high-performance drivers.
;----------------------------------------
;
;	test	driver_function,4	; check high-performance drv ?????
	test	driver_function,4	; check high-performance drv = 5 or 6
	jne	f_get_parameters_1	; jump, if high-performance support.
	mov	dh,BAD_COMMAND		; return DH = BAD_CMOOAND
	stc				; return set carry flag indicate error
	ret				; return to caller
f_get_parameters_1:
	mov	_ES[bp],cs		; return ES = CS current code segment
	mov	_DI[bp],offset parameter_list; return DI = address to para_lst
	clc				; return clear carry flag indicate ok.
	ret				; return to caller


verify_handle:
;
;----------------------------------------
;Ensure that their handle is real.  If it isn't, we pop off our return
;address, and return to *their* return address with cy set.
;----------------------------------------
;
	mov	bx,_BX[bp]		; get the input handle
	cmp	bx,offset handles	; compare with 1st handle in group
	jb	verify_handle_bad	; jump, if handle bad.
	cmp	bx,offset end_handles	; compare with last handle in group
	jae	verify_handle_bad	; jump, if handle bad.
	cmp	[bx].in_use,0		; check if input handle in use
	je	verify_handle_bad	; jump, if not in use
	ret				; return to caller if handle ok.
verify_handle_bad:
	mov	dh,BAD_HANDLE		; return DH = BAD_HANDLE value
	add	sp,2			; pop off caller address.
	stc				; set carry flag 
	ret				; return to caller's caller

;
;----------------------------------------
;
;----------------------------------------
;
set_recv_isr:				; get the old interrupt into es:bx
					; board's interrupt vector
	mov	ah,35h			; DOS int 21h:35h get int vector
	mov	al,int_no		; AL = 1st IRQ byte
	or	al,al			; check IRQ byte = 0
	je	set_isr_no_irq		; jump, if no IRQ set
	add	al,8			; AL = 1st IRQ byte map to master
					; 8259 DOS int #
					; check 1st IRQ byte with master
	cmp	al,8+8			; 8259 DOS int #
	jb	set_recv_isr_1		; jump, if less, master 8259 IRQ
	add	al,70h - 8 - 8		; slave 8259 IRQ map to DOS int #.
set_recv_isr_1:
	int	21h			; ES:BX = 1st IRQ's DOS ISR addr
	mov	their_recv_isr.offs,bx	; their recv ISR offs = BX
	mov	their_recv_isr.segm,es	; their recv ISR segm = ES
					; now set our recv interrupt.
	mov	ah,25h			; DOS int 21h:25H set int vector
	mov	dx,offset recv_isr	; DX = recv ISR offset
	int	21h			; 1st IRQ's DOS ISR addr = DS:DX
					; Now enable interrupts
	mov	al,int_no		; AL = 1st IRQ byte
	call	unmaskint		; unmask 8259 mask register enable int

set_isr_no_irq:
	ret				; return to caller

;
;----------------------------------------
;
;----------------------------------------
;
count_in_err:				;
	assume	ds:nothing		;
	linc	errors_in		; increment handle's receive error
	ret				; return to caller

;
;----------------------------------------
;
;----------------------------------------
;
count_out_err:				;
	assume	ds:nothing		;
	linc	errors_out		; increment handle's transmit error
	ret				; return to caller

their_recv_isr	dd	0		; original owner of board int

;
;----------------------------------------
; I have had a problem with some hardware which under extreme LAN loading
; conditions will re-enter the recv_isr. Since the 8259 interrupts for
; the card are masked off, and the card's interrupt mask register is 
; cleared (in 8390.asm at least) disabling the card from interrupting, this
; is clearly a hardware problem. Due to the low frequencey of occurance, and
; extreme conditions under which this happens, it is not likely to be fixed
; in hardware any time soon, plus retrofitting of hardware in the field will
; not happen. To protect the driver from the adverse effects of this I am
; adding a simple re-entrancy trap here.  - gft - 910617
;----------------------------------------
;

in_recv_isr	db 	0		; flag to trap re-entrancy

recv_isr:
	cmp	cs:in_recv_isr, 0	; check int enter flag
	je	no_re_enter		; jump, if not re-enter
	iret				; otherwise, re-entered exit

no_re_enter:				; not re-entered
IFDEF	SHARE_IRQ
	pushf				; save flag
	call	share_isr		; check our interrupt condition
	jnz	our_recv_isr		; jump, our interrupt
	popf				; restore flag
	jmp	cs:their_recv_isr	; others interrupt
our_recv_isr:
	popf				; restore flag
ENDIF
	mov	cs:in_recv_isr, 1	; set int enter flag
;
;----------------------------------------
; I realize this re-entrancy trap is not perfect, you could be re-entered
; anytime before the above instruction. However since the stacks have not
; been swapped re-entrancy here will only appear to be a spurious interrupt.
; - gft - 910617
;
; In order to achieve back-to-back packet transmissions, we handle the
; latency-critical portion of transmit interrupts first.  The xmit
; interrupt routine should only start the next transmission, but do
; no other work.  It may only touch ax and dx (the only register necessary
; for doing "out" instructions) unless it first pushes any other registers
; itself.
;----------------------------------------
;
	push	ax			; save AX =
	push	dx			;
	call	xmit			;
;
;----------------------------------------
; Now switch stacks, push remaining registers, and do remaining interrupt work.
;----------------------------------------
;
	push	ds
	mov	ax,cs			; ds = cs.
	mov	ds,ax			;
	assume	ds:code			;

	mov	savesp,sp		;
	mov	savess,ss		;

	mov	ss,ax			;
	mov	sp,offset our_stack	;
	cld				;

	push	bx			;
	push	cx			;
	push	si			;
	push	di			;
	push	bp			;
	push	es			;
;
;----------------------------------------
; The following comment is wrong in that we now do a specific EOI command,
; and because we don't enable interrupts (even though we should).
;
; Chips & Technologies 8259 clone chip seems to be very broken.  If you
; send it a Non Specific EOI command, it clears all In Service Register
; bits instead of just the one with the highest priority (as the Intel
; chip does and clones should do).  This bug causes our interrupt
; routine to be reentered if: 1. we reenable processor interrupts;
; 2. we reenable device interrupts; 3. a timer or other higher priority
; device interrupt now comes in; 4. the new interrupting device uses
; a Non Specific EOI; 5. our device interrupts again.  Because of
; this bug, we now completely mask our interrupts around the call
; to "recv", the real device interrupt handler.  This allows us
; to send an EOI instruction to the 8259 early, before we actually
; reenable device interrupts.  Since the interrupt is masked, we
; are still guaranteed not to get another interrupt from our device
; until the interrupt handler returns.  This has another benefit:
; we now no longer prevent other devices from interrupting while our
; interrupt handler is running.  This is especially useful if we have
; other (multiple) packet drivers trying to do low-latency transmits.
;----------------------------------------
;					; Disable further device interrupts
	mov	al,int_no		; AL = 1st IRQ byte
	call	maskint			; disable int through 8259 IRQ mask reg
;
;----------------------------------------
; The following is from Bill Rust, <wjr@ftp.com>
; this code dismisses the interrupt at the 8259. if the interrupt number
;  is > 8 then it requires fondling two PICs instead of just one.
;----------------------------------------
;
	mov	al, int_no		; AL = 1st IRQ byte
	cmp	al, 8			; check 1st IRQ with master 8259 IRQ
	jg	recv_isr_4		; jump, if not master 8259
	add	al, 60h			; AL = specific EOI dismissal
	out	20h, al			; send specific EOI to master 8259
	jmp	recv_isr_3		; jump, done
recv_isr_4:				;
	add	al,60h - 8		; AL = specific EOI (# between 9 & 15).
	out	0a0h,al			; send to slave 8259 (PC/AT only)
	mov	al,62h			; AL = acknowledge
	out	20h,al			; send to master 8259
recv_isr_3:

;	sti				; enable external interrupt flag
	call	recv			; receive 

	cli				; interrupts *must* be off between
					; here and the stack restore, because
					; if we have one of our interrupts
					; pending, we would trash our stack.
;
;----------------------------------------
; According to Novell IMSP Technical Memo #2, Lost Interrupts and NetWare,
; some LAN controllers can glitch the interrupt line when the interrupt
; mask register on the LAN controller is enabled to allow the LAN controller
; to interrupt. This can cause spurious/lost interrupt problems, especially
; on some 486 processors which latch the int line to the processor. To prevent
; glitches from being propogated throug, move the call to recv_exiting (re-
; enables interrupts on the LAN card) to before when the 8259 interrupts are
; unmasked. - gft - 910617
;----------------------------------------
;
	call	recv_exiting		;

	mov	al,int_no		; Now reenable device interrupts
	call	unmaskint		;

	pop	es			;
	pop	bp			;
	pop	di			;
	pop	si			;
	pop	cx			;
	pop	bx			;

	mov	ss,savess		;
	mov	sp,savesp		;
;
;----------------------------------------
; move the next call up to above the call unmaskint - gft - 910617
;	call	recv_exiting		;this routine can enable interrupts.
; DDP - This is a BIG mistake.  This routine SHOULD NOT enable interrupts.
;	doing so can cause interrupt recursion and blow your stack.
;	Processor interrupts SHOULD NOT be enabled after enabling device
;	interrupts until after the "iret".  You will lose atleast 12 bytes
;	on the stack for each recursion.
;----------------------------------------
;
	pop	ds			;
	assume	ds:nothing		;
	pop	dx			;
	pop	ax			;
	mov	cs:in_recv_isr, 0	; clear the re-entrancy flag - gft - 901617
	iret				;


;
;----------------------------------------
; input AL = IRQ number to mask
;
;----------------------------------------
;
maskint:				;
	or	al,al			; check IRQ number = timer IRQ
	je	maskint_1		; jump, if timer IRQ,don't mask off

	assume	ds:code			;
	mov	dx,21h			; DX = 21H, assume master 8259 port addr
	cmp	al,8			; check input IRQ vs. master 8259 IRQ
	jb	mask_not_irq2		; jump, if master 8259's IRQ
	mov	dx,0a1h			; DX = A1H, slave 8259 port address
	sub	al,8			; AL = map IRQ to slave 8259 IRQ
mask_not_irq2:
	mov	cl,al			; CL = AL, IRQ # for master/slave 8259
					; DX = determined 8259 port address
	in	al,dx			; AL = 8259 IRQ mask reg value
	mov	ah,1			; AH = 1 i.e bit 0 set.
	shl	ah,cl			; AH = set proper bit correspond IRQ #
	or	al,ah			; AL = new 8259 IRQ mask setting
;
;----------------------------------------
; 500ns Stall required here, per INTEL documentation for eisa machines
; - gft - 910617
;----------------------------------------
;
	push	ax			; save new 8259 IRQ mask setting
					; assume AT bus speed is standard
					; and fixed(8MHz = 125ns/cycle)
					; an i/o read requires 5 cycles
					; ISA 8 bit read cycle minimun 530ns
					; 3 times 8bit io read delay 1.5 us
	in	al,61h			; Dummy read port 61h(status port)
	in	al,61h			; Dummy read port 61h(status port)
	in	al,61h			; Dummy read port 61h(status port)
	pop	ax			; restore new 8259 IRQ mask setting
	out	dx,al			; set determined IRQ mask register
maskint_1:				;
	ret				; return to caller


;
;----------------------------------------
; input: AL = IRQ number to unmask
;
;----------------------------------------
;
unmaskint:
	assume	ds:code			;
	mov	dx,21h			; DX = 21H, assume master 8259 port addr
	mov	cl,al			; CX = input IRQ # to unmask
	cmp	cl,8			; check input IRQ vs. master 8259 IRQ
	jb	unmask_not_irq2		; jump, if master 8259's IRQ
	in	al,dx			; AL = master IRQ mask reg value
	and	al,not (1 shl 2)	; AL = clear slave cascade bit in mask
;	push	ax			; save new 8259 IRQ mask setting
;					; 3 times 8bit io read delay 1.5 us
;	in	al,61h			; Dummy read port 61h(status port)
;	in	al,61h			; Dummy read port 61h(status port)
;	in	al,61h			; Dummy read port 61h(status port)
;	pop	ax			; restore new 8259 IRQ mask setting
	out	dx,al			; set new master mask (enable slave int)
;
;----------------------------------------
; 500ns Stall required here, per INTEL documentation for eisa machines
; - gft - 910617
;----------------------------------------
;
					; assume AT bus speed is standard
					; and fixed(8MHz = 125ns/cycle)
					; an i/o read requires 5 cycles
					; ISA 8 bit read cycle minimun 530ns
					; 3 times 8bit io read delay 1.5 us
	push	ax			; save AX
	in	al,61h			; Dummy read port 61h(status port)
	in	al,61h			; Dummy read port 61h(status port)
	in	al,61h			; Dummy read port 61h(status port)
	pop	ax			; restore AX
	mov	dx,0a1h			; DX = slave 8259 port address
	sub	cl,8			; CL = IRQ # map to slave 8259
unmask_not_irq2:

	in	al,dx			; AL = IRQ mask reg value of determined 8259
	mov	ah,1			; AH = 1 ie. bit 0 set
	shl	ah,cl			; AH = proper bit set correspond IRQ #
	not	ah			; AH = proper bit clear correspond IRQ #
	and	al,ah			; AL = clear input IRQ bit
;
;----------------------------------------
; 500ns Stall required here, per INTEL documentation for eisa machines
; - gft - 910617
;----------------------------------------
;
	push	ax			; save new 8259 IRQ mask setting
					; 3 times 8bit io read delay 1.5 us
	in	al,61h			; Dummy read port 61h(status port)
	in	al,61h			; Dummy read port 61h(status port)
	in	al,61h			; Dummy read port 61h(status port)
	pop	ax			; restore new 8259 IRQ mask setting
	out	dx,al			; set determined IRQ mask register
					  
	ret				; return to caller


;
;----------------------------------------
; called when we want to determine what to do with a received packet.
;
; input:  cx = packet length
;	  es:di -> packet type
;	  dl = packet class.
;
; output: es:di = 0 if the packet is not desired
;	  es:di -> packet buffer to be filled by the driver.
;  
;----------------------------------------
;
recv_find:
	assume	ds:code, es:nothing	;
	push	cx			; save packet length

;
;----------------------------------------
; If -n option take IEEE 802.3 encapsulated packets that could be Novell IPX 
; and make them Ethernet encapsulated Novell IPX packets (for PDSHELL).
;----------------------------------------
;
	test	flagbyte,N_OPTION	; check user "-n" option
	jz	not_n_op		; jump, if not

;
;----------------------------------------
; Make IEEE 802.3-like packets that could be Novell IPX into BlueBook class
; Novell type 8137 packets.
; !!!Please refer to f_send_pkt for IPX data structure!!!
;----------------------------------------
;
	cmp	dl,IEEE8023		; check DL=input class vs. IEEE 802.3
	jne	recv_not_802_3		; jump, if class not an IEEE 802.3 
	cmp	word ptr es:[di],0ffffh	; check packet type vs. 0xffff
	jne	recv_not_8137		; jump, if type not IPX
	sub	di,2			; ES:DI = addr len byte of IEEE 802.3
	mov	es:[di],3781h		; write(0x8137) fake as Novell protocol
	mov	dl,BLUEBOOK		; set DL = BLUEBOOK class
	jmp	short recv_not_8137	;
recv_not_802_3:
;
;----------------------------------------
; Convert incoming Ethernet type 8137 IPX packets to type 8138,
; as with -n in effect we can't send type 8137, 
; and it will only confuse Netware.
;----------------------------------------
;
	cmp	dl,BLUEBOOK		; check DL=input class vs. BLUEBOOK
	jne	recv_not_8137		; jump, if class not a BLUEBOOK
	cmp	word ptr es:[di],3781h	; check pocket type vs. 0x8137 packet
	jne	recv_not_8137		; jump, if type not 8137
	mov	es:[di],word ptr 3881h	; write 0x8138 
recv_not_8137:
not_n_op:

	mov	bx,offset handles	; BX = address of 1st handle
recv_find_1:
	cmp	[bx].in_use,0		; check current handle in use
	je	recv_find_2		; jump, if not in use
					; handle in use
	mov	ax,[bx].receiver.offs	; AX = receiver's offset
					; check receiver not equal null
	or	ax,[bx].receiver.segm	; AX = receiver's offset OR segment
	je	recv_find_2		; jump, if receiver not exist

	mov	cx,[bx].packet_type_len	; CX = current handle's packet len 
	lea	si,[bx].packet_type	; SI = addr of current handle's type
	jcxz	recv_find_3		; jump, if packet len = 0

	cmp	[bx].class, dl		; check handle's class = input class
	jne	recv_find_2		; jump, if not equal

	push	di			; save input packet type offset
	repe	cmpsb			; compare the packet
	pop	di			; restore input packet type offset
	je	recv_find_3		; jump, if the same, otherwise,next
recv_find_2:				; 
	add	bx,(size per_handle)	; BX = address of next handle
	cmp	bx,offset end_handles	; check BX = end of last handle
	jb	recv_find_1		; loop, if not reach end of last handle
					; can't find a correct one
	linc	packets_dropped		; increment pkt drop statistic

	pop	cx			; restore input packet length
recv_find_5:
	xor	di,di			; return DI = null
	mov	es,di			; return ES = null
	ret				; return to caller
recv_find_3:
	pop	cx			; restore the input packet length

	linc	packets_in		; increment rcv pkt statistic
	add	bytes_in.offs,cx	; add up the received bytes.
	adc	bytes_in.segm,0		;

	les	di,[bx].receiver	; DI = address of receiver
	mov	receive_ptr.offs,di	; receiver routine offset = input DI
	mov	receive_ptr.segm,es	; receiver routine segment = input ES

	test	flagbyte,W_OPTION	; check user select "-w" option
	je	recv_find_6		; jump, if not "-w" option
;
;----------------------------------------
; does the receiver signature match whats currently in memory?
; if not,jump to fake return
;----------------------------------------
;
	push	si			; save SI
	push	cx			; save input packet len
	lea	si,[bx].receiver_sig	; SI = addr handle's receiver signature
	mov	cx,8/2			; CX = signature word len
	repe	cmpsw			; compare signature
	pop	cx			; restore input packet len
	pop	si			; restore SI
	jne	recv_find_5		; jump, if signature ont the same
recv_find_6:

	mov	found_handle,bx		; found handle = current handle
	mov	ax,0			; AX = allocate buffer request.
	stc				; set carry flag, indicate an odd number
	push	ax			; save allocate request & used as flags
	pushf				; save flags reg in case iret.
	call	receive_ptr		; call pkt rcv client for a buffer.
;
;----------------------------------------
; on return, flags should be at top of stack. if an IRET has been used,
; then 0 will be at the top of the stack
;----------------------------------------
;
	pop	bx			; BX = restore reg from stack
	cmp	bx,0			; check same allocate buffer request
	je	recv_find_4		; jump, if same allocate buffer(iret)
	add	sp,2			; normal return, balanced stack 
recv_find_4:
	ret				; return to caller


;
;----------------------------------------
; called after we have copied the packet into the buffer.
;
; input: ds:si ->the packet
;	 cx = length of the packet.
;
; preserve bx.
;----------------------------------------
;
recv_copy:
	assume	ds:nothing, es:nothing	;

	push	bx			; save register
	mov	bx,found_handle		; BX = previous found handle 
	mov	ax,1			; AX = store buffer request.
	clc				; clear carry flag, indicate an even #
	push	ax			; save store buffer request & used as flags
	pushf				; save flags reg incase iret used.
	call	receive_ptr		; call pkt rcv client to copy a buffer.
	pop	bx			; BX = restore reg from stack
	cmp	bx,1			; check BX = store buffer request
	je	recv_copy_1		; jump, if store buufer request(iret)
	pop	bx			; normal return, balance stack
recv_copy_1:
	pop	bx			; restore register
	ret				; return to caller

;
;----------------------------------------
; Queue an iocb.
; input:  es:di -> iocb
;	  interrupts disabled.
;
; modify: ds:si.
;----------------------------------------
;
send_queue:
	assume	ds:nothing, es:nothing	;
	mov	es:[di].next.offs,0	; set input blk next ptr offset = 0
	mov	es:[di].next.segm,0	; set input blk next ptr segment = 0
	mov	si,send_head.offs	; SI = xmit queue head's offset
	or	si,send_head.segm	; SI = xmit queue head's offset OR segment 
	jnz	sq_notempty		; jump, if not and empty queue
	mov	send_head.offs,di	; Set xmit queue head offset=input DI
	mov	send_head.segm,es	; Set xmit queue head segment=input ES
	jmp	sq_settail		; jump to set xmit queue tail
sq_notempty:				; Queue is not empty
	lds	si,send_tail		; DS:SI = xmit queue tail segment:offset
					; enqueue an iocb in queue
	mov	ds:[si].next.offs,di	; set current tail next offset = in DI
	mov	ds:[si].next.segm,es	; Set current tail next segment= in ES
sq_settail:
	mov	send_tail.offs,di	; set xmit queue tail offset= in DI
	mov	send_tail.segm,es	; set xmit queue tail segment= in ES
	ret


;
;----------------------------------------
; Dequeue an iocb and possibly call its upcall.
; input:  device or processor interrupts disabled
;	  ah = return code.
;
; output: es:di -> iocb
;
; modify: ds:si, ax, bx, cx, dx, bp.
;----------------------------------------
;
send_dequeue:
	assume	ds:nothing, es:nothing
	les	di,send_head		; ES:DI = xmit queue head
	lds	si,es:[di].next		; DS:SI = xmit queue head's next block
	mov	send_head.offs, si	; xmit queue new head = current head's
	mov	send_head.segm, ds	; next block
	or	es:flags[di], DONE	; set previous head's DONE flag
	mov	es:ret_code[di], ah	; set previous head's retcode
	test	es:[di].flags,CALLME	; check previous head's CALLME upcall
	je	send_dequeue_1		; jump, if no upcall
	push	es			; save ES = iocb segment
	push	di			; save DI = iocb offset
	clc				; Clear carry flag
	mov	ax,1			; AX = 1 a number cant be flags
	push	ax			; save a number on stack
	pushf				; Save flags reg on stack in case iret
	call	es:[di].upcall		; Call the client.
	pop	ax			; AX = first word on stack.
	cmp	ax,1			; check AX = 1
	je	send_dequeue_2		; jump, if AX = 1(iret happened)
	add	sp,2			; normal return, balance stack
send_dequeue_2:
	pop	di			; restore iocb segment
	pop	es			; restore iocb offset
send_dequeue_1:				;
	ret				; return to caller


code	ends

	end	start

