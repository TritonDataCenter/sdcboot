;------ nansi_i.asm ----------------------------------------------
; Contains code only needed at initialization time.
; Copyright (C) 1986,1999 Daniel Kegel
; Updated 2003, 2007 by Eric Auer
;
; This program is free software; you can redistribute it and/or
; modify it under the terms of the GNU General Public License
; as published by the Free Software Foundation; either version 2
; of the License, or (at your option) any later version.
;
; This program is distributed in the hope that it will be useful,
; but WITHOUT ANY WARRANTY; without even the implied warranty of
; MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
; GNU General Public License for more details.
;
; You should have received a copy of the GNU General Public License
; along with this program; if not, write to the Free Software
; Foundation, Inc., 51 Franklin Street, 5th Floor, Boston, MA 02110-1301 USA
; or look on the web at http://www.gnu.org/copyleft/gpl.html.
;
; Modifications:
; 2/12/85: Removed code for BIOS takeover, added processor check
; 2/13/85: changed processor check to PUSH SP
; 6/10/90: Deleted processor check, deleted output translation table
; Revision 1.9  91/04/07  21:46:34  dan_kegel
;   Added /s option for security.  Disables keyboard redefinition.
; Revision 1.8  91/04/07  18:23:04  dan_kegel
;   Now supports /t commandline switch to tell nansi about new text modes.
; Revision 1.7  91/04/06  21:38:00  dan_kegel
; 11/20/03: Added /R /B and /P commandline switch parsing. Enforces
;   BIOS char output, BIOS beep and "chain if unknown", respectively.
;   /P switch additionally triggers extra initialization code.
; 05/23/07: Added /Q option for quiet bell
;
;-----------------------------------------------------------------

	; to nansi.asm
	public	dosfn0			; must be FIRST in this part!

	; from nansi.asm
	extrn	dev_info_word:word	; *** our device attributes
	extrn	break_handler:near
	extrn	int_29:near
	extrn	int_2F:near
	extrn	old2fofs:word		; old int 2f pointer
	extrn	old2fseg:word		; old int 2f pointer
	extrn	req_ptr:dword
	extrn	is_ega:byte
	extrn	kbd_read_func:byte	; 0/1 or 11h - func to read keys
	extrn	textmode_table:byte	; 256 bit array of video modes
	extrn	bios_putchar:byte	; *** flag to force BIOS putchar
	extrn	bios_bell:byte		; *** flag to force BIOS or silent bell
	extrn	chain_handlers:byte	; *** flag to enable chaining
	extrn	prev_handler:dword	; *** pointer to chained handler
	extrn	prev_handler_o:word	; *** pointer to chained handler
	extrn	prev_handler_s:word	; *** pointer to chained handler

	; from nansi_p.asm
	extrn	param_buffer:word	; adr of first byte free for params
	extrn	param_end:word		; adr of last byte used for params
	extrn	redef_end:word		; adr of last used byte for redefs

	; from nansi_f.asm
	extrn	ibm_kkr:byte

	; for checking init overrun by comparing DOSFN0 to DOSFN0_END
	; in load map
	public	dosfn0_end

	; for debugging
	public	arg_s

code	segment byte public 'CODE'
	assume	cs:code, ds:code

;-------- dos function # 0 : init driver ---------------------
; Initializes device driver interrupts and buffers, then
; passes ending address of the device driver to DOS.
; Since this code is only used once, the buffer can be set up on top
; of it to save RAM.

dosfn0	proc	near
	; *** this label is at the same time the start of the       ***
	; *** param_buffer space (512+5 bytes) which is overwritten ***
	; *** later (from the end) by storing keyboard redef's...   ***

	; Get pointer to commandline options in ES:SI.
	call	near ptr d0_get_dev_args

	; See whether this computer has the new BIOS calls
	; *** now done BEFORE processing args which may override it ***
	call	near ptr detect_extbios

	; Process arguments in ES:SI.
	; Sets variables textmode_table[], and nukes ibm_kkr.
	; *** sets other flags, too, when it detects   ***
	; *** the corresponding command line switches. ***
	call	near ptr d0_process_args

	; Print out 'installed' message.
	mov	dx, offset install_msg
	mov	ah, 9
	int	21h

	;-----
	xor	ax, ax
	mov	ds, ax
	pushf
	cli
	; Install BIOS keyboard break handler. (*** why that? ***)
	mov	bx, 6Ch	; 1bh * 4
	mov	word ptr [BX],offset break_handler
	mov	[BX+02], cs
	; Install INT 29 quick putchar.
	mov	bx, 0a4h ; 29h * 4
	mov	word ptr [bx], offset int_29
	mov	[bx+2], cs
	; Install INT 2F install check
	mov	bx, 0bch ; 2fh * 4
	push	word ptr [bx]	; old2fofs
	push	word ptr [bx+2]	; old2fseg
	mov	word ptr [bx], offset int_2F
	mov	[bx+2], cs

	push	cs
	pop	ds
	push	cs
	pop	es			; es=cs so we can use stosb

	pop	old2fseg
	pop	old2fofs
	popf

	cld				; make sure stosb increments di

	; Test presence of EGA or better by calling BIOS fn 12h.10h.
	mov	ah, 12h
	mov	bx, 0ff10h
	int	10h			; bh=0-1, bl=0-3 if EGA
	xor	ax, ax
	test	bx, 0FEF0H
	jnz	got_ega_status		; sorry, charlie
		inc	ax		; Yippee- it's an EGA or better.
got_ega_status:
	mov	is_ega, al		; save boolean variable
	
	; Calculate addresses of start and end of parameter/redef buffer.
	; The buffer occupies the same area of memory as this code!
	; ANSI parameters are accumulated at the lower end, and
	; keyboard redefinitions are stored at the upper end; the variable
	; param_end is the last byte used by params (changes as redefs added);
	; redef_end is the last word used by redefinitions.
	mov	di, offset ibm_kkr
	mov	al, [di]		; are redefinitions possible?
	cmp	al, nearret
	jnz	larger_buffer		; else... we can overwrite part of
	inc	di			; (but not the first byte of...)
	mov	param_buffer, di	; the lookup handling code and use
	add	di, 128			; a smaller parameter buffer
	mov	param_end, di		; addr of last byte in free area
	mov	redef_end, di		; param_end == redef_end stops lookup
	jmp	defined_buffer
; param_buffer ... param_end is buffer for ANSI ESC parser
; param_end ... redef_end is buffer for keyboard redefinitions
; when redefinitions grow, space for parser shrinks.
larger_buffer:
	mov	di, offset dosfn0
	mov	param_buffer, di	; *** buffer overwrites dosfn0 ***
	add	di, 512			; *** buffer is 512 (+5) bytes ***
	mov	param_end, di		; addr of last byte in free area

; ??	push	di
	; *** only 5 bytes at dosfn0 + 512 are initialized now, ***
	; *** but be warned: They overwrite stuff!              ***
	; Default redefinition table: control-printscreen -> control-P
	mov	al, 16		; control P
	stosb
	mov	ax, 7200h	; control-printscreen
	stosw
	mov	ax, 1		; length field
	mov	redef_end, di	; address of last used word in table
	stosw
; ??	pop	di

defined_buffer:
	inc	di
	; *** Everything after dosfn0 start label can be wiped  ***
	; *** after we returned from this dosfn0 call.          ***
	; *** If ibm_kkr is disabled, even more can be wiped... ***
	; Return ending address of this device driver.
	; (If ax=di=FFFF, DOS would refuse to load us.)
	xor	ax, ax
	lds	si, req_ptr
	mov	word ptr [si+0Eh], di
	mov	[si+10h], cs

	; Return "not busy" exit status.
	ret			; jump back to int_done, status AX
				; (does POPs and retf)

install_msg	db	'NANSI.sys 4.0d'
	db                      ': New ANSI driver. Copyright 1986-2007 Daniel Kegel and others', 13, 10
		; -1999 by Dan Kegel, plus a small 2003 update by Eric Auer.
		; and a 1 line patch 2005/2006, removing "and dev_info,0ffbfh"
	db	9,'http://www.kegel.com/nansi/  <dank at kegel.com>', 13, 10
	db	9,'This is free software.  It comes with ABSOLUTELY NO WARRANTY.', 13, 10
	db	9,'You are welcome to redistribute it under the terms of the', 13, 10
	db	9,'GNU Public License; see http://www.gnu.org/ for details.', 13, 10, '$'

dosfn0	endp


;--------------------------------------------------------------------------
; Get pointer to device arguments in ES:SI.
; On entry and exit, DS=CS.
; Trashes AX, CX, DI.
d0_get_dev_args	proc	near
	les	si, req_ptr			; es:si = BPB/arg ptr ptr
	les	si, es:[si+12h]			; es:si = arg ptr

	; DOS 2 passes a pointer to the DEVICE= line, with a null before the
	; args; DOS 3 passes a pointer to the char after the =.
	cld
	mov	ah, 30h
	int	21h				; get dos version number to AL
	cmp	al, 2
	jnz	gda_ret
		mov	di, si
		mov	cx, 256
		mov	al, 0
		repnz	scasb			; skip to arguments field
		mov	si, di
gda_ret:
	ret
d0_get_dev_args	endp

;--------------------------------------------------------------------------
; Process arguments in ES:SI.
; On entry, DS=CS, ES:SI = argument string.
; Trashes AX,BX,CX,DX,SI.
d0_process_args	proc	near
	; Skip to first blank char (skip past driver filename).
arg_bloop:	call	d0_getchar
		jz	arg_done_0
		call	isspace
		jz	arg_loop
		jmp	short arg_bloop

arg_done_0:	jmp	arg_done
arg_bad_0:	jmp	arg_bad

arg_loop:
		; Skip to next nonblank char.  Exit if end of args.
		call	d0_getchar
		jz	arg_done_0
		call	isspace
		jz	arg_loop

		; We are at start of an argument.  (Are your hackles up?)
		; All legal arguments start with a forward slash and a letter.
		cmp	al, '/'
		jnz	arg_bad_0
		call	d0_getchar
		jz	arg_bad_0
		; Handle each possible option
		CMP	AL,'X'
		JNZ	arg_not_x
		; /X option allows distinguishing normal from keypad keys
		; ... and takes no arguments
		AND	kbd_read_func,0feh	; *** trigger code 0e0h
						; *** special treatment!
		MOV	DX,offset xopt_msg
		MOV	AH,9
		INT	21h
		JMP	arg_loop
arg_not_x:
		CMP	AL,'C'
		JNZ	arg_not_c
		; Switch /C forces 101+ key keyboard mode
		OR	kbd_read_func,10h	; *** ENable ext. keyboard
		MOV	DX,offset copt_msg
		MOV	AH,09
		INT	21h
		JMP	arg_loop
arg_not_c:
		CMP	AL,'K'
		JNZ	arg_not_k
		; As DOS kernel SWITCHES=/K this forces 84 key keyboard mode
		AND	kbd_read_func,0e0h	; *** DISable ext. keyboard
		MOV	DX,offset kopt_msg
		MOV	AH,09
		INT	21h
		JMP	arg_loop
arg_not_k:
		cmp	al, 'S'
		jnz	arg_not_s
arg_s:		; /S option (security) takes no argument
		; it simply disables the keyboard redefinition sequence
		mov	dx, offset kkr_off_msg
		mov	ah, 9
		int	21h
		mov	al, nearret
		mov	ibm_kkr, al
		jmp	arg_loop

arg_not_s:	cmp	al, 'T'
		jnz	arg_not_t
		; /T option takes a hexadecimal argument
		call	d0_get_hex
		jc	arg_not_t

		; Video mode AL is a text mode.
		; Set the corresponding bit in textmode_table.
		mov	cl, al
		shr	al,1
		shr	al,1
		shr	al,1
		cbw
		add	ax, offset textmode_table
		xchg	ax, bx
		and	cl, 7
		mov	al, 1
		shl	al, cl
		or	byte ptr [bx], al

		jmp	arg_loop

arg_not_t:	cmp	al, 'R'
		jnz	arg_not_r
arg_r:		; /R option (BIOS putchar) takes no argument
		; it enforces BIOS putchar (mode classification trick)
		mov	dx, offset ropt_msg
		mov	ah, 9
		int	21h
		mov	al, 1
		mov	bios_putchar, al
		jmp	arg_loop

arg_not_r:	cmp	al, 'B'
		jnz	arg_not_b
arg_b:		; /B option (BIOS bell) takes no argument
		; it enforces BIOS bell instead of lowlevel bell
		mov	dx, offset bopt_msg
		mov	ah, 9
		int	21h
		mov	al, 1
		mov	bios_bell, al
		jmp	arg_loop

arg_not_b:	cmp	al, 'Q'
		jnz	arg_not_q
arg_q:		; /Q option (quiet bell) takes no argument
		; it silences the bell
		mov	dx, offset qopt_msg
		mov	ah, 9
		int	21h
		mov	al, -1
		mov	bios_bell, al
		jmp	arg_loop

arg_not_q:	cmp	al, 'P'
		jnz	arg_not_p
arg_p:		; /P option (chain) takes no argument
		; it enables forwarding / chaining of device requests
		; which NANSI cannot handle to other CONs which can.
		mov	dx, offset popt_msg
		mov	ah, 9
		int	21h
		mov	al, 1
		mov	chain_handlers, al
		; Find pointer to current CON, the one which NANSI
		; will replace, and store it. DOS 2.0+
		; *** Aitor adds: This is NOT necessarily CON. It is
		; *** only the current STDIN.
		push	bx
		push	cx
		push	es
		mov	ah, 52h		; get list of lists -> ES:BX
		int	21h
		les	bx, es:[bx+0ch]	; pointer to device header CON
		; *** Aitor tells: This is only STDIN, not always CON!
		mov	ax,es		; store pointer in prev_handler
		mov	ds:prev_handler_o, bx
		mov	ds:prev_handler_s, ax
		mov	ax, es:[bx+4]	; device info word of that one
; WRONG: 40h is Generic IOCTL which we DO want to chain, not eof-on-input
; Thanks to Eduardo Casino for pointing that out :-) (patched 2006)
; wrong:	and	ax,0ffbfh	; ...
		or	ds:dev_info_word, ax	; we now "can do", too
		; Bits: CIB? O??x 1GRT cnoi, where C=char, I=ioctl,
		; B=output-till-busy, ?=unused O=open/close - FreeDOS
		; kernel never uses that, x="set by keyb",
		; G="Generic IOCTL" - MS ANSI has that plus IOCTL plus
		; the NANSI features), R=rawmode (no cr/lf/eof stuff),
		; T=fasttty (int29h), c=clock$ n=nul o=stdout, i=stdin
		mov	ax, es:[bx+10]	; name string of other driver
		cmp	ax, ds:[10]	; our name string
		jnz	chain_no_con
		mov	ax, es:[bx+12]
		cmp	ax, ds:[12]	; stupid: Arrowsoft ASM would
					; "cmp ax,12" without the DS:
		jnz	chain_no_con
		mov	ax, es:[bx+14]
		cmp	ax, ds:[14]
		jnz	chain_no_con
		mov	ax, es:[bx+16]
		cmp	ax, ds:[16]
		jnz	chain_no_con
		jmp	short chain_ok
		;
		; else, the current STDIN is no real CON, and we could
		; (DOS 3.1+) surf the device chain starting with NUL
		; in "list of lists" to find the real con.
chain_no_con:	mov	dx, offset no_con_msg
		mov	ah, 9		; write string to STDOUT
		int	21h
		add	bx,10		; point to name of the "non CON stdin"
		mov	cx,8		; string length
chain_x_lp:	mov	dl,es:[bx]	; fetch 1 char (might be "$")
		inc	bx
		mov	ah, 2		; write 1 char to STDOUT
		int	21h
		loop	chain_x_lp
		mov	dx, offset crlf_msg
		mov	ah, 9
		int	21h
		;
chain_ok:	pop	es
		pop	cx
		pop	bx
		jmp	arg_loop

arg_not_p:

arg_bad:	; Otherwise it was an illegal option.
		mov	dx, offset arg_bad_msg
		mov	ah, 9
		int	21h
arg_done:
nearret	label	byte	; for copying near return opcode (brain surgery)
	ret

arg_bad_msg	db "NANSI.SYS: Possible options are /S /X /K /C /R /B /Q /P /Tnn", 13, 10
		db 9,"/S Safe mode (disable keyboard redefinition),", 13, 10
		db 9,"/X treat keypad separately /K force 84 key mode /C 101+ key mode", 13, 10
		db 9,"/R BIOS slow compatible mode /B BIOS beep /Q no beep /P chain ioctl", 13, 10
		db 9,"/Tnn make NANSI treat mode nn (2 hexadecimal digits) as text mode", 13, 10
		db 9,"Example: NANSI.SYS /S /B /T0b /T4f /T51 /T52", 13, 10, "$"

kkr_off_msg	db "NANSI.SYS /S: Safe mode - disabling keyboard redefinition."
		db 13, 10, "$"
xopt_msg	db "NANSI.SYS /X: Recognize keypad keys independently."
		db 13, 10, "$"
kopt_msg	db "NANSI.SYS /K: Force 84 key keyboard behaviour."
		db 13, 10, "$"
copt_msg	db "NANSI.SYS /C: Force 101+ key keyboard behaviour."
		db 13, 10, "$"
ropt_msg	db "NANSI.SYS /R: Force BIOS calls for putchar."
		db 13, 10, "$"
bopt_msg	db "NANSI.SYS /B: Force BIOS calls for beeping."
		db 13, 10, "$"
qopt_msg	db "NANSI.SYS /Q: Ignore beep / bell character."
		db 13, 10, "$"
popt_msg	db "NANSI.SYS /P: Enable chaining to other drivers."
		db 13, 10, "$"
no_con_msg	db "NANSI.SYS /P: The stdin driver to which we chained was no"
		db 13, 10, "full CON - please report. Detected driver: $"
crlf_msg	db 13,10,"$"

d0_process_args	endp

;--------
; ES:SI points to next arg char.  Return next char in AL, Z=EOLN.
d0_getchar:
	lods	byte ptr es:[si]
	cmp	al, 'a'
	jb	d0gc_nlc
	cmp	al, 'z'
	ja	d0gc_nlc
		sub	al, 'a'-'A'	; convert from lower to uppercase
d0gc_nlc:
	cmp	al, 13
	jz	d0gc_done
	cmp	al, 10
d0gc_done:
	ret

;--------
; Return with Z set if char in AL is a blank or a tab.
isspace:
	cmp	al, 20h
	jz	isspace_ret
	cmp	al, 9
isspace_ret:
	ret

;-------- d0getnib
; Get a char from ES:[SI++]; convert it to hex in AL.
; Returns char in DL, nibble in AL, flags 'above' if char not a valid hex char.
; Preserves all other registers.
d0getnib:
	call	d0_getchar
	mov	dl, al		; save char in DL
	; is it in range '9'+1 ... 'A'-1?
	cmp	al, 'A'
	jae	d0gn_ok
	cmp	al, '9'
	ja	d0gn_ret	; '9' < c < 'A' means char is not hex.
d0gn_ok:
	sub	al, '0'
	cmp	al, 9
	jbe	d0gn_0
		sub	al, 'A' - '0' - 10
d0gn_0:
	cmp	al,0fh		; above means c < '0' or c > 'F'
d0gn_ret:
	; flags set to "above" if char could not be converted to hex
	ret

;--------
; d0_get_hex
; Get a two-digit hexadecimal number & convert to a byte in AL.
; If any char is not hex, print error message and return with carry set.

d0_get_hex:
	call	d0getnib	; get lower nibble
	ja	d0gh_bogus
	shl	al, 1
	shl	al, 1
	shl	al, 1
	shl	al, 1
	xchg	ah, al
	call	d0getnib	; get upper nibble
	ja	d0gh_bogus
	or	al, ah		; AL = byte
	; Return with carry clear.
	clc
d0gh_ret:
	ret
d0gh_bogus:
	push	dx
		; Print out error message.
		mov	ah, 9
		mov	dx, offset gh_errmsg
		int	21h
	; Print out offending char and CRLF.
	pop	dx
	mov	ah, 2
	int	21h
	call	d0_put_crlf
	; Return with carry set.
	stc
	jmp	d0gh_ret

gh_errmsg	db	"NANSI.SYS: Expected two-digit hexadecimal number in CONFIG.SYS"
		db	", but got $"

;--------------------
; Print a hexadecimal number in AL to the console.
d0_put_hex:
	call	d0_put_nib
d0_put_nib:
	ror	al, 1
	ror	al, 1
	ror	al, 1
	ror	al, 1
	mov	dl, al
	and	dl, 0fh
	add	dl, '0'
	cmp	dl, '9'
	jbe	d0_pn_ok
		add	dl, 'A' - '9' - 1
d0_pn_ok:
	push	ax
	mov	ah, 2
	int	21h
	pop	ax
	ret

;---------
; Print a newline.
d0_put_crlf:
	mov	ah, 2
	mov	dl, 13
	int	21h
	mov	ah, 2
	mov	dl, 10
	int	21h
	ret

; Call once during initialization.  Sets the 10h bit of kbd_read_func if
; this computer supports the new keyboard BIOS calls 10h/11h.
; *** Changed to simpler and better FreeDOS kernel 2032 console.asm ***
; *** detection style: if 40h:[96h] test 10h NZ, kbd. is extended!  ***
detect_extbios:
	push	ds
	push	ax
	mov	ax,40h
	mov	ds,ax
	mov	al,ds:[96h]	; keyboard flags
	test	al,10h		; extended keyboard (101+ keys)?
	pop	ax
	pop	ds
	jz	detected_oldbios
	or	kbd_read_func,10h	; we will use func 10h/11h
detected_oldbios:
	ret

; *** Old version of detect_extbios: ***
	if 0	; *** ***
detect_extbios:
	TEST kludge1_00C3,10h
	JNZ kludge1_0D69
	; Jam special test code into keyboard buffer
	MOV AH,5
	MOV CX,0FFFFh
	INT 16h
	; Try to read special test code from keyboard buffer
	MOV CX,10h
kludge1_0D56:
	; Get keystroke using extended keyboard read
	MOV AH,10h
	INT 16h
	; Is it our special test code?
	CMP AX,0FFFFh
	; Yes; Set flag saying that the extended keyboard BIOS is supported
	JZ  kludge1_0D64
	LOOP kludge1_0D56
	JMP kludge1_0D69
kludge1_0D64:
	OR  kludge1_00C3,10h
kludge1_0D69:
	RET
	endif	; *** ***

; *** check the map file - everything after the start of dosfn0   ***
; *** is used for data or does not even stay resident after init! ***

dosfn0_end:

code	ends
	end				; of nansi_i.asm
