; FreeDOS NLSFUNC
; Copyright (c) 2004,2005 Eduardo Casino <casino_e@terra.es>
;
; This program is free software; you can redistribute it and/or modify
; it under the terms of the GNU General Public License as published by
; the Free Software Foundation; either version 2 of the License, or
; (at your option) any later version.
;
; This program is distributed in the hope that it will be useful,
; but WITHOUT ANY WARRANTY; without even the implied warranty of
; MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
; GNU General Public License for more details.
;
; You should have received a copy of the GNU General Public License
; along with this program; if not, write to the Free Software
; Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
;
; NOTE: The memory allocation function is basically copied from rbkeyswp
;       by Ralf Brown. Rbkeyswp is in the public domain, many thanks
;       to Ralf!
;
; NOTE2: I've included quotes from Ralf Brown Interrupt List into the
;        comments to ease understanding of the code.
;        
; 04-12-05  Eduardo Casino   First version
; 05-01-12  Eduardo Casino   Fix bug in command line parsing. Kernel
;                            compatibility checks.
; 05-11-18  Eduardo Casino   IOCTL support
; 06-08-22  Eduardo Casino   Fix memory allocation bug
;
; TODOS: * Fallback mechanisms
;        * Check that AX == CX for disk reads
;        * Check if a package is already loaded
;        * Better memory handling
;        * Localization
;

; Version 2 uses words instead of bytes for Yes and No characters
;
%ifndef NLSFUNC_VERSION
%define NLSFUNC_VERSION 0xFD02	; FreeDOS NLS version we are compatible with
%endif

%if (NLSFUNC_VERSION != 0xFD01) && (NLSFUNC_VERSION != 0xFD02)
%error This NLSFUNC only supports NLS versions 0xFD01 and 0xFD02
%endif

NLS_FLAG_DIRECT_UCASE		equ	0x0001  ; DOS-65-2[012]
NLS_FLAG_DIRECT_FUCASE		equ	0x0002  ; DOS-65-A[012]
NLS_FLAG_DIRECT_YESNO		equ	0x0004  ; DOS-65-23
NLS_FLAG_DIRECT_GETDATA		equ	0x0008  ; DOS-65-XX, DOS-38
NLS_FLAGS			equ	NLS_FLAG_DIRECT_UCASE | NLS_FLAG_DIRECT_FUCASE | NLS_FLAG_DIRECT_YESNO | NLS_FLAG_DIRECT_GETDATA

NLS_MAX_PKGSIZE		equ	1009
;
;          16 (FAR * + UWORD + UWORD + int + UWORD + UWORD + unsigned)
;         +7  +(6 * 4) (all subfunctions + pointers)
;         +2  +28  (country info)              f0
;         +2  +128 (uppercase)                 f2
;         +2  +256 (lowercase)                 f3
;         +2  +128 (fuppercase)                f4
;         +2  +22  (fterminator)               f5
;         +2  +256 (collate)                   f6
;         +2  +130 (DBCS)                      f7
;         = 						1009 bytes
; DBCS leadbytes are always >= 128. The DBCS table
; consists on leadbyte ranges. Worst case is that
; alternate bytes are leadbytes, thus we have 64 ranges.
; This means that the maximum DBCS table size is 
;  2 (length) + 64*2 (ranges) + 2 (0x0000: end mark)


%ifdef NEW_NASM
	        cpu     8086
%endif
%include "exebin.mac"

; ===========================================================================
; RESIDENT PART
; ===========================================================================

		EXE_begin
		EXE_stack 512
		SECTION .text

; We are using the original PSP as read buffer. However, the maximum size we
; need is 258 (256 bytes plus (word) length), so we add this extra word right
; after the PSP
; 
readbuf_ext	dw	0
;
;


orig_stack	dd	0		; Caller's stack
stack_buffer	dd	0		; Buffer for saving caller's stack

default_file	db	"\COUNTRY.SYS", 0
country_file	dd	0		; Pointer to actual country file name
tblidx_off	dd	0		; Off. of first entry in file v. 0xFD02
					; or off. of cty/cp tbl. index v 0xFD01
nls_pkg		dd	0		; Pointer to NLS Package Buffer
read_buf	dd	0		; Pointer to Read Buffer
kern_buf	dd	0		; Pointer to Kernel Buffer
buf_len		dw	0		; Length of Kernel Buffer
num_pkg		dw	0		; Number of NLS packages in file
meta_siz	dw	0		; Size of Metadata information

DE_INVLDFUNC	equ	1
DE_FILENOTFND	equ	2
result		dw	0

subfnct		dw	0

NLS_DEFAULT	equ	0xFFFF
country		dw	0
codepage	dw	0
currpos		dd	0
table_start	dw	0
country_info	dw	0
tmp_buf		dd	0


; Pointer to Kernel NLS info
;
kernel_nls	dd	0

; Pointers to hardcoded tables
;
hc_ucase_fn	dd	0
hc_ucase	dd	0
hc_fucase	dd	0
hc_collate	dd	0
hc_fchar	dd	0
hc_dbcs		dd	0
hc_yesno	dd	0	; Not really a table


;%define NLS_YESNO_IS_MANDATORY
;
; Subfunctions in COUNTRY.SYS
;
NLS_CTYINFO	equ	0x0001
NLS_UCASE	equ	0x0002
NLS_LCASE	equ	0x0004
NLS_FUCASE	equ	0x0008
NLS_FCHAR	equ	0x0010
NLS_COLLATE	equ	0x0020
NLS_DBCS	equ	0x0040
NLS_YESNO	equ	0x0100
NLS_LOADYESNO	equ	0x1000	; Flag if the YESNO table is to be loaded

NLS_MANDATORY	equ	NLS_CTYINFO | NLS_UCASE | NLS_FUCASE | NLS_FCHAR | NLS_COLLATE | NLS_DBCS

%ifdef NLS_YESNO_IS_MANDATORY
NLS_MANDATORY	equ	NLS_MANDATORY | NLS_YESNO
%endif
nls_subfcts	dw	0	; FLAGS: NLS subfunctions present in package

; ===========================================================================
;  INT 2F NLSFUNC API
; ===========================================================================


						; FreeDOS NLS version we are
						; compliant with
NLS_FREEDOS_NLSFUNC_VERSION	equ	NLSFUNC_VERSION
NLS_FREEDOS_NLSFUNC_ID		equ	0x534b	; FreeDOS NLSFUNC magic #

chain:		db	0xEA			; jmp far
old_int2f	dw	0,0

int2f:		cmp	ah, 0x14		; New Int 2Fh entry point
		jne	chain

i2f14:		cmp	al, 0x00
		je	i2f1400
		cmp	al, 5
		jnc	chain			; Function > 4

; Quote from Bart Oldeman:
;
; 2. nlsfunc would have to copy anything in between ss:sp and ss:920
;    (_disk_api_tos, that's the top of the stack used here in any DOS >=
;     4.0) to a temp area (max 384 bytes), set sp to 920, and with that call
;     DOS. Then after the call adjust the stack pointer, then swap it back,
;     then return.
;
		push	ds
		push	es
		push	si
		push	di

		mov	[cs:orig_stack+2], ss
		mov	[cs:orig_stack], sp

		push	es
		push	ds
		push	si
		push	di
		push	cx

		lds	si, [cs:orig_stack]
		les	di, [cs:stack_buffer]
		mov	cx, 0x0920			; _disk_api_tos
		sub	cx, si
		cld
		rep	movsb

		pop	cx
		pop	di
		pop	si
		pop	ds
		pop	es

		mov	sp, 0x0920			; Reset stack

		cmp	al, 0x04
		jne	cmp_1403
		jmp	i2f1404
cmp_1403:	cmp	al, 0x03
		jne	cmp_1402
		jmp	i2f1403
cmp_1402:	cmp	al, 0x02
		jne	i2f1401
		jmp	i2f1402

; ----- MUX-14-00 : NLSFUNC - INSTALLATION CHECK ---------------------------
;
i2f1400:	cmp	bx, NLS_FREEDOS_NLSFUNC_VERSION
		jne	chain		; Invalid Kernel NLS version
		cmp	cx, NLS_FREEDOS_NLSFUNC_ID
		jne	chain		; Invalid magic number
		mov	bx, cx		; Return magic number in BX
		mov	al, 0xFF
		iret


; ----- MUX-14-01 : NLSFUNC - CHANGE CODE PAGE ------------------------------
;
; It calls the internals of MUX-14-03 and then informs the drivers of the
; code page change
;   FIXME: (Check return codes)
;
i2f1401:	call	load_nls_info
		jc	.ret

		call	nls_set_cp
.ret:		jmp	i2f14_ret	; AX holds return code

nls_set_cp:	push	es
		push	di
		push	ds
		push	si
		push	dx
		push	cx
		push	bx

		mov 	word [cs:result], 0


		mov	ax, 0xAD00		; DISPLAY: INSTALLATION CHECK
		int	0x2F
		cmp	al, 0xFF
		jc	.disperr		; NO DISPLAY

		; DISPLAY version < 1.00, notify using its MUX-AD interface
		;
		cmp	bx, 0x0100
		jnc	.ioctl

		mov	ax, 0xAD01		; SET ACTIVE CODE PAGE
		mov	bx, [cs:codepage]
		int	0x2F
		jnc	.ioctl

; 41h (65)  network: Access denied
;          (DOS 3.0+ [maybe 3.3+???]) codepage switching not possible
;
.disperr:	mov	word [cs:result], 0x41


		; Change code page using the IOCTL interface
		;
		; We're using read_buf in the following way
		;
		; Offset  Length    Description
		; 00h  9  BYTEs     ASCIZ Device name (Max len == 8 plus NULL
		; 10h               Datablock for IOCTL
		;  ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
		; 10h    WORD    length of data
		; 12h    WORD    code page ID (see #01757 at INT 21/AX=6602h)
		; 14h 2N BYTEs   DCBS (double byte charset) lead byte range
		;                  start/end for each of N ranges (DOS 4.0)
		;        WORD    0000h  end of data (DOS 4.0)
		;
.ioctl:		les	di, [cs:read_buf]
		add	di, 0x10

		mov	ax, [cs:codepage]
		mov	[es:di+2], ax

		; Locate the DBCS table
		;
		lds	si, [cs:kern_buf]
		lds	si, [si+8]		; nlsPackage
		mov	cx, [si+14]		; Number of subfunctions
		add	si, 16			; Begin pointers
.find_sf7:	cmp	byte [si], 7
		je	.sf7_found
		add	si, 5
		loop	.find_sf7

		; Not found. It should never reach here. Anyway,
		; put an empty DBCS table and go on.
		;
		mov	word [es:di], 2		; Length of codepage
		jmp	.getchain

.sf7_found:	lds	si, [si+1]		; Pointer to DBCS table
		mov	cx, [si]		; Length of DBCS table
		mov	[es:di], cx
		add	word [es:di], 2		; Length of codepage
		add	di, 4
		add	si, 2
		cld				; Copy table
		rep	movsb

.getchain:	mov	ax, 0x122C	; GET DEVICE CHAIN
		int	0x2F		; BX:AX
		mov	es, bx
		mov	di, ax

		; Format of DOS device driver header:
		; Offset  Size    Description     (Table 01646)
		;  00h    DWORD   pointer to next drv, offset=FFFFh if last drv
		;  04h    WORD    device attributes (see #01647,#01648)
		;  06h    WORD    device strategy entry point
		;                 call with ES:BX -> request header
		;                 (see #02597 at INT 2F/AX=0802h)
		;  08h    WORD    device interrupt entry point
		;  ---character device---
		;  0Ah  8 BYTEs   blank-padded character device name
		;
		; Bitfields for device attributes (character device):
		; Bit(s)  Description     (Table 01647)
		;  15     set (indicates character device)
		;  14     IOCTL supported (see AH=44h)
		;  13     (DOS 3.0+) output until busy supported
		;  12     reserved
		;  11     (DOS 3.0+) OPEN/CLOSE/RemMedia calls supported
		;  10-8   reserved
		;  7      (DOS 5.0+) Generic IOCTL check call supported
		;            (driver command 19h)
		;         (see AX=4410h,AX=4411h)
		;  6      (DOS 3.2+) Generic IOCTL call supported (driver
		;            command 13h)
		;         (see AX=440Ch,AX=440Dh"DOS 3.2+")
		;  5      reserved
		;  4      device is special (use INT 29 "fast console output")
		;  3      device is CLOCK$
		;  2      device is NUL
		;  1      device is standard output
		;  0      device is standard input


.chkchar:	mov	ax, [es:di+4]		; Device atributes
		test	ah, 0x80		; Character device?
		jz	.jmpnext
		test	al, 0x40		; Generic IOCTL?
		jnz	.chkbusy
.jmpnext:	jmp	.nextdev

		;  INT 2F - DOS 3.3+ PRINT - GET PRINTER DEVICE
		;          AX = 0106h
		;  Return: CF set if files in print queue
		;              AX = error code 0008h (queue full)
		;              DS:SI -> device driver header (see #01646)
		;          CF clear if print queue empty
		;              AX = 0000h
		;  Desc:  determine which device, if any, PRINT is currently
		;         using for output
		;  Notes: undocumented prior to the release of MS-DOS 5.0
		;         this function can be used to allow a program to avoid
		;         printing to the printer on which PRINT is currently
		;         performing output

		; Check if busy
		;
.chkbusy:	mov	ax, 0x0106		; Get printer device
		clc				; Clear carry in case PRINT is
		push	ds			; not installed, 
		push	si
		int	0x2F

		jnc	.notbusy		; Print queue empty
						; (or 0x0106 not implemented)

		; Check if we are trying to write to the busy device
		; ES:DI == DS:SI ??
		;
		; DS:SI device header
		;
		cmp	di, si
		jne	.notbusy

		mov	ax, ds
		mov	bx, es
		cmp	ax, bx
		jne	.notbusy

		mov	word [cs:result], 0x41
		pop	si
		pop	ds
		jmp	.nextdev

.notbusy:	; Get device name from [es:di+10] (8 chars)
		; (Copy to buffer as Open File needs ASCIZ)

		push	di

		mov	cx, 8
		add	di, 10
		lds	si, [cs:read_buf]
.nextc:		mov	al, [es:di]
		cmp	al, ' '
		je	.terminate
		mov	[si], al
		inc	si
		inc	di
		loop	.nextc
.terminate:	mov	byte [si], 0


		mov	ax, 0x1226		; Open File
		mov	cl, 0x01		; Write only
		lds	dx, [cs:read_buf]	; Points to ASCIZ device name
		int	0x2F

		pop	di
		pop	si
		pop	ds

		jnc	.call_ioctl
		mov	word [cs:result], 0x41
		jmp	.nextdev		; Failed to open

		;INT 2F U - DOS 3.3+ internal - IOCTL
		;        AX = 122Bh
		;        BP = 44xxh (BP=440Ch Generic Character Device Request)
		;        BX = device handle
		;        CH = category code (see #01545)
		;        CL = function number (see #01546)
		;        DS:DX -> parameter block (see #01549)
		;        SI = parameter to pass to driver
		;                 (European MS-DOS 4.0, OS/2 comp box)
		;        DI = parameter to pass to driver
		;                 (European MS-DOS 4.0, OS/2 comp box)
		;Return: CF set on error
		;            AX = error code (see #01680 at AH=59h/BX=0000h)
		;        CF clear if successful
		;
		;(Table 01545)
		;Values for IOCTL category code:
		; 00h    unknown (DOS 3.3+)
		;
		;(Table 01546)
		;Values for generic character IOCTL function:
		; 4Ah    select code page (see #01549)
		;
		;(Table 01549)
		;Format of parameter block for functions 4Ah and 6Ah:
		;Offset  Size    Description
		; 00h    WORD    length of data
		; 02h    WORD    code page ID (see #01757 at INT 21/AX=6602h)
		; 04h 2N BYTEs   DCBS (double byte char set) lead byte range
		;                  start/end for each of N ranges (DOS 4.0)
		;        WORD    0000h  end of data (DOS 4.0)
		; 


.call_ioctl:	push	bp
		mov	bx, ax
		mov	cx, 0x004A	; Select Code Page
		mov	bp, 0x440C	; Generic IOCTL
		mov	ax, 0x122B
		lds	dx, [cs:read_buf]
		add	dx, 0x10
		int	0x2F
		pop	bp

		jnc	.closedev
		cmp	ax, 0x01	; "function number invalid"
		je	.closedev

		; If device reports "unknown command", just ignore it.
		; Any other error is relevant. We need to check the extended
		; error information.
		;
		mov	ax, 0x122D	; Get Extended Error Code
		int	0x2F
		cmp	ax, 0x16	; "unknown command given to driver"
		je	.closedev

		; Any other error, set error code
		;
		mov	word [cs:result], 0x41

		; In any case, close device and get next one
		;
.closedev:	mov	ax, 0x1227		; Close File 
		int	0x2F
		jnc	.nextdev
		mov	word [cs:result], 0x41

.nextdev:	cmp	word [es:di], 0xFFFF	; Last?
		je	.ret
		les	di, [es:di]		; Load next device
		jmp	.chkchar	
		

.ret:		mov	ax, [cs:result]
		pop	bx
		pop	cx
		pop	dx
		pop	si
		pop	ds
		pop	di
		pop	es
		ret


; ----- MUX-14-03 : NLSFUNC - SET CODE PAGE ---------------------------------
;                   Called by int 2138h
;
; Obtains all the country's nls info from COUNTRY.SYS and updates the internal
; tables.
;
; NOTE: From kernel/nls.c
;     "Therefore, setPackage() will substitute the current country ID, if
;     cntry==-1, but leaves cp==-1 in order to let NLSFUNC choose the most
;     appropriate codepage on its own."
;
;     In this implementation "most appropriate" means "the one in the first
;     entry found", so entries in COUNTRY.SYS must be arranged with this in
;     mind.
;
i2f1403:	call	load_nls_info
		jc	.ret
		mov	bx, [cs:codepage]
.ret:		jmp	i2f14_ret



; ----- MUX-14-02 : NLSFUNC - GET EXTENDED COUNTRY INFO ---------------------
;                   Called by int 2165h
;
; BP	- Subfunction
; BX    - Code page
; DX    - Country code
; DS:SI - Internal code page structure
;           Offset  Size    Description
;            00h    DWORD   pointer to country file name
;            04h    WORD    system code page
;            06h    WORD    implementation flags
;            08h    DWORD   pointer to current NLS package
;            0Ch    DWORD   pointer to first item of info chain
; ES:DI - Kernel buffer
; CX    - Buffer size
;
; Returns: Carry set on error, AX == DOS error code or DE_FILENOTFND if !found
;          Carry clear on success, AX == 0
;
i2f1402:	call	resolve_defaults
		cmp	bp, 0
		je	i2f1402_inv
		cmp	bp, 1
		je	i2f1402_sf1		; Subfunction 1
		cmp	bp, 8
		jb	i2f1402_sf2		; Subfunctions 2 to 7
i2f1402_inv:	mov	ax, DE_INVLDFUNC
		jmp	i2f14_ret

i2f1402_sf1:	cmp	cx, 31
		jnc	i2f1402_sf1a		; Buffer is big enough
		mov	ax, DE_INVLDFUNC
		jmp	i2f14_ret

i2f1402_sf1a:	push	di
		mov	byte [es:di], 1		; Subfunction 1
		mov	word [es:di+1], 28	; Size of following info
		mov	word [es:di+3], dx	; Country
		mov	word [es:di+5], bx	; Codepage
		add	di, 7			; Copy country info just after
		mov	[cs:kern_buf], di	;     codepage
		pop	di
		mov	cx, 24			; Size of data to copy
		mov	[cs:codepage], bx
		jmp	i2f1404_2		; Rest is shared with i2f1404

		; Subfunctions 2 to 7 all return a pointer to the nls info
		;
i2f1402_sf2:	cmp	cx, 5
		jnc	i2f1402_sf2a
		mov	ax, DE_INVLDFUNC
		jmp	i2f14_ret
i2f1402_sf2a:	push	es
		push	di
		push	bp
		mov	[cs:codepage], bx
		mov	[cs:country], dx
		mov	[cs:subfnct], bp
		call	get_nls_info
		pop	bp
		pop	di
		pop	es
		jnc	i2f1402_sf2b
		jmp	i2f14_ret
i2f1402_sf2b:	mov	ax, bp
		mov	[es:di], al
		mov	ax, [cs:read_buf]
		mov	[es:di+1], ax
		mov	ax, [cs:read_buf+2]
		mov	[es:di+3], ax
		xor	ax, ax
		jmp	i2f14_ret
		

; ----- MUX-14-04 : NLSFUNC - GET COUNTRY INFO ------------------------------
;                   Called by int 2138h
;
; BX    - Code page					(Ignored)
; DX    - Country code
; DS:SI - Internal code page structure
;           Offset  Size    Description
;            00h    DWORD   pointer to country file name
;            04h    WORD    system code page
;            06h    WORD    implementation flags
;            08h    DWORD   pointer to current NLS package
;            0Ch    DWORD   pointer to first item of info chain
; ES:DI - Kernel buffer
; CX    - Buffer size
;
; Returns: Carry set on error, AX == DOS error code or DE_FILENOTFND if !found
;          Carry clear on success, AX == 0
;
i2f1404:	call	resolve_defaults
		mov	[cs:kern_buf], di
		mov	word [cs:codepage], NLS_DEFAULT ; don't check for cp
i2f1404_2:	mov	ax, es
		mov	word [cs:subfnct], 1
		mov	[cs:country], dx
		mov	[cs:buf_len], cx
		mov	[cs:kern_buf+2], ax

		; Returns: Carry set on error, AX == Error code
		;          Carry clear on success
		call	get_nls_info
		jc	i2f14_ret

		mov	cx, [cs:buf_len]
		lds	si, [cs:read_buf]	; Copiar info en el buffer
		add	si, 6			; Skip length, country, cp
		les	di, [cs:kern_buf]
		cld
		rep	movsb
		mov	ax, cx		; Should be Zero if OK


i2f14_ret:	; Restore DOS stack
		;
		les	di, [cs:orig_stack]
		lds	si, [cs:stack_buffer]
		mov	cx, 0x0920		; _disk_api_tos
		sub	cx, di
		cld
		rep	movsb

		mov	sp, [cs:orig_stack]

		pop	di
		pop	si
		pop	es
		pop	ds

i2f14_ret2:	push	bp
		mov	bp, sp
		or	ax, ax
		jnz	i2f14_retc		; set carry
			; -6 == (BP), -2 (CS), -4 (IP), -6 (flags)
			; current SP = on old_BP
		and	byte [ss:bp-6], 0xfE	; clear carry
		pop	bp
		iret
i2f14_retc:	or	byte [ss:bp-6], 1
		pop	bp
		iret


; ---------------------------------------------------------------------------
; Function: resolve_country - Updates country with current kernel value if
;                             country == NLS_DEFAULT
;
; Args:     DX              - Country code
;           
; Returns:  DX updated
;
resolve_country:
		push	es
		push	di
		les	di, [si+8]
		jmp	chk_country

; ---------------------------------------------------------------------------
; Function: resolve_defaults - Updates country and codepage with current kernel
;                              values if country == NLS_DEFAULT and/or
;                              codepage == NLS_DEFAULT, respectively.
;
; Args:     BX               - Code page
;           DX               - Country code
;           
; Returns:  BX, DX updated
;
resolve_defaults:
		push	es
		push	di
		les	di, [si+8]
		cmp	bx, NLS_DEFAULT
		jne	chk_country
		mov	bx, [es:di+6]
chk_country:	cmp	dx, NLS_DEFAULT
		jne	.ret
		mov	dx, [es:di+4]
.ret:		pop	di
		pop	es
		ret


; ---------------------------------------------------------------------------
; Function: load_nls_info - Reads NLS info from COUNTRY.SYS
;
; Args:     BX            - Code page
;           DX            - Country code
;           DS:SI         - Pointer to kernel NLS info
;           
;
load_nls_info:	call	resolve_country
		mov	[cs:country], dx
		mov	[cs:codepage], bx
		mov	[cs:kern_buf], si
		mov	dx, ds
		mov	[cs:kern_buf+2], dx

		lds	dx, [cs:country_file]
		
		xor	cx, cx
		mov	ax, 0x1226		; Open read-only
		int	0x2F

		jnc	.getpkg
.ret:		ret

.getpkg:	
		call	get_nls_pkg
		jnc	.ldmetadata
		jmp	nls_close

.ldmetadata:	
		mov	bp, [cs:codepage]
		lds	si, [cs:read_buf]
		les	di, [cs:nls_pkg]
		mov	word [es:di], 0
		mov	word [es:di+2], 0
		mov	ax, [cs:country]
		mov	[es:di+4], ax		; Country
		mov	[es:di+6], bp		; Codepage

		mov	word [es:di+8], NLS_FLAGS

		mov	cx, [si]		; Number of subfct following

%if NLSFUNC_VERSION == 0xFD01
		mov	al, [cs:hc_yesno]	; Default values
		mov	[es:di+10], al
		mov	al, [cs:hc_yesno+1]
		mov	[es:di+11], al
		add	di, 12
%else
		mov	ax, [cs:hc_yesno]	; Default values
		mov	[es:di+10], ax
		mov	ax, [cs:hc_yesno+2]
		mov	[es:di+12], ax
		add	di, 14
%endif
		mov	[es:di], cx

		; As we are assuming less that 16 subfunctions per entry,
		; all the subfunction data offsets are already in ds:si
		;
		
		; PASS 1:
		; Check which subfunctions are present and update number
		; of subfunctions if YESNO is present (as its info is
		; integrated into the NLS package header)
		;
		; If YESNO is present AND /Y is on, update the Y/N info
		;
		add	si, 2
chk_sf:		mov	ax, [si+2]
		
		cmp	ax, 0x23
		jne	.sf1
		or	word [cs:nls_subfcts], NLS_YESNO
		dec	word [es:di]	; Update number of subfunctions

		; Read Yes/No info only if /Y is on
		;
		test	 word [cs:nls_subfcts], NLS_LOADYESNO
		jz	.sf_next

		call	read_sf23
		jnc	.sf_next

		mov	[cs:result], ax
		jmp	nls_close

.sf1:		cmp	ax, 1
		jne	.sf2
		or	word [cs:nls_subfcts], NLS_CTYINFO
		jmp	.sf_next

.sf2:		cmp	ax, 2
		jne	.sf3
		or	word [cs:nls_subfcts], NLS_UCASE
		jmp	.sf_next

.sf3:		cmp	ax, 3
		jne	.sf4
		or	word [cs:nls_subfcts], NLS_LCASE
		jmp	.sf_next

.sf4:		cmp	ax, 4
		jne	.sf5
		or	word [cs:nls_subfcts], NLS_FUCASE
		jmp	.sf_next

.sf5:		cmp	ax, 5
		jne	.sf6
		or	word [cs:nls_subfcts], NLS_FCHAR
		jmp	.sf_next

.sf6:		cmp	ax, 6
		jne	.sf7
		or	word [cs:nls_subfcts], NLS_COLLATE
		jmp	.sf_next

.sf7:		cmp	ax, 7
		jne	.unsupported
		or	word [cs:nls_subfcts], NLS_DBCS
		jmp	.sf_next

.unsupported:	dec	word [es:di]

.sf_next:	dec	cx
		jcxz	chk_mandatory
		add	si, 8
		jmp	chk_sf


		; Check that all the mandatory subfunctions are present
		;
chk_mandatory:	mov	ax, [cs:nls_subfcts]
		and	ax, NLS_MANDATORY
		cmp	ax, NLS_MANDATORY
		je	chk_missing
		
		mov     word [cs:result], DE_FILENOTFND
		jmp	nls_close

		; PASS 2:
		; Check which subfunctions are missing and point to
		; the hardcoded tables, if necessary
		;
		; Locate the pointers to data at predicatble positions to
		; comply with NLS_REORDER_POINTERS (even if it is not defined,
		; it won't hurt)
		;
chk_missing:	mov	ax, [cs:nls_subfcts]
		test	ax, NLS_UCASE
		jnz	.sf4miss

		
		; UCASE (sf2) should be nlsPointers[0]
		;
		mov	byte [es:di+2], 2
		mov	cx, [cs:hc_ucase]
		mov	[es:di+3], cx
		mov	cx, [cs:hc_ucase+2]
		mov	[es:di+5], cx
		inc	word [es:di]	; Update number of subfunctions

.sf4miss:	test	ax, NLS_FUCASE
		jnz	.sf5miss

		; FUCASE (sf4) should be nlsPointers[1]
		;
		mov	byte [es:di+7], 4
		mov	cx, [cs:hc_fucase]
		mov	[es:di+8], cx
		mov	cx, [cs:hc_fucase+2]
		mov	[es:di+10], cx
		inc	word [es:di]	; Update number of subfunctions

.sf5miss:	test	ax, NLS_FCHAR
		jnz	.sf6miss

		; FCHAR (sf5) should be nlsPointers[2] (not really, but
		; it is its position in the harcoded package)
		;
		mov	byte [es:di+12], 5
		mov	cx, [cs:hc_fchar]
		mov	[es:di+13], cx
		mov	cx, [cs:hc_fchar+2]
		mov	[es:di+15], cx
		inc	word [es:di]	; Update number of subfunctions

.sf6miss:	test	ax, NLS_COLLATE
		jnz	.sf7miss

		; COLLATE (sf6) should be nlsPointers[3] (again, it is
		; its position in the harcoded package)
		;
		mov	byte [es:di+17], 6
		mov	cx, [cs:hc_collate]
		mov	[es:di+18], cx
		mov	cx, [cs:hc_collate+2]
		mov	[es:di+20], cx
		inc	word [es:di]	; Update number of subfunctions

.sf7miss:	test	ax, NLS_DBCS
		jnz	loadnlsinfo

		; DBCS (sf7) should be nlsPointers[4] (irrelevant for
		; NLSFUNC_VERSION 0xFD01, but necessary for 0xFD02)
		;
		mov	byte [es:di+22], 7
		mov	cx, [cs:hc_dbcs]
		mov	[es:di+23], cx
		mov	cx, [cs:hc_dbcs+2]
		mov	[es:di+25], cx
		inc	word [es:di]	; Update number of subfunctions

		; PASS 3:
		; Re-read the subfct tables and load the info
		;
loadnlsinfo:	mov	cx, [es:di]	; Number of subfct following
		dec	cx		; minus subfct 1
		
		; Calculate nlsinfo start
		;
		add	di, 2
		mov	ax, 5			; subfct + pointer
		mul	cl			; Subfunctions < 256
		add	ax, di
		mov	[cs:country_info], ax

		; Calculate nls tables start
		; 
		; 1Ch (country info) + 3 (subfct + length word)
		;
		add	ax, 0x1F
		mov	[cs:table_start], ax

		; Actual number of subfunctions in file
		;
		lds	si, [cs:read_buf]
		mov	cx, [si]

		add	si, 2

sbfctloop:	mov	ax, [si+2]
		cmp	ax, 0x23
		jne	.chk_sf1
		jmp	.next			; Ignore

.chk_sf1:	cmp	ax, 1
		jne	.fillentry

		; Special treatment for country info
		;
		push	ax			; For load_table()
		mov	bp, [cs:country_info]   
		mov	[es:bp], al
		inc	bp
		push	es			;
		push	bp			;
		call	load_table

		jnc	.set_ucasefn
		mov     [cs:result], ax
		jmp	nls_close

.set_ucasefn:	mov	bp, [cs:country_info]
		mov	ax, [cs:hc_ucase_fn]
		mov	[es:bp+25], ax
		mov	ax, [cs:hc_ucase_fn+2]
		mov	[es:bp+27], ax
		
		jmp	.next

		; Fill in the subcft entry
		;
.fillentry:	mov	bp, di
		cmp	ax, 2
		je	.writesf
		cmp	ax, 3
		jne	.is_sf4
		add	bp, 25			; If exists, put it last
		jmp	.writesf
.is_sf4:	cmp	ax, 4
		jne	.is_sf5
		add	bp, 5
		jmp	.writesf
.is_sf5:	cmp	ax, 5
		jne	.is_sf6
		add	bp, 10
		jmp	.writesf
.is_sf6:	cmp	ax, 6
		jne	.is_sf7
		add	bp, 15
		jmp	.writesf
.is_sf7:	cmp	ax, 7
		jne	.next		; Unsupported. Ignore.
		add	bp, 20

.writesf:	push	ax			; For load_table()
		mov	[es:bp], al		; Subfct
		mov	ax, [cs:table_start]
		mov	[es:bp+1], ax
		mov	ax, es
		mov	[es:bp+3], ax


		; Search and load info
		;
		push	es			; Discarded on function exit
		push	word [cs:table_start]	;
		call	load_table

		jnc	.new_start
		mov     [cs:result], ax
		jmp	nls_close

.new_start: 	add	word [cs:table_start], ax

.next:		dec	cx
		jcxz	hook
		add	si, 8
		jmp	sbfctloop


read_sf23:	push	cx
		mov	dx, [si+4]	; Seek Yes/No table
		mov	cx, [si+6]
		mov	bp, 0x4200
		mov	ax, 0x1228
		int	0x2F
		pop	cx

		jc	.ret

		push	ds
		push	cx
		lds	dx, [cs:tmp_buf]
		mov	cx, 14		; Tag + Sig + Len + 2 * WORD
		mov	ax, 0x1229
		int	0x2F
		pop	cx

		jnc	.updateyesno
		pop	ds
.ret:		ret

.updateyesno:	push	si
		mov	si, dx
%if NLSFUNC_VERSION == 0xFD01
		; Update only if single byte. If not, leave the defaults.
		;
		mov	ax, [si+12]
		cmp	ah, 0
		jne	.dontupdate	; It's a dual-byte char
		mov	[es:di-1], al
		mov	ax, [si+10]
		mov	[es:di-2], al
%else
		mov	ax, [si+10]
		mov	[es:di-4], ax
		mov	ax, [si+12]
		mov	[es:di-2], ax
%endif
.dontupdate:	pop	si
		pop	ds
		ret


hook:		lds	si, [cs:kern_buf]

		mov	ax, [cs:nls_pkg]
		mov	dx, [cs:nls_pkg+2]

; Until the kernel has full COUNTRY.SYS support, we are not chaining but
; _replacing_ the active package.
;
%ifdef DO_CHAIN
		; Check if already hooked and chain if not.
		;
		cli				; Protect the change!

		les	di, [si+12]		; First item in chain

.loop		mov	cx, es
		jcxz	.dochain		; Not found
		cmp	cx, dx
		jne	.next
		cmp	ax, di
		je	.chained		; Already chained
.next:		les	di, [es:di]		; Next item
		jmp	.loop
		
.dochain:	les	di, [si+8]		; Current active package
		mov	[es:di], ax		; Chains new package
		mov	[es:di+2], dx
		mov	[si+8], ax		; Updates actPkg
		mov	[si+10], dx

.chained:	sti
%else
		mov	[si+8], ax		; Updates nlsInfo.actPkg
		mov	[si+10], dx
		mov	[si+12], ax		; Updates nlsInfo.chain
		mov	[si+14], dx
%endif
		mov	word [cs:result], 0
		jmp	nls_close

; ---------------------------------------------------------------------------
; Function: load_table - 
;
; Args:
;           
; Returns:
;
load_table:	push	cx
		push	di
		push	ds

		; Search and load info
		; (bx already set)
		mov	bp, 0x4200
		mov	ax, 0x1228
		mov	dx, [si+4]
		mov	cx, [si+6]
		int	0x2F
		jc	.ret
		
		lds	dx, [cs:tmp_buf]
		mov	cx, 0x0A
		mov	ax, 0x1229
		int	0x2F

		jc	.ret

		; FIXME: Add signature checking (is it necessary?)
		;
		mov	bp, sp	
		mov	ax, [ss:bp+12]
		cmp	ax, 1
		je	.no_backwards

		; move pointer 2 bytes backwards (to re-read table
		; size information
		;
		push	ax
		mov	di, dx
		mov	dx, -2
		mov	cx, 0xFFFF
		mov	bp, 0x4201	; Seek from current
		mov	ax, 0x1228
		int	0x2F
		pop	ax

		jc	.ret

.no_backwards:	mov	cx, [di+8]	; Read size
		mov	bp, sp	
		lds	dx, [ss:bp+8]	; DS + +DI + CX + IP

		; For country info, MSDOS compatible COUNTRY.SYS has a length
		; of 38 bytes, although last 10 are unused. FreeDOS allows
		; only 28, so update length.
		;
		cmp	ax, 1
		jne	.chk_sf7
		mov	cx, 28
		mov	di, dx
		mov	[di], cx
		inc	dx
		inc	dx
		jmp	.do_read

.chk_sf7:	; Subfunction 7 (DBCS) is a bit special. Even if length is 0
		; there is a table terminator (0000h)
		; 
		cmp	ax, 7
		jne	.cont
		or	cx, cx
		jnz	.cont
		add	cx, 2

.cont:		add	cx, 2		; Length of table + word (length)

.do_read:	mov	ax, 0x1229
		int	0x2F
		
.ret:		pop	ds
		pop	di
		pop	cx
		ret	6

; ---------------------------------------------------------------------------
; Function: get_nls_info - 
;
; Args:
;           
; Returns:
;
get_nls_info:	
		lds	dx, [cs:country_file]
		
		xor	cx, cx			; Read-only
		mov	ax, 0x1226		; Open
		int	0x2F

		jnc	.getpkg
		ret

.getpkg:	call	get_nls_pkg		; Find and read pkg metadata
		jnc	.chksbfn
		jmp	nls_close

		; Check subfunction
		; We are assuming that there are less than 16 subfunctions
		; per entry, which is more than reasonable
		;
.chksbfn:	mov	di, dx
		mov	cx, [di]
		jcxz	.nfound
		add	di, 2			; Point to first subfunction

.loop:		mov	ax, [di+2]		; subfunct id
		cmp	ax, [cs:subfnct]
		je	.found
		add	di, 8			; Point to next subfunction
		loop	.loop
.nfound:	mov	word [cs:result], DE_FILENOTFND
		jmp	nls_close

.found:		
		mov	dx, [di+4]		; subfn-data-entry
		mov	cx, [di+6]
		mov	ax, 0x1228		; BP, BX  already set
		int	0x2F
		jnc	.readsubfct
		mov	[cs:result], ax
		jmp	nls_close

.readsubfct:	mov	ax, 0x1229		; Read subfunction data hdr
		mov	cx, 0x0A
		lds	dx, [cs:read_buf]	
		int	0x2F
		jnc	.getsbfctlen
		mov	[cs:result], ax
		jmp	nls_close

.getsbfctlen:	mov	di, dx
		mov	cx, [di+8]		; header length

		mov	[di], cx		; Put length in the beginning
		inc	dx			; of the buffer and move two
		inc	dx			; bytes forward
		jcxz	.zerolength

		mov	ax, 0x1229		; read data
		int	0x2F
		jnc	.success
		mov	[cs:result], ax
		jmp	nls_close

.zerolength:	cmp	word [cs:subfnct], 7	; DBCS has a table terminator
		jne	.success
		mov	di, dx
		mov	word [di], 0

.success:	mov	word [cs:result], 0

nls_close:	mov	ax, 0x1227
		int	0x2F

		mov	ax, [cs:result]
		or	ax, ax
		jz	nls_noerr
		stc
		ret
nls_noerr:	clc
		ret


; ---------------------------------------------------------------------------
; Function: get_nls_pkg - Open file and read pkg metadata
;
; Args:
;           
; Returns:	BX file handle
;               DX metadata pointer in file
; Trashes:	AX, CX, BP, DS, DI
;
ENTRY_SIZE	equ	14
ENTRY_NUM	equ	18
COUNTRY_OFF	equ	2
CODEPAGE_OFF	equ	4
HEADER_OFF	equ	10

get_nls_pkg:	mov	bx, ax			; Look for table
		mov	dx, [cs:tblidx_off]
		mov	cx, [cs:tblidx_off+2]
		mov	bp, 0x4200
		mov	ax, 0x1228		; Seek
		int	0x2F

		jnc	.readtable
		mov	[cs:result], ax
		ret
.readtable:	mov	cx, 2 + (ENTRY_SIZE * ENTRY_NUM)
		lds	dx, [cs:read_buf]
		mov	ax, 0x1229
		int	0x2F

		jnc	.srchentry
		mov	[cs:result], ax
		ret
.srchentry:	mov	di, dx
		mov	cx, [di]	; Number of entries
		inc	di
		inc	di
.chknum:	jcxz	.nfound
		cmp	cx, ENTRY_NUM
		jb	.clean
		sub	cx, ENTRY_NUM
		mov	[cs:num_pkg], cx
		mov	cx, ENTRY_NUM
		jmp	.srchloop
.clean:		mov	word [cs:num_pkg], 0
.srchloop:	mov	ax, [cs:country]
		cmp	ax, [di+COUNTRY_OFF]
		jne	.next
		mov	ax, [cs:codepage]
		cmp	ax, NLS_DEFAULT		; do not check for codepage
		jne	.cmpcp
		mov	ax, [di+CODEPAGE_OFF]
		mov	[cs:codepage], ax	; Update codepage
		jmp	.found
.cmpcp:		cmp	ax, [di+CODEPAGE_OFF]
		je	.found
.next:		add	di, ENTRY_SIZE
		loop	.srchloop
		mov	cx, ENTRY_SIZE * ENTRY_NUM
		lds	dx, [cs:read_buf]
		mov	ax, 0x1229
		int	0x2F

		jc	.readerr
		mov	cx, [cs:num_pkg]
		mov	di, dx
		jmp	.chknum
.readerr:	mov	[cs:result], ax
		ret
.nfound:	mov	word [cs:result], DE_FILENOTFND	; Not found
		stc
		ret
.found:		; Get header offset, header size
		;
		mov	dx, [di+HEADER_OFF]
		mov	cx, [di+HEADER_OFF+2]
		mov	bp, 0x4200
		mov	ax, 0x1228		; Seek
		int	0x2F

		jnc	.readentry
		mov	[cs:result], ax
		ret

.readentry:	lds	dx, [cs:read_buf]

		mov	cx, 130			; Read a header up to 16
						; pointers (more than enough)
		mov	ax, ds
		mov	[cs:tmp_buf+2], ax	; Temporary buf for reading
		mov	ax, dx			; subfunction headers. Locate 
		add	ax, cx			; at the end of pointers
		mov	[cs:tmp_buf], ax

		mov	ax, 0x1229		; read entry
		int	0x2F

		jnc	.ret
		mov	[cs:result], ax
.ret:		ret


end_resident:
; ================== END OF RESIDENT CODE ================================

ERR_SUCCESS	equ	0
ERR_INSTALLED	equ	ERR_SUCCESS
ERR_NOTALLOWED	equ	1
ERR_KERNEL	equ	1
ERR_NOTFOUND	equ	1
ERR_FILEOPEN	equ	1
ERR_FILEREAD	equ	1
ERR_FILEINVLD	equ	1
ERR_INVLDSWITCH	equ	1
ERR_MEMORY	equ	1
ERR_USAGE	equ	1

filename	times 0x100 db 0
fname_len	dw	0
invalid		db	0		; Invalid kernel flag
errlevel	db	ERR_SUCCESS

fake_int2f:	cmp	ax, 0x1400
		je	.i2f1400
		jmp	chain
.i2f1400:	cmp	bx, NLS_FREEDOS_NLSFUNC_VERSION
		je	.testid
		or	byte [cs:invalid], 1
		jmp	chain		; Invalid Kernel NLS version
.testid:	cmp	cx, NLS_FREEDOS_NLSFUNC_ID
		je	.saveptr
		or	byte [cs:invalid], 1
		jmp	chain		; Invalid magic number

		; First, save pointer to kernel NLS info
		;
.saveptr:	mov	[cs:kernel_nls], si
		mov	[cs:kernel_nls+2], ds

		; Check if kernel has a path
		;
		push	cx
		mov	bx, [si]
		mov	cx, [si+2]
		or	bx, bx
		jnz	.found
		or	cx, cx
		jnz	.found

		; Kernel has not a path, either. We use default: "\COUNTRY.SYS"
		;
.notfound:	mov	cx, cs
		mov	bx, default_file
.found:		mov	[cs:country_file], bx
		mov	[cs:country_file+2], cx

		; Get pointers to hardcoded tables
		;
		push	dx
		push	di
		push	es

		les	di, [si+12]		; Hardcoded nls package

		mov	bx, [es:di+10]		; Yes/No characters
		mov	dx, es
		mov	[cs:hc_yesno], bx
		mov	[cs:hc_yesno+2], dx

%if NLSFUNC_VERSION == 0xFD01
		mov	cx, [es:di+12]		; Number of subfunctions
		add	di, 14
%else
		mov	cx, [es:di+14]		; Number of subfunctions
		add	di, 16
%endif


.get_hcinfo:	mov	al, [es:di]
		mov	bx, [es:di+1]
		mov	dx, [es:di+3]

		cmp	al, 1
		jne	.chk_ucase
		mov	bx, [es:di+25]
		mov	dx, [es:di+27]
		mov	[cs:hc_ucase_fn], bx
		mov	[cs:hc_ucase_fn+2], dx
		jmp	.next
.chk_ucase:	cmp	al, 2
		jne	.chk_fucase
		mov	[cs:hc_ucase], bx
		mov	[cs:hc_ucase+2], dx
		jmp	.next
.chk_fucase:	cmp	al, 4
		jne	.chk_fchar
		mov	[cs:hc_fucase], bx
		mov	[cs:hc_fucase+2], dx
		jmp	.next
.chk_fchar:	cmp	al, 5
		jne	.chk_collate
		mov	[cs:hc_fchar], bx
		mov	[cs:hc_fchar+2], dx
		jmp	.next
.chk_collate:	cmp	al, 6
		jne	.chk_dbcs
		mov	[cs:hc_collate], bx
		mov	[cs:hc_collate+2], dx
		jmp	.next
.chk_dbcs:	cmp	al, 7
		jne	.next
		mov	[cs:hc_dbcs], bx
		mov	[cs:hc_dbcs+2], dx
.next:		dec	cx
		jcxz	.exit
		add	di, 5
		jmp	.get_hcinfo
.exit:		pop	es
		pop	di
		pop	dx
		pop	cx

		mov	bx, cx		; Return magic number in BX
		mov	al, 0x01	; Not OK to install
		jmp	i2f14_ret2	; Set carry and return


;;;
;;; PROGRAM START
;;;


start:		push	cs
		pop	ds
		; Say Hello!
		;
		mov	dx, Hello
		mov	ah, 0x09
		int	0x21

		; Check if we are installed
		;
		mov	bx, NLS_FREEDOS_NLSFUNC_VERSION
		mov	cx, NLS_FREEDOS_NLSFUNC_ID
		mov	ax, 0x1400
		int	0x2F

		cmp	al, 0
		je	install
		cmp	al, 0xFF
		je	installed?
		
		mov	si, ErrNotAllowed
		mov	byte [cs:errlevel], ERR_NOTALLOWED
		jmp	quit

installed?:	cmp	bx, NLS_FREEDOS_NLSFUNC_ID
		jne	install
		mov	si, ErrInstalled
		mov	byte [cs:errlevel], ERR_INSTALLED
		jmp	quit

install:	; Set a temporary read buffer
		;  258 bytes needed -> 17 paragraphs
		mov	bx, 17
		mov	ah, 0x48
		int	0x21
		jnc	settmpbuf

		mov	si, ErrMemory
		mov	byte [cs:errlevel], ERR_MEMORY
		jmp	quit

settmpbuf:	mov	word [cs:read_buf], 0
		mov	[cs:read_buf+2], ax

		; Get vect to original int2f handler
		;
		mov     ax, 0x352F
		int     0x21		; get vector to ES:BX
		mov     ax, es
		mov     [cs:old_int2f], bx
		mov     [cs:old_int2f+2], ax

		; Install fake NLSFUNC handler and call int2138 with invalid
		; country to get default parameters from kernel
		;
fromkernel:	mov     ax, 0x252F
		push	cs
		pop	ds
                mov     dx, fake_int2f
                int     0x21

		mov	ax, 0x38FF	; Country code in bx
		mov	bx, 0xFFFE	; Invalid country code
		mov	dx, NLS_DEFAULT	; Set code page
		int	0x21
		;
		; Now [country_file] points to kernel's (or default) value.
		;
		; Restore handler
		;
		push	ds
		mov	ax, 0x252F
		lds	dx, [cs:old_int2f]
                int     0x21
		pop	ds

		; Check if this is a compatible kernel
		;
		test	byte [cs:invalid], 1
		jz	parse

		mov	si, ErrKernel
		mov	byte [cs:errlevel], ERR_KERNEL
		jmp	quit
		

		; Parse command line
		;
parse:		mov	si, 0x80
		xor	cx, cx
		mov	cl, [cs:si]
		inc	si
		cld
.loop1:		mov	al, [cs:si]
		cmp	al, 0x0D
		jne	.cont
		jmp	findfile
.cont:		call	isblank
		jne	.nbfound
		inc	si
		loop	.loop1
		jmp	findfile
.nbfound:	cmp	al, '/'		; switch?
		je	.isswitch
		call	get_filename
.nextloop:	loop	.loop1
		jmp	findfile
.isswitch:	call	parse_switch
		jmp	.nextloop

parse_switch:	inc	si
		dec	cx
		mov	al, [cs:si]
		cmp	al, '?'
		je	.usage
		cmp	al, 'y'
		je	.loadyn
		cmp	al, 'Y'
		je	.loadyn
.invalid:	mov	[cs:ErrInvldSwitch2], al
		mov	si, ErrInvldSwitch
		mov	byte [cs:errlevel], ERR_INVLDSWITCH
		jmp	quit
.usage:		mov	si, Usage
		mov	byte [cs:errlevel], ERR_USAGE
		jmp	quit
.loadyn:	or	word [cs:nls_subfcts], NLS_LOADYESNO
		inc	si
		ret

get_filename:	push	cx
		mov	di, filename
.loop2:		mov	al,[cs:si]
		mov	[cs:di], al
		call	isblank2
		je	.end
		inc	si
		inc	di
		loop	.loop2
.end:		mov	byte [cs:di], 0
		pop	ax
		sub	ax, cx		; Length of filename
		inc	ax		; plus '\0'
		mov	[cs:fname_len], ax
		mov	word [cs:country_file], filename
		mov	ax, cs
		mov	[cs:country_file+2], ax
		ret

isblank2:	cmp	al, '/'
		je	isblank.ret
isblank:	cmp	al, ' '
		je	.ret
		cmp	al, 0x09
		je	.ret
		cmp	al, 0x0A
		je	.ret
		cmp	al, 0x0D
.ret:		ret


		; Test for existance and fail to install if not
		;
findfile:	mov	ax, 0x4E00
		xor	cx, cx
		lds	dx, [cs:country_file]
		int	0x21
		jnc	testfile

		lds	si, [cs:country_file]
		call	printz
		mov	si, ErrNotFound
		mov	byte [cs:errlevel], ERR_NOTFOUND
		jmp	quit

		; Check if it is a valid file
		;
testfile:	mov	ax, 0x3D00	; Open file read-only
		int	0x21
		jnc	chkfileid

		lds	si, [cs:country_file]
		call	printz
		mov	si, ErrFileOpen
		mov	byte [cs:errlevel], ERR_FILEOPEN
		jmp	quit

chkfileid:	mov	bx, ax		; Read from file
		mov	ah, 0x3F
		mov	cx, 128
		lds	dx, [cs:read_buf]
		int	0x21
		jnc	cmpidstr

		lds	si, [cs:country_file]
		call	printz
		mov	si, ErrFileRead
		mov	byte [cs:errlevel], ERR_FILEREAD
		jmp	quit

cmpidstr:	mov	ah, 0x3E		; Close file
		int	0x21
		mov	si, dx
		mov	ax, cs
		mov	es, ax
		mov	di, csys_idstring
		mov	cx, IDSTRING_LEN
		cld
		repe	cmpsb
		jcxz	allocmem

		lds	si, [cs:country_file]
		call	printz
		mov	si, ErrFileInvld
		mov	byte [cs:errlevel], ERR_FILEINVLD
		jmp	quit

		; Allocate memory and mark it as owned by DOS
		; point nls structures to the allocated memory
		;
allocmem:	mov	ax, NLS_MAX_PKGSIZE

		mov	[cs:stack_buffer], ax
		add	ax, 384+15	; 384: Space to save caller (DOS) stack
					;  15: Paragraph calculation
		mov	cl, 4
		shr	ax, cl
		mov	cx, ax
		call	alloc_mem
		jnc	setnlspkg
		mov	si, ErrMemory
		mov	byte [cs:errlevel], ERR_MEMORY
		jmp	quit
setnlspkg:	mov	word [cs:nls_pkg], 0
		mov	[cs:nls_pkg+2], ax
		mov	[cs:stack_buffer+2], ax
		
		mov	dx, [si+11]	; Offset of first entry in file
		mov	cx, [si+13]	;
		mov	[cs:tblidx_off], dx
		mov	[cs:tblidx_off+2], cx
		
                ; Now, install new int2f handler
                ;
	        mov     ax, 0x252F
		push	cs
		pop	ds
                mov     dx, int2f
                int     0x21            ; DS:DX -> new interrupt handler

		;; WARNING
		;; Current kernels have limited COUNTRY= processing support.
		;; In some cases, they load just the country specific
		;; information; in others, they ignore LCASE and DBCS tables.
		;; To ensure that all the info is loaded, we force a
		;; a package load
		;;

		les	si, [cs:kernel_nls]	; Ptr to kernel nls info
		les	si, [es:si+8]		; Ptr to current pkg
		mov	bx, [es:si+4]		; Save country
		mov	word [es:si+4], 0xFFFE	; Set invalid country
		mov	ax, 0x38FF		; Set Country Code (in BX)
		mov	dx, 0xFFFF
		int	0x21
		jnc	.load_success

		; If failed, restore country code
		;
		mov	[es:si+4], bx

		; Point read buffer to PSP (already allocated memory
		; will be freed on exit)
		;
.load_success:	mov	ax, cs
		mov	[cs:read_buf+2], ax
		mov	word [cs:read_buf], 0
		
		; Free some bytes, release environment
		;
		mov	bx, [cs:0x2C]	; Segment of environment
		mov	es, bx
		mov	ah, 0x49	; Free memory
		int	0x21

		; Terminate and stay resident
		;
		pop	bx
		mov	dx, end_resident+15
		mov	ax, [cs:fname_len]
		add	dx, ax
		mov	cl, 4
		shr	dx, cl		; Convert to paragraphs

		mov	ax, 0x3100	; Errorlevel 0
		int	0x21


		; Print message and exit
		;
quit:		push	cs
		pop	ds
		call	printz
		mov	al, [cs:errlevel]	; Set errorlevel
		mov	ah, 0x4C		; Exit
		int	0x21

; CX == Size
; Return: Carry set if failed, clear if success
;         AX: segment or error code
;
alloc_mem:	mov	ax, 0x5802	; get UMB link state
		int	0x21
		mov	[cs:link_state], al	; and remember it for later restore
		mov	ax, 0x5803	; set link state
		mov	bx, 1		;   UMBs are part of memory chain
		int	0x21
		mov	ax, 0x5800	; get allocation strategy
		int	0x21
		mov	[cs:alloc_strat], ax	; and remember it for later restore
		mov	ax, 0x5801	; set allocation strategy
		mov	bx, 0x81	;   try high memory first, best fit
		int	0x21
		mov	ah, 0x48	; allocate memory
		mov	bx, cx		;   this is how much we need
		int	0x21		; try to allocate the UMB
		pushf			; remember whether we succeeded
		push	ax 
		mov	ax, 0x5801	; restore strategy
		db	0xBB		; mov bx
alloc_strat	dw	0
		int	0x21
		mov	ax, 0x5803	; restore link state
		db	0xBB		; mov bx
link_state	db	0, 0
		int	0x21
		pop	ax
		popf			; get back whether we were successful
		ret


printz:		mov	dl, [si]
                or	dl, dl
                jz	.ret
                                                                                
                inc	si
                mov	ah, 0x02
                int	0x21
                                                                                
                jmp	printz
                                                                                
.ret:		ret
		
Hello		db	"FreeDOS NLSFUNC ver. 0.4", 13, 10, '$'
ErrInstalled	db	"NLSFUNC is already installed", 13, 10, 0
ErrNotAllowed	db	"NLSFUNC is not allowed to install", 13, 10, 0
ErrKernel	db	"NLSFUNC: Incompatible kernel version", 13, 10, 0
ErrNotFound	db	": File not found", 13, 10, 0
ErrFileOpen	db	": Error opening file", 13, 10, 0
ErrFileRead	db	": Error reading from file", 13, 10, 0
ErrFileInvld	db	": Invalid COUNTRY.SYS file format", 13, 10, 0
ErrInvldSwitch	db	"Invalid switch: /"
ErrInvldSwitch2	db	0 , 13, 10, 0
ErrMemory	db	"Not enough memory", 13, 10, 0
Usage		db	13, "FreeDOS NLSFUNC. Adds NLS (National Language "
		db	"Support) functionality.", 13, 10
		db	13, "(C) 2004 Eduardo Casino, under the terms of the "
		db	"GNU GPL, Version 2", 13, 10, 10
		db	"Syntax:", 13, 10, 10
		db	"  NLSFUNC [/Y|/?] [[D:][PATH]FILE]", 13, 10, 10
		db	"Options:", 13, 10, 10
		db	"  /Y              - Loads (optional) YES/NO table"
		db	13, 10
		db	"  /?              - Prints usage", 13, 10
		db	"  [D:][PATH]FILE  - Path to a file containing the NLS"
		db	" information.", 13, 10, 0

csys_idstring	db	0xFF, "COUNTRY"
IDSTRING_LEN		equ	$ - csys_idstring

		EXE_end
