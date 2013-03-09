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
;   PC/FTP Packet Driver source, conforming to version 1.05 of the spec
;   Updated to version 1.08 Feb. 17, 1989.
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
; 06-13-94 D.T.	add full duplex(FDUP) support data structure
; 11-16-94 D.T.	add DMA timer(BUSTIMER) support data structure
;
;-----------------------------------------------------------------------
.286

INCLUDE	defs.asm			;
;
;----------------------------------------
;
;----------------------------------------
;
code	segment word public		;
	assume	cs:code, ds:code	;
					;
EXTRN	phd_dioa: byte			; ptr to DOS cmd tail & buffer bytes
EXTRN	phd_environ: word		; ptr to DOS environ var addr word
EXTRN	their_isr: dword		; ptr to original pkt drv ISR stoe address(dword)
EXTRN	is_at: byte			; ptr to PC AT flag byte
EXTRN	sys_features: byte		; ptr to sys feature byte(MC/2nd 8259)
EXTRN	int_no: byte			; ptr to IRQ, I/O addr, DMA info bytes
EXTRN	flagbyte: byte			; ptr to user options/card init. flagbyte
EXTRN	packet_int_no: byte		; ptr to pkt drv int # bytes(4)
EXTRN	driver_class: byte		; ptr to class bytes(Bluebook/IEEE802.3..etc)

EXTRN	usage_msg: byte			; "usage: driver [-d -n] <packet_int_no> <args>"
EXTRN	already_msg:byte		; packet driver already exist message
EXTRN	int_msg:byte			; IRQ number range error message
EXTRN	int_msg_num:word		; IRQ number upper limit message
EXTRN	no_resident_msg:byte		; borad init error message
EXTRN	copyright_msg: byte		; PCnet-ISA driver copyright msg bytes
EXTRN	copyleft_msg:byte		; original copyright message
EXTRN	packet_int_msg:byte		; packet software interrupt number message
EXTRN	location_msg:byte		; packet driver location message
EXTRN	packet_int_num:byte		; packet driver IRQ number message
EXTRN	eaddr_msg:byte			; packet driver ethernet address message
EXTRN	aaddr_msg:byte			; packet driver ARCnet address message
EXTRN	crlf_msg:byte			; carriage return & line feed message

EXTRN	parse_args: near		; parse the arguments.
EXTRN	print_parameters: near		; ptr to print IRQ,I/O addr,DMA info.
EXTRN	our_isr: near			; ptr to our pkt drv ISR routine
EXTRN	etopen: near			; ptr to init. card & cascade DMA routine 
EXTRN	get_address: near		; ptr to get interface addr routine 

EXTRN	devices_init: near		; PCnet device detect & init
EXTRN	display_error_message: near	; display error message

PUBLIC	etopen_diagn			; ptr to etopen err level byte
PUBLIC	error				; ptr to error handle routine
PUBLIC	start_1				; ptr to pkt drv init routine

PUBLIC	keyword_string_table		; ptr to keyword string table
PUBLIC	b_kint_no			; ptr to keyword string INT #
PUBLIC	b_kirq_no			; ptr to keyword string IRQ #
PUBLIC	b_kdma_no			; ptr to keyword string DMA #
PUBLIC	b_kioaddr_no			; ptr to keyword string IO addr
PUBLIC	b_ktp_en			; ptr to keyword string TP
PUBLIC	b_kdmarotate			; ptr to keyword string DMA ROTATE
PUBLIC	b_kbustype			; ptr to keyword string BUSTYPE
PUBLIC	b_kled0				; ptr to keyword string LED 0
PUBLIC	b_kled1				; ptr to keyword string LED 1
PUBLIC	b_kled2				; ptr to keyword string LED 2
PUBLIC	b_kled3				; ptr to keyword string LED 3
PUBLIC	b_kfdup				; ptr to keyword string FDUP
PUBLIC	b_kdmatimer			; ptr to keyword string BUS(DMA)TIMER
PUBLIC	bustype_table			; ptr to bustype string table
PUBLIC	b_pci_bus			; ptr to pci bustype string
PUBLIC	b_vesa_bus			; ptr to vesa bustype string
PUBLIC	b_pnp_bus			; ptr to pnp bustype string
PUBLIC	b_isa_bus			; ptr to isa bustype string
PUBLIC	b_pci1_bus			; ptr to pci bustype string mechanism1
PUBLIC	b_pci2_bus			; ptr to pci bustype string mechanism2
PUBLIC	bustype_para_table		; ptr to bustype parameter table
PUBLIC	fullduplex_table		; ptr to full duplex string table
PUBLIC	b_10baseT			; ptr to fdup 10baseT string
PUBLIC	b_AUI				; ptr to fdup AUI string
PUBLIC	b_fdup_off			; ptr to fdup disable string
PUBLIC	fullduplex_para_table		; ptr to full duplex parameter table

INCLUDE	printnum.asm			;
INCLUDE	decout.asm			;
INCLUDE	digout.asm			;
INCLUDE	chrout.asm			;

end_tail_1	LABEL	byte		; end of the resident driver

signature	db	'PKT DRVR',0	;
signature_len	equ	$-signature	;
					; from verifypi.asm(07-12-93,end)
our_address	db	EADDR_LEN dup(?); ethernet address

;
;----------------------------------------
;
;----------------------------------------
;
etopen_diagn	db	0		; errorlevel from etopen if set
;
;----------------------------------------
; keyword string
;----------------------------------------
;
keyword_string_table	label byte

b_kint_no	db	'INT',0		; interrupt number
b_kirq_no	db	'IRQ',0		; IRQ number
b_kdma_no	db	'DMA',0		; DMA number
b_kioaddr_no	db	'IOADDR',0	; I/O address
b_ktp_en	db	'TP',0		; Twisted Pair
b_kdmarotate	db	'DMAROTATE',0	; DMA rotate priority
b_kbustype	db	'BUSTYPE',0	; BUS type for scan
b_kled0		db	'LED0',0	; LED 0
b_kled1		db	'LED1',0	; LED 1
b_kled2		db	'LED2',0	; LED 2
b_kled3		db	'LED3',0	; LED 3
b_kfdup		db	'FDUP',0	; FDUP
b_kdmatimer	db	'BUSTIMER',0	; BUS(DMA) timer
;
;----------------------------------------
; bustype string
;----------------------------------------
;
bustype_table	label byte
b_pci_bus	db	'PCI',0		; PCI device/bus type
b_vesa_bus	db	'VESA',0	; VESA device/bus type
b_pnp_bus	db	'PNP',0		; Plug and play device
b_isa_bus	db	'ISA',0		; ISA devices
b_pci1_bus	db	'PCI1',0	; PCI device/bus type mechanism 1
b_pci2_bus	db	'PCI2',0	; PCI device/bus type mechanism 2

;
;----------------------------------------
; bustype internal representation
;----------------------------------------
;
bustype_para_table	label byte
		db	11h		; PCI device/bus type
		db	10h		; VESA device/bus type
		db	01h		; Plug and play device
		db	00h		; ISA devices
		db	11h		; PCI device/bus type mechanism 1
		db	11h		; PCI device/bus type mechanism 2
;
;----------------------------------------
; full duplex string
;----------------------------------------
;
fullduplex_table	label byte
b_10baseT	db	'10BASET',0	; full duplex use 10 base T connector
b_AUI		db	'AUI',0		; full duplex use AUI connector
b_fdup_off	db	'OFF',0		; disable full duplex

;
;----------------------------------------
; full duplex internal representation
;----------------------------------------
;
fullduplex_para_table	label byte
		db	01h		; full duplex use 10 base T connector
		db	03h		; full duplex use AUI connector
		db	00h		; disable full duplex

;
;----------------------------------------
;
;----------------------------------------
;
already_error:				;
	mov	ax,DRIVER_EXIST		; AX = driver already exist error number
	mov	dx,offset already_msg	; DX = address of aleardy message
	call	display_error_message	; display error message
	;
	mov	di,offset packet_int_no	; DX = address of pkt int #
	call	print_number		; print message & interrupt #
	mov	ax,4c05h		; (AH,AL) = (terminate,errorlevel 5)
	int	21h			; terminate and return error code 5

usage_error:				;
	mov	dx,offset usage_msg	; DX = address of usage message
	mov	ax,USAGE_HELP		; AX = usage help error number

;
;----------------------------------------
;
;----------------------------------------
;
error:					;
	call	display_error_message	; display error message
error_out:
	mov	ax,4c0ah		; (AH,AL) = (terminate,errorlevel 10)
	int	21h			; terminate and return error code 10

INCLUDE	timeout.asm			;

;
;----------------------------------------
;
;----------------------------------------
;
start_1:				;
	cld				; set string operation direction flag

	mov	dx,offset copyright_msg	; DX = address of copyright message
	mov	ah,9			; AH = display string
	int	21h			; display string DS:DX

	mov	dx,offset copyleft_msg	; DX = address of copyleft message
	mov	ah,9			; AH = display string
	int	21h			; display string DS:DX

;
;----------------------------------------
; Get the feature byte (if reliable) so we can know if it is a microchannel
; computer and how many interrupts there are.
;----------------------------------------
;
	mov	ah,0c0h			; AH = 0xC0 get feature bytes
	int	15h			; ES:BX <- sys features block
	jc	look_in_ROM		; jump, if carry flag, error use rom.
	or	ah,ah			; check ah = 0
	jnz	look_in_ROM		; jump, if ah not equal zero
	mov	dx,es:[bx]		; DX = # of feature bytes
	cmp	dx,4			; check DX vs. the feature bytes
	jae	got_features		; jump, if return feature bytes >= 4
look_in_ROM:				;
	mov	dx,0f000h		; DX = ROM segment
	mov	es,dx			; ES = 0xF000
	cmp	byte ptr es:[0fffeh],0fch; check an AT special byte at 0xfffe
	jne	identified		; jump, if value at 0xfffe != 0xfc
	or	sys_features,TWO_8259	; set sys features byte has 2nd 8259
	jmp	identified		; assume no microchannel and jump
got_features:
	mov	ah,es:[bx+2]		; model byte
	cmp	ah,0fch			;
	je	at_ps2			;
	ja	identified		; FD, FE and FF are not ATs
	cmp	ah,0f8h			;
	je	at_ps2			;
	ja	identified		; F9, FA and FB are not ATs
	cmp	ah,09ah			;
	jbe	identified		; old non-AT Compacs go here
at_ps2:					; 9B - F8 and FC are assumed to
	mov	ah,es:[bx+5]		;   have reliable feature byte
	mov	sys_features,ah		;
identified:

	mov	si,offset phd_dioa+1	; SI = address to DOS command tail
	call	skip_blanks		; skip blank & ACSII 9
	cmp	al,CR			; check 1st available character = CR
	je	usage_error_j_1		; jump, if AL = carriage return
;
;----------------------------------------
; print the location we were loaded at.
;----------------------------------------
;
	mov	dx,offset location_msg	; DX = address of location message
	mov	ah,9			; AH = display string
	int	21h			; display string DS:DX

	mov	ax,cs			; AX = CS
	call	wordout			; print AX as a word size

	mov	dx,offset crlf_msg	; DX = address of CR & LF message
	mov	ah,9			; AH = display string
	int	21h			; display string DS:DX

chk_options:
	call	skip_blanks		; skip blanks & ASCII 9
	cmp	al,'-'			; check first available char = '-'
	jne	no_more_opt		; jump, if 1st available char != '-'
	inc	si			; skip the option char '-'
	lodsb				; AL = next char
	or	al,20h			; AL = converted lower case char
	cmp	al,'d'			; check AL = 'd'
	jne	not_d_opt		; jump, if AL != 'd'
	or	flagbyte,D_OPTION	; set flagbyte D_OPTION on
	jmp	chk_options		; loop, check option again
not_d_opt:
	cmp	al,'n'			; check AL = 'n'
	jne	not_n_opt		; jump, if AL != 'n'
	or	flagbyte,N_OPTION	; set flagbyte N_OPTION on
	jmp	chk_options		; loop, check option again
not_n_opt:
	cmp	al,'w'			; check AL = 'w'
	jne	not_w_opt		; jump, if AL != 'w'
	or	flagbyte,W_OPTION	; set flagbyte W_OPTION on
	jmp	chk_options		; loop, check option again
not_w_opt:
usage_error_j_1:
	jmp	usage_error		; jump to usage error
no_more_opt:
					; parse the input parameters
;
;----------------------------------------
; parse_args should parse the arguments.
; called with ds:si -> immediately after the driver's segment address
; (move the packet INT number parsing into parse_args routine)
;----------------------------------------
;
	call	parse_args		; parse keyw:INT,[IRQ,IOaddr,DMA#,..]
	jc	usage_error_j_1		; jump, if input error indicated
					; check end of line
;	call	skip_blanks		; skip blanks & ASCII 9
;	cmp	al,CR			; check first available char = CR
;	jne	usage_error_j_1		; jump, if input error
;
;----------------------------------------
; Detect PCnet device
;----------------------------------------
;

start_device_detect:			; 

	call	devices_init		; PCnet device detect & init.
	jnc	device_detect_done	; jump, no error
	jmp	error_out		; error, exit(error message displayed)
	;
device_detect_done:			; 
	call	verify_packet_int	; check pkt drv existence
	jnc	packet_int_ok		; jump, if pkt int is ok
	mov	ax,INT_RANGE		; AX = INT range error number
	jmp	error			; jump, pkt drv bad
packet_int_ok:
	jne	packet_int_unused	; check pkt drv in use
	jmp	already_error		; jump, if there's one there, error.
packet_int_unused:			; pkt drv int is ok.
					
;
;----------------------------------------
; Verify that the interrupt number they gave is valid.
;----------------------------------------
;
	cmp	int_no,15		; check int no(IRQ #) > 15
	ja	int_bad			; jump, if current IRQ # > 15
	test	sys_features,TWO_8259	; check 2nd 8259 existence
	jnz	int_ok			; jump, if 2nd 8259, don't check <= 7.
	mov	int_msg_num,'7'+' '*256	; modify int msg num from '15' to ' 7'
	cmp	int_no,7		; check int no(IRQ #) < 7
	jbe	int_ok			; jump, if current IRQ # < 7
int_bad:				; if IRQ # is bad
	mov	dx,offset int_msg	; DX = addr of int msg
	mov	ax,IRQ_RANGE		; AX = IRQ range error number
	jmp	error			; jump to error handling
int_ok:					; IRQ # is good

;
;----------------------------------------
; Map IRQ 2 to IRQ 9 if needed.
;----------------------------------------
;
	test	sys_features,TWO_8259	; check system has 2nd 8259
	je	no_mapping_needed	; jump, if 2nd 8259 not exist
	cmp	int_no,2		; check IRQ 2 selected
	jne	no_mapping_needed	; jump, if IRQ 2 not selected
	mov	int_no,9		; map int no from IRQ 2 to IRQ 9.
no_mapping_needed:
;
;----------------------------------------
; If they chose the -d option, 
; don't call etopen when we are loaded,
; but when we are called for the first time
;
; Save part of the tail, needed by delayed etopen
;----------------------------------------
;
	test	flagbyte,D_OPTION	; check user option '-d'(delayed init)
	jnz	delayed_open		; jump, if delayed option selected
;
;----------------------------------------
; etopen should initialize the device.  If it needs to give an error, it
; can issue the error message and quit to dos.
;----------------------------------------
;
	call	etopen			; init the device.
	jnc	yes_resident		; jump, if no error(carry flag clear)
	jmp	no_resident		; jump, if error occured
delayed_open:
	mov	dx,offset end_tail_1	; DX = addr of end delayed init label
	push	dx			; save addr of end delayed init label
	call	take_packet_int		; save original & setup our packet int
	jmp	delayed_open_1		; jump, continue delayed init device

yes_resident:
	push	dx			; save DX(addr of end resident)

	call	print_parameters	; print IRQ #, io addr, DMA info.
	or	flagbyte,CALLED_ETOPEN	; set flag byte CALLED_ETOPEN flag bit

	call	take_packet_int		; save original & setup our packet int

	cmp	driver_class,1		; check driver class = Ethernet
	jne	print_addr_2		; jump, if driver class != ethernet

	push	ds			; save DS
	pop	es			; ES = DS
	mov	di,offset our_address	; DI = addr of our address bytes
	mov	cx,EADDR_LEN		; CX = ethernet addr length
;
;----------------------------------------
; get the address of the interface.
; enter with
;   es:di -> place to put the address,
;   cx = size of address buffer.
; exit with
;   nc, cx = actual size of address, 
;   or cy if buffer not big enough.
;----------------------------------------
;
	call	get_address		; get ethernet address from device
					; & put in our address
	mov	dx,offset eaddr_msg	; DX = address of etheraddress message
	mov	ah,9			; AH = display string
	int	21h			; display string DS:DX

	mov	si,offset our_address	; SI = addr of our ethernet address
	call	print_ether_addr	; print ethernet address in DS:SI

	mov	dx,offset crlf_msg	; DX = address of CR & LF message
	mov	ah,9			; AH = display string
	int	21h			; display string DS:DX

print_addr_2:
					; did we support ARCnet ????
	cmp	driver_class,8		; check driver class = ARCnet
	jne	print_addr_3		; jump, if driver class != ARCnet

	push	ds			; save DS
	pop	es			; ES = DS
	mov	di,offset our_address	; DI = addr of our address bytes
	mov	cx,ARCADDR_LEN		; CX = ARC addr length
;
;----------------------------------------
; get the address of the interface.
; enter with
;   es:di -> place to get the address,
;   cx = size of address buffer.
; exit with
;   nc, cx = actual size of address,
;   or cy if buffer not big enough.
;----------------------------------------
;
	call	get_address		; get ethernet address from device
					; & put in our address
	mov	dx,offset aaddr_msg	; DX = address of aaddr message
	mov	ah,9			; AH = display string
	int	21h			; display string DS:DX

	mov	al,our_address		; AL = our address
	mov	cl,' '			; CL = char ' ', include leading zero
	call	byteout			; convert/print AL into hex char

	mov	dx,offset crlf_msg	; DX = address of CR & LF message
	mov	ah,9			; AH = display string
	int	21h			; display string DS:DX

print_addr_3:
delayed_open_1:

	mov	ah,49h			; AH = free memory block
	mov	es,phd_environ		; ES = environment segment
	int	21h			; free ES = environment segment

	mov	bx,1			; BX = the stdout handle.
	mov	ah,3eh			; AH = close file(in case redirected)
	int	21h			; close file of handle

	pop	dx			; restore addr of end delayed init label
	add	dx,0fh			; DX = round up to next highest paragraph.
	mov	cl,4			; CL = shift count
	shr	dx,cl			; convert DX from binary to hex
	mov	ah,31h			; AH = terminate, stay resident.
	mov	al,etopen_diagn		; AL = errorlevel(0 - 9, diagnostics)
	int	21h			; terminate process & stay resident

no_resident:
	mov	dx,offset no_resident_msg; DX = address no resident mseeage
	mov	ax,DRIVER_INIT_BAD	; AX = driver init fail error number
	call	display_error_message	; display error message

	mov	ax,4c00h + 32		; (AH,AL) = (terminate,errorlevel 32)
	cmp	al,etopen_diagn		; check AL = etopen errorlevel
	ja	no_et_diagn		; jump, if AL > etopen errorlevel
	mov	al,etopen_diagn		; AL = etopen errorlevel
no_et_diagn:
	int	21h			; terminate and return error code 32

;
;----------------------------------------
; 			Suggested errorlevels:
;
; _____________________  0 = normal
; 			 1 = unsuitable memory address given; corrected
; In most cases every-	 2 = unsuitable IRQ level given; corrected
; thing should work as	 3 = unsuitable DMA channel given; corrected
; expected for lev 1-5	 4 = unsuitable IO addr given; corrected (only 1 card)
; _____________________	 5 = packet driver for this int # already loaded
; External errors, when	20 = general cable failure (but pkt driver is loaded)
; corrected normal	21 = network cable is open             -"-
; operation starts	22 = network cable is shorted          -"-
; _____________________ 23 = 
; Packet driver not	30 = usage message
; loaded. A new load	31 = arguments out of range
; attempt must be done	32 = unspecified device initialization error
;			33 = 
;			34 = suggested memory already occupied
;			35 = suggested IRQ already occupied
;			36 = suggested DMA channel already occupied
;			37 = could not find the network card at this IO address
;----------------------------------------
;

take_packet_int:			;
	mov	ah,35h			; AH = get interrupt number
	mov	al,packet_int_no	; AL = packet interrupt #
	int	21h			; get packet int # in ES:BX
	mov	their_isr.offs,bx	; their isr offset = BX
	mov	their_isr.segm,es	; their isr segment = ES

	mov	ah,25h			; AH = set interrupt number
	mov	dx,offset our_isr	; DX = address of pkt drv ISR
	int	21h			; set ISR addr as DS:DX
	ret				; return to caller

INCLUDE	verifypi.asm			;
INCLUDE	getnum.asm			;
INCLUDE	getdig.asm			;
INCLUDE	skipblk.asm			;
INCLUDE	printea.asm			;

code	ends

	end

