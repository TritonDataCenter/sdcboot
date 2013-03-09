; include file for the Free-DOS XMS driver
; Copyright 2001-2005, Martin Str”mberg.
; Copyright (c) 1995, Till Gerken

INFO_STR		equ	'80286 64MB version'
DRIVER_VERSION		equ	'0.03.Temperaments'
DRIVER_VER		=	003
INTERFACE_VERSION	equ	'2.0'
INTERFACE_VER		=	200h

XMS_START		=	1088	; XMS starts at 1088k. After HMA.
XMS_MAX 		= 	0fbc0h	; The maximum amount of XMS memory.
INT15_MAX		= 	XMS_MAX-1	; The maximum amount of INT15 reserved memory.
INT15_MAX_STR		equ	"0xfbbf"
NUMHANDLES_MAX		=	1024

CMD_INIT		=	0	; Init command (used when installed).
CMD_ISTATUS		= 	6	; Input status command (used when checking if this is a (character?) device).
CMD_OSTATUS		= 	0ah	; Output status command (used when checking if this is a (character?) device).

STATUS_DONE		=	0100h	; Driver is done.
STATUS_ERROR		=	8000h	; Driver failure.
ERROR_BAD_CMD		=	0003h	; Unknown command.

FLAG_INITIALISED	= 	01h
FLAG_HMA_USED		= 	02h
FLAG_A20_STATE		= 	04h	; Keeps A20 state across INT15 call.

VDISK_ID_STR		equ	"VDISK"
VDISK_ID_LEN		=	5
VDISK_ID_INT19_OFS	=	12h
VDISK_ID_HMA_OFS	=	13h

XMS_NOT_IMPLEMENTED	=	80h
XMS_VDISK_DETECTED	=	81h
XMS_A20_FAILURE		=	82h
XMS_DRIVER_FAILURE	=	8eh
XMS_DRIVER_FATAL	=	8fh
XMS_HMA_NOT_THERE	=	90h
XMS_HMA_IN_USE		=	91h
XMS_HMAREQ_TOO_SMALL	=	92h
XMS_HMA_NOT_USED	=	93h
XMS_A20_STILL_ENABLED	=	94h
XMS_ALREADY_ALLOCATED	=	0a0h
XMS_NO_HANDLE_LEFT	=	0a1h
XMS_INVALID_HANDLE	=	0a2h
XMS_INVALID_SOURCE_HANDLE =	0a3h
XMS_INVALID_SOURCE_OFFSET =	0a4h
XMS_INVALID_DESTINATION_HANDLE=	0a5h
XMS_INVALID_DESTINATION_OFFSET=	0a6h
XMS_INVALID_LENGTH	=	0a7h
XMS_OVERLAP		=	0a8h
XMS_PARITY_ERROR	=	0a9h
XMS_BLOCK_NOT_LOCKED	=	0aah
XMS_BLOCK_LOCKED	=	0abh
XMS_LOCK_COUNT_OVERFLOW	=	0ach
XMS_LOCK_FAILED		=	0adh
XMS_ONLY_SMALLER_UMB	=	0b0h
XMS_NO_UMB_AVAILABLE	=	0b1h
XMS_UMB_SEGMENT_NR_INVALID =	0b2h

XMS_COMMAND_LINE_LENGTH_MAX = 	100h

struc	request_hdr
  req_size	db	?		; number of bytes stored
  unit_id	db	?		; unit ID code
  cmd		db	?		; command code
  status	dw	?		; status word
  rsvd		db	8 dup (?)	; reserved
ends	request_hdr

struc	init_strc
  init_hdr	db	size request_hdr dup (?)
  units		db	?		; number of supported units
  end_addr	dd	?		; end address of resident part
  cmd_line	dd	?		; address of command line
ends	init_strc

struc	desc
  limit		dw	0ffffh		; segment limit
  base0_15	dw	?		; low word of base address
  base16_23	db	?		; high byte of base address
  flags		db	93h		; std read/write segment
  reserved	db	0
ends	desc

struc	xms_handle
  xbase		dw	?		; base address in kbytes
  xsize		dw	?		; size in kbytes
  used		db	?		; 1 if used, > 1 if locked.
ends	xms_handle

struc	xms_move_strc
  len_low		dw	?		; block length in bytes
  len_high		dw	?		; block length in bytes
  src_handle		dw	?		; source handle
  src_offset_low	dw	?		; offset into source
  src_offset_high	dw	?		; offset into source
  dest_handle		dw	?		; destination handle
  dest_offset_low	dw	?		; offset into destination
  dest_offset_high	dw	?		; offset into destination
ends	xms_move_strc

