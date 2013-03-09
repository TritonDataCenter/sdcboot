;--- nansi.asm ----------------------------------------------------------
; New ANSI terminal driver.
; Optimized for speed in the case of multi-character write requests.
; Copyright (C) 1986, 1999 Daniel Kegel
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
; The following files make up the driver:
;	nansi.asm   - all DOS function handlers except init
;	nansi_p.asm - parameter parser for ANSI escape sequences
;	nansi_f.asm - ANSI command handlers
;	nansi_i.asm - init DOS function handler
; Parts should be linked in that order. The _i part does not stay in RAM.
;
; Daniel Kegel, Bellevue, Washington & Pasadena, California
; Revision history:
; 5  july 85: brought up non-ANSI portion except forgot backspace
; 6  july 85: split off ANSI stuff into other files, added backspace
; 11 july 85: fixed horrible bug in getchar; changed dosfns to subroutines
; 12 july 85: fixed some scrolling bugs, began adding compaq flag
; 9  aug 85:  added cursor position reporting
; 10 aug 85:  added output character translation
; 11 aug 85:  added keyboard redefinition, some EGA 80x43 support
; 10 sept 85: Tandy 2000 support via compaq flag (finding refresh buffer)
; 30 Jan 86:  removed Tandy 2000 stuff, added graphics mode support
; 12 feb 86:  added int 29h handler, added PUSHA/POPA, added direct beep,
;             direct cursor positioning, takeover of BIOS write_tty,
;	      noticed & squashed 2 related bugs in tab expansion
; 13 feb 86:  Squashed them again, harder
; 7 jun 90:   Removed horrible bug in nansi_f.asm causing funny background
;	      Restored nansi_i.asm to match 'classic' 2.2- no check for
;	      80186 stack instructions
; 10 jun 90:  Deleted output char translation
; 11 jun 90:  Amazing.  AH was always zero on entry, in DOS 2.0 thru 3.31.
;	      Otherwise nansi would have crashed; there was a cbw missing.
; Revision 1.5  90/07/03  21:33:54  dank
;   Changed version number to 3.0
; Revision 1.6  90/07/09  23:59:55  dan_kegel
;   Slight change to doc file; lists contents of archive, specifies name, etc.
; Revision 1.7  91/04/06  21:10:19  dan_kegel
;   1. New address.  2. Mode 99 is graphics cursor on/off.
;   Even with graphics cursor off, use BIOS routine to set cursor location
;   in graphics modes.
; Revision 1.8  91/04/07  18:22:17  dan_kegel
;   Now supports /t commandline switch to tell nansi about new text modes.
; Revision 1.9  91/04/07  21:46:01  dan_kegel
;   Added /s option for security.  Disables keyboard redefinition.
; 20 nov 03: Added processing for /R /B and /P command line switches:
;   /R forces using BIOS for char output, /B forces using BIOS for the bell
;   and /P enables the "chain if unknown function" feature.
; 23 may 07: Added int 2f handler, is_g_override, /Q "quiet bell".
;
;------------------------------------------------------------------------

	; from nansi_f.asm
	extrn	f_escape:near, f_in_escape:near
	extrn	ibm_kkr:byte

	; from nansi_p.asm
	extrn	param_end:word, redef_end:word

	; from nansi_i.asm
	extrn	dosfn0:near

	; to nansi_p.asm
	public	f_loopdone
	public	f_not_ansi
	public	f_ansi_exit

	; to both nansi_p.asm and nansi_f.asm
	public	cur_x, cur_y, max_x, cur_attrib
	public	get_max_y

	; to nansi_f.asm
	public	pseudo_flag
	public	xy_to_regs, get_blank_attrib
	public	port_6845
	public	wrap_flag
	public	cur_parm_ptr
	public	cur_coords, saved_coords, max_y
	public	escvector, string_term
	public	cpr_esc, cprseq
	public	video_mode
	public	lookup
	public	lookup_skipfix	; was "lookup_04c5"
	public	in_g_mode
	public	is_g_override

	; to nansi_i.asm
	public	dev_info_word	; *** /P
	public	req_ptr, break_handler
	public	int_29		; DOS fast TTY handler
	public	int_2F		; install check etc
	public	old2fofs	; old int 2f handler
	public  old2fseg	; old int 2f handler
	public	is_ega		; EGA or newer (can do > 25 lines)
	public	kbd_read_func	; was "kludge1_00c3"
	public	textmode_table	; Array of extra text mode numbers
	public	bios_putchar	; *** /R
	public	bios_bell	; *** 0 default, 1 /B and -1 /Q
	public	chain_handlers	; *** /P
	public	prev_handler	; *** /P
	public	prev_handler_o	; *** /P
	public	prev_handler_s	; *** /P

	; debugging
	public	driver_header

;--- push_all, pop_all ------------------------------------------------
; Save/restore all user registers.
push_all	macro
	push	ax
	push	bx
	push	cx
	push	dx
	push	bp
	push	si
	push	di
	endm

pop_all	macro
	pop	di
	pop	si
	pop	bp
	pop	dx
	pop	cx
	pop	bx
	pop	ax
	endm

keybuf	struc				; Used in getchar
len	dw	?
adr	dw	?
keybuf	ends

; ---------------------------------------------------------

ABS40	segment at 40h
	org	1ah
buffer_head	dw	?	; Used in 'flush input buffer' dos call.
buffer_tail	dw	?

	org	49h
crt_mode	db	?
crt_cols	dw	?
crt_len		dw	?
crt_start	dw	?
cursor_posn	dw	8 dup (?)
cursor_mode	dw	?
active_page	db	?
addr_6845	dw	?	; used to reposition / resize cursor
				; *** unless /R option (BIOS putchar) used
crt_mode_set	db	?	; = 7 only if monochrome display adaptor
crt_palette	db	?
	org	6ch
timer_low	dw	?	; low word of time-of-day counter (18.2 hz)
				; *** used for beep unless /B option used
	org	84h
bios_max_y	db	?	; only if is_ega=1 (EGA/newer: visible lines)
bios_char_h	db	?	; character height, if EGA/newer
		db	?	; (upper half of char height)

ABS40	ends

; ---------------------------------------------------------

CODE	segment byte public 'CODE'
assume	cs:code, ds:code

	; Device Driver Header

	; org	0
driver_header:
	dd	-1			; next device
dev_info_word:
	dw	8013h			; attributes: char, int29, stdin, stdout
					; ("device information word")
	; *** for comparison: MS ANSI has 0c053h - in addition to our features,
	; *** it supports IOCTL (8000h) and "Generic IOCTL" (40h, some docs
	; *** erroneously call this "eof-on-input"...)
	dw	strategy		; request header pointer entry
	dw	interrupt		; request entry point
	db	'CON     '		; device name (must be 8 char long)

	; Gadget: Identification- in case somebody TYPEs the assembled driver,
	; it will clear screen just before the install_msg is TYPEd.
; ---	db	27, '[2J', 13, 10

	db	13,10,"This is the NANSI.sys device driver. Read NANSI.DOC."
	db	13,10,26	; EOF

; ---	moved install_msg to non resident part for 4.0
; ---	db	8, 32, 26		; Gadget: 26 is EOF char (for TYPE).

;----- variable area --------------------
req_ptr	label	dword
req_off	dw	?
req_seg	dw	?

pseudo_flag	db	0	; 1 = simulate cursor in graphics modes
wrap_flag	db	1	; 0 = no wrap past line end
escvector	dw	0	; state vector of ESCape sequencor
video_mode	db	3	; ROM BIOS video mode (2=BW, 3=color)
max_y		db	24
max_cur_x	label	word	; used to get both max & cur at once
max_x		db	79	; line width (79 for 80x25 modes)
cur_coords	label	word
cur_x		db	0	; cursor position (0 = left edge)
cur_y		db	0	;		  (0 = top edge)
saved_coords	dw	?	; holds XY after a SCP escape sequence
string_term	db	0	; either escape or double quote
cur_attrib	db	7	; current char attributes
cur_page	db	0	; current display page
video_seg	dw	?	; segment of video card
f_cptr_seg	dw	?	; part of fastout write buffer pointer
cur_parm_ptr	dw	?	; last byte of parm area now used
port_6845	dw	?	; port address of 6845 card
is_ega		db	?	; =0 if not, =1 if so.

kbd_read_func	db  1		; 1 (84 key keyboard) or 10h/11h (new)
	; low bit being 0 means "treat keypad and numpad differently"
	; and activates "keyE0fixup" usage. High 4 bits are simply the
	; high 4 bits of the AH used for int 16h calls.

bios_putchar	db	0	; *** 1 to force BIOS putchar
bios_bell	db	0	; *** 1 to force BIOS bell -1 for silence
chain_handlers	db	0	; *** 1 to enable "chaining"
prev_handler	label dword	; *** pointer to previous CON
prev_handler_o	dw	0	; *** pointer offset
prev_handler_s	dw	0 	; *** pointer segment

brkkeybuf	db	3	; control C
fnkeybuf	db	?	; holds second byte of fn key codes
cpr_buf		db	9 dup (?), '['	; make it 9 in case monster display
cpr_esc		db	1bh	; descending buffer for cpr function

; following table holds a bitmap of video modes; text modes have their bit set.
; Modes 0,1,2,3 and 7 are always text; 4, 5, 6 are always graphics.
; Other modes vary from board to board.
; This table can be set from the commandline when loading NANSI (see nansi_i).
textmode_table	db	8fh, 31 dup (0)

; following four keybufs hold information about input
; Storage order determines priority- since the characters making up a function
; key code must never be separated (say, by a Control-Break), they have the
; highest priority, and so on.  Keyboard keys (except ctrl-break) have the
; lowest priority.

fnkey	keybuf	<0, fnkeybuf>	; fn key string (0 followed by scan code)
cprseq	keybuf	<0>		; CPR string (ESC [ y;x R)
brkkey	keybuf	<0, brkkeybuf>	; ^C
xlatseq	keybuf	<0>		; keyboard reassignment string

;---- get_max_y ------------------------------------------------
; If EGA, reads BIOS's idea of max_y; else assumes 24.
; Stores result in max_y, and returns it in AL also.
; Trashes AH.
; No other registers affected.
; No assumptions made about DS or ES.
get_max_y	proc	near
	cmp	cs:is_g_override,0
	jz	real_max_y
	mov	al, cs:max_y		; just stick to current max_y
	ret
real_max_y:
	mov	al, 24
	; Are we using an EGA?
	cmp	cs:is_ega, 1
	jnz	get_max_y_done
		; Yes.  Read max_y from BIOS data area.
		mov	ax, abs40
		push	ds
		mov	ds, ax
		assume	ds:abs40
		mov	al,bios_max_y	; al = max_y if not ancient PC
		assume	ds:nothing
		pop	ds
get_max_y_done:
	mov	cs:max_y, al
	ret
get_max_y	endp


;------ xy_to_regs --------------------------------------------
; on entry: x in cur_x, y in cur_y
; on exit:  dx = chars left on line, di = address
; Alters ax, bx.
xy_to_regs	proc	near
	; Find number of chars 'till end of line, keep in DX
	mov	ax, max_cur_x
	mov	bx, ax			; save max_x & cur_x for next block
	mov	ah, 0			; ax = max_x
	xchg	dx, ax
	mov	al, bh
	mov	ah, 0			; ax = cur_x
	sub	dx, ax
	inc	dx			; dx is # of chars till EOL
	; Calculate DI = current address in text buffer
	mov	al, bl			; al = max_x
	inc	al
	mul	cur_y
	add	al, bh			; al += cur_x
	adc	ah, 0			; AX is # of chars into buffer
	add	ax, ax
	xchg	di, ax			; DI is now offset of cursor.
	ret
xy_to_regs	endp


;------- dos_fn_tab -------------
; This table is used in "interrupt" to call the routine that handles
; the requested function. 0 is init, (1 block devices), (2 block dev),
; *3 IOCTL*, 4 read, 5 peek, 6 input_status (nothing to peek -> busy),
; 7 flush input buffers, 8 write, 9 write-verify,
; 10 get out status (MS kernel requires this), 11 flush output buffers
; *** chaining option will let IOCTL of previous CON shine through,
; *** as well as "get out status" and "flush out buffers" and all
; *** functions beyond 0bh/11. Functions 1/2, too, if anybody cares.

max_cmd	equ	16
dos_fn_tab label word	; *** dosfn0 becomes nopcmd after 1. usage...
d_f_t:	dw	dosfn0, nopcmd, nopcmd, badcmd	; 03 is IOCTL read
	dw	dosfn4, dosfn5, dosfn6,	dosfn7
	dw	dosfn8, dosfn8, nopcmd, nopcmd	; 0a is get out status
	; *** should we do "get out status" ourselves even in /P mode?
	dw	badcmd, nopcmd, nopcmd, nopcmd	; 0c is IOCTL write
	; 0d / 0e are open / close, (0f block dev), 10 out until busy
	dw	dosfn8	; 10h is output until busy: works like output
	; but can return a smaller char count than it was called with


;------- strategy ----------------------------------------------------
; DOS calls strategy with a request which is to be executed later.
; Strategy just saves the request.

strategy	proc	far
	mov	cs:req_off,BX
	mov	cs:req_seg,ES
	ret
strategy	endp

;------ interrupt -----------------------------------------------------
; This is where the request handed us during "strategy" is
; actually carried out.
; Calls one of 12 subroutines depending on the function requested.
; Each subroutine returns with exit status in AX.

interrupt	proc	far
	sti
	push_all			; preserve caller's registers
	push	ds
	push	es

	; Read requested function information into registers
	lds	bx,cs:req_ptr
	mov	al,[BX+02h]		; ax = function code
	cbw				; THIS WAS LEFT OUT IN CLASIC NANSI!

	les	si,[BX+0Eh]		; ES:SI = input/output buffer addr
	mov	cx,[BX+12h]		; cx = input/output byte count

	cmp	al, max_cmd
	ja	unk_command		; too big, exit with error code

	xchg	bx, ax
	shl	bx, 1			; form index to table of words
	mov	ax, cs
	mov	ds, ax
	;
	mov	ax, word ptr dos_fn_tab[bx]	; *** load table entry
	;
	or	bx,bx			; *** is this the init call?
	jnz	isnt_fn_0		; *** If not, okay. Else, we
	mov	bx, offset nopcmd	; *** zap the function, replace
	mov	cs:dos_fn_tab, bx	; *** it by a nop for the future
isnt_fn_0:

	call	ax			; <<< main action is done there >>>

int_done:
	lds	bx,cs:req_ptr		; report status
	or	ax, 100h		; (always set done bit upon exit)
	mov	[bx+03],ax

	pop	ES			; restore caller's registers
	pop	DS
	pop_all
	ret				; return to DOS.

unk_command:
	call	badcmd
	jmp	int_done

interrupt	endp

;----- BIOS break handler -----------------------------------------
; Called by BIOS when Control-Break is hit (vector was set up in Init).
; Simply notes that a break was hit.  Flag is checked during input calls.
; *** Why do we set this up? Default handler should do the same! ***

break_handler	proc
	mov	cs:brkkey.len, 1
	iret
break_handler	endp


;------ badcmd -------------------------------------------------------
; Invalid function request by DOS.
badcmd	proc	near
	cmp	cs:chain_handlers,1	; should we chain through?
	jz	chaincmd
	mov	ax, 813h		; return "Error: invalid cmd"
	ret
badcmd	endp


;------ nopcmd -------------------------------------------------------
; Unimplemented or dummy function request by DOS.
nopcmd	proc	near
	cmp	cs:chain_handlers,1	; should we chain through?
	jz	chaincmd
	xor	ax, ax			; No error, not busy.
	ret
nopcmd	endp

;------ chaincmd -----------------------------------------------------
; Chain through function requests to previous CON handler
; *** Must return status in AX. ES:BX destroyed. New in NANSI 4.0 ***
chaincmd	proc	near
	les	bx,cs:prev_handler	; device header of other CON
	mov	ax, es			; strategy / interrupt segment
	mov	word ptr cs:chStrat+2, ax
	mov	word ptr cs:chIntr+2, ax
	mov	ax, es:[bx+6]		; strategy offset
	mov	word ptr cs:chStrat, ax
	mov	ax, es:[bx+8]		; interrupt offset
	mov	word ptr cs:chIntr, ax
	les	bx,cs:req_ptr		; ES:BX request pointer
	call	dword ptr cs:chStrat	; pass ES:BX to strategy call
	call	dword ptr cs:chIntr	; do the main CON device call
	les	bx,cs:req_ptr		; check returned status
	mov	ax,es:[bx+3]		; load returned status to AX
	ret

chStrat	dd 0			; strategy handler far pointer
chIntr	dd 0			; interrupt handler far pointer
chaincmd	endp

;------- dos function #4 ----------------------------------------
; Reads CX characters from the keyboard, places them in buffer at
; ES:SI.
dosfn4	proc	near
	jcxz	dos4done
	mov	di, si
dos4lp:	push	cx
	call	getchar
	pop	cx
	stosb
	loop	dos4lp
dos4done:
	xor	ax, ax			; No error, not busy.
	ret
dosfn4	endp

;-------- dos function #5: non-destructive input, no wait ------
; One-character lookahead into the keyboard buffer.
; If no characters in buffer, return BUSY; otherwise, get value of first
; character of buffer, stuff into request header, return DONE.
dosfn5	proc	near
	call	peekchar
	jz	dos5_busy

	lds	bx,req_ptr
	mov	[bx+0Dh], al
	xor	ax, ax			; No error, not busy.
	jmp	short dos5_exit
dos5_busy:
	MOV	ax, 200h		; No error, busy.
dos5_exit:
	ret

dosfn5	endp

;-------- dos function #6: input status --------------------------
; Returns "busy" if no characters waiting to be read.
dosfn6	proc	near
	call	peekchar
	mov	ax, 200h		; No error, busy.
	jz	dos6_exit
	xor	ax, ax			; No error, not busy.
dos6_exit:
	ret
dosfn6	endp

;-------- dos function #7: flush input buffer --------------------
; Clears the IBM keyboard input buffer.  Since it is a circular
; queue, we can do this without knowing the beginning and end
; of the buffer; all we need to do is set the tail of the queue
; equal to the head (as if we had read the entire queue contents).
; Also resets all the device driver's stuffahead buffers.
dosfn7	proc	near
	xor	ax, ax
	mov	fnkey.len, ax		; Reset the stuffahead buffers.
	mov	cprseq.len, ax
	mov	brkkey.len, ax
	mov	xlatseq.len, ax

	mov	ax, abs40
	mov	es, ax
	mov	ax, es:buffer_head	; clear queue by making the tail
	mov	es:buffer_tail, ax	; equal to the head

	xor	ax, ax			; no error, not busy.
	ret
dosfn7	endp

;------ int_2F ----------------------------------------------
; Int 2F handles DOS multiplexer calls for ANSI control interfaces
; Most useful is probably the install check function. The rest is
; only marginally useful...
; Installed as int 2fh by dosfn0 (init).
old2f	label	dword
old2fofs	dw	?
old2fseg	dw	?

int_2F	proc	near
	cmp	ah,1ah	; CON / ANSI?
	jnz	no2f
	cmp	al,2
	ja	no2f	; RBIL tells that DOS 5+ also chains if AL > 2
	jz	flag2f	; misc flag functions
	cmp	al,1
	jz	ioctli	; "ioctl" functions
inst2f:	mov	al,-1	; the install check: int 2f.1a00 returns AL=ff
	iret
no2f:	jmp	dword ptr cs:old2f	; chain to any other handlers

	; int 2f.1a02 with buffer dsdx: first byte 0: set interlock to 2nd
	; int 2f.1a02 with buffer dsdx: first byte 1: store /L flag to 3rd
flag2f:	push	bx	; interlock: if set, do not hook int 10.0 that much
	mov	bx,dx	; ... as we never hook int 10.0 anyway, ignore :-p
	cmp	byte ptr ds:[bx],1	; get the /L flag?
	jnz	ilock
	mov	byte ptr ds:[bx+2],0	; "the /L flag is not set"
ilock:	nop				; "set the interlock flag"	
good2f:	pop	bx
	push	bp
	mov	bp,sp
	and	byte ptr [bp+6],0feh	; clear carry flag on stack
	pop	bp
	iret

bad2f:	pop	bx
	push	bp
	mov	bp,sp
	or	byte ptr [bp+6],1	; set carry flag on stack
	pop	bp
	iret

	; int 2f.1a01 with buffer dsdx, function cl: 7f get, 5f set
	; display (mode) info... Parameter block is:
	; byte 0, 0, word 0e, even intens / odd blink, 1 txt 2 gfx,
	; bit/pixel (0 mono), pix W, pix H, char W, char H
	; this stuff should also be available via IOCTL, too...?
ioctli:	cmp	cl,5fh			; set mode info
	jz	setm2f
	cmp	cl,7fh			; get mode info
	jnz	no2f

getm2f:	push	bx			; return resolution info
	mov	bx,dx
	call	checki
	jnz	bad2f			; return with error
	push	ax
	xor	ax,ax
	inc	ax
	mov	ds:[bx+4],ax		; LSB: 0 intensity 1 blink (dummy)
	inc	ax			; 2 = graphics mode
	call	in_g_mode		; may think VESA text is gfx, see /T
	sbb	al,0			; if carry: 1 = text mode
	mov	ds:[bx+6],ax
	mov	al,4			; bits per pixel (dummy)
	mov	ds:[bx+8],ax
	call	get_max_y
	inc	ax
	;
	mov	ah,8
	cmp	cs:is_ega, 1
	jnz	default_char_height
	push	ds
	push	bx
	mov	bx,abs40
	mov	ds,bx
	assume	ds:abs40
	mov	ah,bios_char_h
	pop	bx
	pop	ds
	assume	ds:nothing
default_char_height:
	;
	mul	ah
	mov	ds:[bx+12],ax		; height, pixels
	xor	ax,ax
	mov	al,cs:max_y
	inc	ax
	mov	ds:[bx+16],ax		; height, chars
	mov	al,cs:max_x
	inc	ax
	mov	ds:[bx+14],ax		; width, chars
	mov	ah,8
	mul	ah
	mov	ds:[bx+10],ax		; width, pixels
	pop	ax	
	jmp	short good2f		; pop bx, return okay

setm2f:	push	bx			; override mode / resolution params
	mov	bx,dx
	call	checki
	jnz	bad2f			; return with error
	push	ax			; blink/intensity thing [4] ignored
	mov	ax,ds:[bx+6]		; mode (re)set ESC also stop override
	mov	cs:is_g_override,ax	; override: 0 no, 1 text, 2 graphics
					; ignore bpp [8], pixel W/H [10]/[12]
	mov	ax,ds:[bx+14]		; width, in chars
	dec	al
	mov	cs:max_x,al		; override width
	mov	ax,ds:[bx+16]		; height, in chars
	dec	al
	mov	cs:max_y,al		; override height
	pop	ax
	jmp	good2f			; pop bx, return okay

checki:	cmp	word ptr ds:[bx],0	; version 0?
	jnz	checkf
	cmp	word ptr ds:[bx+2],14	; data size okay?
	jnz	checkf
	test	ax,0			; set zero flag
checkf:	ret
int_2F	endp

;------ int_29 ----------------------------------------------
; Int 29 handles DOS quick-access putchar.
; Last device loaded with attribute bit 4 set gets accessed for
; single-character writes via int 29h instead of via interrupt.
; Must preserve all registers.
; Installed as int 29h by dosfn0 (init).
int_29_buf	db	?

int_29	proc	near
	sti
	push	ds
	push	es
	push_all
	mov	cx, 1
	mov	bx, cs
	mov	es, bx
	mov	ds, bx
	mov	si, offset int_29_buf
	mov	byte ptr [si], al
	call	dosfn8
	pop_all
	pop	es
	pop	ds
	iret
int_29	endp

;------ dosfn8 -------------------------------------------------------
; Handles writes to the device (with or without verify).
; Called with
;  CX    = number of bytes to write
;  ES:SI = transfer buffer
;  DS    = CS, so we can access local variables.

dosfn8	proc	near

	mov	f_cptr_seg, es	; save segment of char ptr

	; Read the BIOS buffer address/cursor position variables.
	mov	ax, abs40
	mov	ds, ax
	assume	ds:abs40

	; Find current video mode and screen size.
	mov	ax,word ptr crt_mode	; al = crt mode; ah = # of columns
	mov	cs:video_mode, al
	cmp	cs:is_g_override,0	; override active? stick to it.
	jnz	fix_max_x
	dec	ah			; ah = max column
	mov	cs:max_x, ah
fix_max_x:

	; Find current cursor coordinates.
	mov	al,active_page
	cbw
	add	ax,ax
	xchg	bx,ax
	mov	ax,cursor_posn[bx]
	mov	cs:cur_coords,AX

	; Find video buffer segment address; adjust it
	; so the offset is zero; return in AX.

	; DS is abs40.
	; Find 6845 address.
	mov	ax, addr_6845
	mov	cs:port_6845, ax
	; Find video buffer address.
	MOV	AX,crt_start
	shr	ax, 1
	shr	ax, 1
	shr	ax, 1
	shr	ax, 1
	add	ah, 0B0h		; assume it's a monochrome card...
	CMP	cs:video_mode,07
	jz	d8_gots
	add	ah, 8			; but if not mode 7, it's color.
d8_gots:
	push	cs
	pop	ds
	assume	ds:code
	mov	video_seg, ax
	mov	es, ax
	call	xy_to_regs		; Set DX, DI according to cur_coords.

	; | If in graphics mode, clear old pseudocursor
	call	in_g_mode
	jc	d8_no_cp
		call	pseudocursor	; write block in xor
d8_no_cp:

	mov	ah, cur_attrib
	mov	ds, f_cptr_seg		; get segment of char ptr
	assume	ds:nothing
	cld				; make sure we'll increment

	; The Inner Loop: 4+12 +4+4 +4+4 +4+4 +4+18 = 60? cycles/loop
	; on 8088; at 4.77 MHz, that gives 13 microseconds/loop.
	; At that speed, it takes 25 milliseconds to fill a screen.

	; Get a character, put it on the screen, repeat 'til end of line
	; or no more characters.
	jcxz	f_loopdone		; if count = 0, we're already done.
	cmp	cs:escvector, 0		; If in middle of an escape sequence,
	jnz	f_in_escapex		; jump to escape sequence handler.

f_tloop:; | If in graphics mode, jump to alternate loop
	; | What a massive kludge!  A better approach would have been
	; | to collect characters for a "write n chars" routine
	; | which would handle both text and graphics modes.
	call	in_g_mode
	jc	f_t_cloop
	jmp	f_g_cloop

f_t_cloop:
	LODSB				; get char! (al = ds:[si++])
	cmp	al, 28			; is it a control char?
	jb	f_control		;  maybe...
f_t_nctl:
	STOSW				; Put Char! (es:[di++] = ax)
	dec	dx			; count down to end of line
	loopnz	f_t_cloop		; and go back for more.
	jz	f_t_at_eol		; at end of line; maybe do a crlf.
	jmp	short f_loopdone

f_looploop:
f_ansi_exit:				; in case we switched into
	loopnz	f_tloop			; a graphics mode
f_t_at_eol:
	jnz	f_loopdone
	jmp	f_at_eol

f_loopdone:

	;--------- All done with write request -----------
	; DI is cursor address; cursor position in cur_y, dl.
	mov	ax, cs
	mov	ds, ax			; get our segment back
	assume	ds:code

	; Restore cur_x = max_x - dx + 1.
	mov	al, max_x
	inc	al
	sub	al, dl
	mov	cur_x, al
	; Set cursor position; cursor adr in DI; cursor pos in cur_x,cur_y
	call	set_pseudocursor
	; Return to DOS.
	xor	ax, ax			; No error, not busy.
	ret

	;---- handle control characters ----
	; Note: cur_x is not kept updated in memory, but can be
	; computed from max_x and dx.
	; Cur_y is kept updated in memory.
f_control:
	cmp	al, 27			; Is it an escape?
	jz	f_escapex
	cmp	al, 13			; carriage return?
	jz	f_cr
	cmp	al, 10			; line feed?
	jz	f_lf
	cmp	al, 9			; tab?
	jz	f_tabx
	cmp	al, 8			; backspace?
	jz	f_bs
	cmp	al, 7			; bell?
	jz	f_bell
	jmp	f_nctl			; then it is not a control char.

f_tabx:	jmp	f_tab
f_escapex:
	call	get_max_y	; update in case user has switched modes
				; Trashes AX, but affects no other registers.
	jmp	f_escape
f_in_escapex:
	jmp	f_in_escape

f_bs:	;----- Handle backspace -----------------
	; Moves cursor back one space without erasing.  No wraparound.
	cmp	dl, cs:max_x		; wrap around to previous line?
	ja	fbs_wrap		; yep; disallow it.
	dec	di			; back up one char & attrib,
	dec	di
	inc	dx			; and note one more char left on line.
fbs_wrap:
	jmp	f_looploop

f_bell:	;----- Handle bell ----------------------
	; Use BIOS to do the beep.  DX is not changed, as bell is nonprinting.
	cmp	cs:bios_bell,1		; BIOS or lowlevel beeping?
	jz	use_bios_bell
	cmp	cs:bios_bell,-1		; lowlevel or no beeping?
	jz	use_no_bell
	call	beep			; call lowlevel beeper
use_no_bell:
	or	sp, sp			; clear z
	jmp	f_looploop		; Let main loop decrement cx.

use_bios_bell:
	call	biosbeep
	jmp	short use_no_bell

f_cr:	;----- Handle carriage return -----------
	; di -= cur_x<<1;		set di= address of start of line
	; dx=max_x+1; 			set bx= chars left in line
	mov	al, cs:max_x
	inc	al
	sub	al, dl			; Get cur_x into ax.
	mov	ah, 0
	sub	di, ax
	sub	di, ax
	mov	dl, cs:max_x		; Full line ahead of us.
	inc	dx
	mov	ah, cs:cur_attrib	; restore current attribute
	or	sp, sp			; clear z
	jmp	f_looploop		; and let main loop decrement cx

f_at_eol:
	;----- Handle overrunning right end of screen -------
	; cx++;				compensate for double loop
	; if (!wrap_flag) { dx++; di-=2; }
	; else do_crlf;
	inc	cx
	test	cs:wrap_flag, 1
	jnz	feol_wrap
		dec	di
		dec	di
		inc	dx
		jmp	f_looploop
feol_wrap:
	; dx=max_x+1; 			set bx= chars left in line
	; di -= 2*(max_x+1);
	; do_lf
	mov	dl, cs:max_x
	inc	dx
	sub	di, dx
	sub	di, dx
	; fall thru to line feed routine

f_lf:	;----- Handle line feed -----------------
	; if (cur_y >= max_y) scroll;		scroll screen up if needed
	; else { cur_y++; di += max_x<<1;	else increment Y

	call	get_max_y	; update in case user has switched modes
;	mov	al, cs:max_y	; unneeded- get_max_y returns it in al, too
	cmp	cs:cur_y, al
 	jb	flf_noscroll
		call	scroll_up		; preserves bx,cx,dx,si,di
		jmp	short flf_done
flf_noscroll:
	inc	cs:cur_y
	mov	al, cs:max_x
	mov	ah, 0
	inc	ax
	add	ax, ax
	add	di, ax
flf_done:
	mov	ah, cs:cur_attrib		; restore current attribute
	or	sp, sp			; clear z
	jmp	f_looploop		; and let main loop decrement cx

f_tab:	;----- Handle tab expansion -------------
	; Get cur_x into al.
	mov	al, cs:max_x
	inc	al
	sub	al, dl
	; Calculate number of spaces to output.
	push	cx			; save cx
	mov	ch, 0
	mov	cl, al			; get zero based x coordinate
	and	cl, 7
	neg	cl
	add	cl, 8			; 0 -> 8, 1 -> 8, ... 7 -> 1
	sub	dx, cx			; update chars-to-eol, maybe set z
	pushf				; || save Z for main loop
	; ah is still current attribute.  Move CX spaces to the screen.
	mov	al, ' '
	call	in_g_mode		; | graphics mode
	jnc	f_tab_putc		; |
	REP	STOSW
	popf				; || restore Z flag for main loop test
	pop	cx			; restore cx
	jmp	f_looploop		; Let main loop decrement cx.

;--------------- graphics mode support -----------------------

f_tab_putc:	; graphics mode- call putc to put the char
	add	dx, cx			; move back to start of tab
f_tp_lp:
	call	putchar
	dec	dx			; go to next cursor position
	loop	f_tp_lp
	popf				; Z set if wrapped around EOL
	pop	cx
	jmp	f_looploop

;---- in_g_mode -------------
; Returns Carry set if text mode, clear if in graphics mode.
; Preserves all registers.
; This is slower than the original- do we want to cache the results?
; Right now we call this after every special character (CR, LF, TAB, ESC).
; *** Actually, 3.4, in  f_tloop, calls this for every character ***
; *** at all, to decide whether to use f_t_cloop or f_g_cloop.   ***
; *** the f_g_cloop uses BIOS (func. 2, 9) for output instead of ***
; *** using faster video memory editing which f_t_cloop uses...  ***
; NOTE: the int2f get/set mode interface would work in more standard
; way if we hooked int10. For now, use mode reset/set ANSI escapes
; to cancel overrides caused by int2f set mode.
in_g_mode	proc	near
	cmp	cs:bios_putchar, 0	; normal operation?
	jz	check_g_mode		; then really check
	clc				; else "all are graphics modes"
	ret

check_g_mode:
	cmp	cs:is_g_override, 1	; set via int2f, reset via ESC seq
	jb	check_g_really
	jz	fake_t
fake_g:	clc				; assume graphics mode
	ret
fake_t:	stc				; assume text mode
	ret

is_g_override	dw	0		; 0 auto, 1/2 force text/graphics

check_g_really:
	push	ax
	push	bx
	push	cx
	mov	al, cs:video_mode
	mov	cl, al
	and	cl, 7
	shr	al, 1
	shr	al, 1
	shr	al, 1
	; al = video_mode/8
	; cl = video_mode%8
	mov	bx, offset textmode_table
	db	2eh		; seg_cs
	xlatb
	rcr	al, cl
	rcr	al, 1
	; carry now set if text mode
	pop	cx
	pop	bx
	pop	ax
	ret
in_g_mode	endp

;---- Where to go when a character turns out not to be special
f_nctl:
f_not_ansi:
	call	in_g_mode
	jnc	f_g_nctl		; graphics mode
f_jmptnctl:
	jmp	f_t_nctl		; text mode

;---- Alternate main loop for graphics mode ----
f_g_cloop:
	LODSB				; get char! (al = ds:[si++])
	cmp	al, 28			; is it a control char?
	jb	f_g_control		;  maybe...
f_g_nctl:
	call	putchar
	dec	dx			; count down to end of line
	loopnz	f_g_cloop		; and go back for more.
	jz	f_g_at_eol		; at end of line; maybe do a crlf.
	jmp	f_loopdone

f_g_control:	jmp	f_control
f_g_at_eol:	jmp	f_at_eol

;---- putchar ------------------------------------------------
; Writes char AL, attribute AH to screen at (max_x+1-dl), cur_y.
; On entry, registers set up as per xy_to_regs.
; Preserves all registers. Uses BIOS functions 2 and 9.
putchar	proc	near
	push	dx
	push	cx
	push	bx
	push	ax
	; 1. Set cursor position.
	mov	al, cs:max_x
	inc	al
	sub	al, dl
	mov	cs:cur_x, al
	mov	dx, cs:cur_coords	; get X & Y into DX
	xor	bx, bx			; choose dpy page 0
	mov	ah, 2			; chose "Set Cursor Position"
	int	10h			; call ROM BIOS
	; 2. Write char & attribute.
	mov	cx, 1
	pop	ax 			; get char in AL
	push	ax
	mov	bl, ah			; attribute in BL
	mov	bh, 0
	mov	ah, 9
	int	10h
	pop	ax
	pop	bx
	pop	cx
	pop	dx
	ret
putchar	endp

;---- set_pseudocursor ------------
; If in graphics mode, set pseudocursor, else set real cursor.
; 8/10/86 drk	Fixed multiple display page code
; *** if in "BIOS putchar" mode, always use BIOS to position cursor ***
; Real cursor setting is done by CRT controller programming.

set_pseudocursor	proc	near
	cmp	cs:bios_putchar, 1	; BIOS usage enforced?
	jz	psc_setpos		; jump directly to BIOS setcursor
					; in "pseudocursor" proc.
	call	in_g_mode
	jnc	pseudocursor

	push	ds
	; Write directly to 6845 cursor address register.
	mov	bx, di
	assume	ds:abs40
	mov	ax, abs40
	mov	ds, ax
	add	bx, crt_start		; In case not display page 0.
	shr	bx, 1			; Convert word index to byte index.

	mov	dx, port_6845
	mov	al, 0eh
	out	dx, al

	jmp	$+2
	inc	dx
	mov	al, bh
	out	dx, al

	jmp	$+2
	dec	dx
	mov	al, 0fh
	out	dx, al

	jmp	$+2
	inc	dx
	mov	al, bl
	out	dx, al

	; Set cursor position in low memory.
	; Does anybody ever use anything but page zero?
	mov	al,active_page
	cbw
	add	ax,ax
	xchg	bx,ax
	mov	ax, cs:cur_coords
	mov	cursor_posn[bx],ax
	pop	ds
	ret

	assume	ds:code
set_pseudocursor	endp

;---- pseudocursor --------------------------------------------------
; If pseudo_flag is true, writes a color 15 block in XOR at the
; current cursor location, and sets cursor position.
; Otherwise, just sets cursor position using the BIOS
; Preserves DS, ES, BX, CX, DX, SI, DI.

pseudocursor	proc	near
	test	pseudo_flag, 1
	jz	psc_setpos
		mov	ax, 8f16h	; xor, color 15, ^V (small block)
		call	putchar
		jmp	short psc_ret

psc_setpos:
	push	dx
	push	bx
	push	ax

	mov	dx, cur_coords		; get X & Y into DX

	push	ds
	mov	ax, abs40
	mov	ds, ax
	assume	ds:abs40
	mov	bh, active_page		; supposed to be zero in graph modes?
	pop	ds
	assume	ds:code
	mov	ah, 2			; chose "Set Cursor Position"
	int	10h			; call ROM BIOS

	pop	ax
	pop	bx
	pop	dx

psc_ret:
	ret

pseudocursor	endp

;--------------- end of graphics mode support --------------------

dosfn8	endp

;--- get_blank_attrib ------------------------------------------------
; Determine new attribute and character for a new blank region.
; Use current attribute, just disallow blink and underline.
; (Pretty strange way to do it.  Might want to disallow rev vid, too.)
; Returns result in AH, preserves all other registers.
get_blank_attrib	proc	near
	mov	ah, 0
	call	in_g_mode
	jnc	gb_aok			; if graphics mode, 0 is bkgnd

	mov	ah, cs:cur_attrib
	and	ah, 7fh			; disallow blink
	cmp	cs:video_mode, 7	; monochrome?
	jnz	gb_aok
		cmp	ah, 1		; underline?
		jnz	gb_aok
		mov	ah, 7		; yep- set it to normal.
gb_aok:	ret
get_blank_attrib	endp


;---- scroll_up ---------------------------------------------------
; Scroll screen up- preserves ax, bx, cx, dx, si, di, ds, es.
; Moves screen up 1 line, fills the last line with blanks.
; Attribute of blanks is the current attribute sans blink and underline.

scroll_up	proc	near
	push	ax
	push	bx
	push	cx
	push	dx

	call	get_blank_attrib
	mov	bh, ah			; color to use on new blank areas
	mov	al, 1			; AL is number of lines to scroll.
	mov	ah, 6			; BIOS: scroll up
	mov	cl, 0			; upper-left-x of data to scroll
	mov	ch, 0			; upper-left-y of data to scroll
	mov	dl, cs:max_x		; lower-rite-x
	mov	dh, cs:max_y		; lower-rite-y (zero based)
	int	10h			; call BIOS to scroll a rectangle.

	pop	dx
	pop	cx
	pop	bx
	pop	ax
	ret
scroll_up	endp


	; *** special function: rip codes from keyboard buffer RAM ***
	; *** (for cases where BIOS cannot support > 84 keys...)   ***
	; (formerly called kludge1)
keyE0fixup	proc
      CMP	AL,0E0h	; the 0e0h <-> 00h is a "don't care" for us.
      JNZ	kludge1_notE0
      MOV	AL,00
      RET
kludge1_notE0:
      CMP	AH,0e0h		; is it an alternate (keypad) version?
      JNZ	kludge1_ret	; else we are done
      CMP	AL,0Ah
      JZ	kludge1_ret1c	; 2 types of ENTER keys
      CMP	AL,0Dh
      JZ	kludge1_ret1c
      CMP	AL,02Fh		; ???
      JZ	kludge1_ret35
kludge1_ret:
      RET
kludge1_ret1c:
      MOV	AH,01Ch
      RET
kludge1_ret35:
      MOV	AH,035h		; ???
      RET
keyE0fixup endp

;---- lookup -----------------------------------------------
; Called by getchar, peekchar, and key to see if a given key has
; been redefined.
; Sets AH to zero if AL is not zero (i.e. if AX is not a function key).
; Returns with Z cleared if no redefinition; otherwise,
; Z is set, SI points to redefinition string, CX is its length.
; Preseves AL, all but CX and SI.
; Redefinition table organization:
;  Strings are stored in reversed order, first char last.
;  The word following the string is the character to be replaced;
;  the next word is the length of the string sans header.
; param_end points to the last byte used by the parameter buffer;
; redef_end points to the last word used by the redef table.

lookup	proc	near
    TEST	BYTE PTR [kbd_read_func],01
    JZ	lookup_skipfix	; *** special: treat numpad / keypad differently
    CALL	keyE0fixup
lookup_skipfix:
	or	al, al
	jz	lookup_noextra
    CMP	AL,0E0h
    JZ	lookup_noextra
	mov	ah, 0			; clear extraneous scan code
lookup_noextra:
	mov	si, redef_end		; Start at end of table, move down.

lu_lp:	cmp	si, param_end
	jbe	lu_notfound		; If below redef table, exit.
	mov	cx, [si]
	cmp	ax, [si-2]		; are you my mommy?
	jz	lu_gotit
	sub	si, 4
	sub	si, cx			; point to next header
	jmp	lu_lp
lu_notfound:
	or	si, si			; clear Z
	jmp	short lu_exit
lu_gotit:
	sub	si, 2
	sub	si, cx			; point to lowest char in memory
	cmp	al, al			; set Z
lu_exit:
	ret
lookup	endp

;---- searchbuf --------------------------------------------
; Called by getchar and peekchar to see if any characters are
; waiting to be gotten from sources other than BIOS.
; Returns with Z set if no chars found, BX=keybuf & SI=keybuf.len otherwise.
searchbuf	proc	near
	; Search the stuffahead buffers.
	mov	cx, 4			; number of buffers to check for chars
	mov	bx, offset fnkey - 4
sbloop:	add	bx, 4			; point to next buffer record
	mov	si, [bx].len
	or	si, si			; empty?
	loopz	sbloop			; if so, loop.
	ret
searchbuf	endp

;---- getchar -----------------------------------------------
; Returns AL = next char.
; Trashes AX, BX, CX, BP, SI.
getchar	proc	near
gc_searchbuf:
	; See if any chars are waiting in stuffahead buffers.
	call	searchbuf
	jz	gc_trykbd		; No chars?  Try the keyboard.
	; A nonempty buffer was found.
	dec	[bx].len
	dec	si
	mov	bp, [bx].adr		; get pointer to string
	mov	al, byte ptr ds:[bp][si]; get the char
	; Recognize function key sequences, move them to highest priority
	; queue.
	sub	si, 1			; set carry if si=0
	jc	gc_nofnkey		; no chars left -> nothing to protect.
	cmp	bx, offset fnkey
	jz	gc_nofnkey		; already highest priority -> done.
	or	al, al
	jnz	gc_nofnkey		; nonzero first byte -> not fnkey.
	; Found a function key; move it to highest priority queue.
	dec	[bx].len
	mov	ah, byte ptr ds:[bp][si]; get the second byte of fn key code
gc_fnkey:
	mov	fnkey.len, 1
	mov	fnkeybuf, ah		; save it.
gc_nofnkey:
	; Valid char in AL.  Return with it.
	jmp	short gcdone

gc_trykbd:
	; Actually get a character from the keyboard.
	mov	ah,[kbd_read_func]	; *** which function to use?
    	and	ah,0f0h			; *** mask to keep only 0 or 10h
	int	16h			; BIOS returns with char in AX
	; If it's Ctrl-break, it has already been taken care of.
	or	ax, ax
	jz	gc_trykbd

	; Look in the reassignment table to see if it needs translation.
	call	lookup			; Z=found; CX=length; SI=ptr
	jnz	gc_noredef
	; Okay; set up the reassignment, and run thru the translation code.
	mov	xlatseq.len, cx
	mov	xlatseq.adr, si
	jmp	gc_searchbuf
gc_noredef:
	call keyE0fixup		; *** why that? VERY kludgy!!! ***
	; Is it a function key?
	cmp	al, 0
	jz	gc_fnkey		; yep- special treatment.
gcdone:	ret	; with character in AL.

getchar	endp

;---- peekchar -----------------------------------------------
; Returns Z if no character ready, AL=char otherwise.
; Trashes AX, BX, CX, BP, SI.
peekchar	proc	near
pc_searchbuf:
	call	searchbuf
	jz	pc_trykbd		; No chars?  Try the keyboard.
	; A nonempty buffer was found.
	dec	si
	mov	bp, [bx].adr		; get pointer to string
	mov	al, byte ptr ds:[bp][si]; get the char
	; Valid char from buffer in AL.  Return with it.
	jmp	short pcdone
pc_trykbd:
	; Actually peek at the keyboard.
	mov	ah,[kbd_read_func]	; *** which function to use?
	and	ah,0f0h			; strip to 0 or 10h
	or	ah,01			; make it 1 or 11h
	int	16h			; BIOS returns with char in AX
	jz	pcexit
	; If it's control-break, it's already been taken care of.
	or	ax, ax
	jnz	pc_notbrk
	mov	ah, 0
	int	16h			; so get rid of it!
	jmp	short pc_trykbd
pc_notbrk:
	; Look in the reassignment table to see if it needs translation.
	call	lookup			; Z=found; CX=length; SI=ptr
	jz	pc_kludge
	call keyE0fixup		; *** why that? very kludgy!!! ***
	jmp	pcdone			; Nope; just return the char.
pc_kludge:
	; Okay; get the first code to be returned.
	add	si, cx
	mov	al, [si-1]
pcdone:	or	ah, 1			; NZ; char ready!
pcexit:	ret	; with character in AL, Z true if no char waiting.
peekchar	endp

;---- beep ------------------------------------------------------
; Beep speaker; period given by beep_div, duration by beep_len.
; Preserves all registers. The BIOS bell /B switch disables this.
; *** default was divider 1300 (917Hz), length 3: shorter "IBM variant"
; *** changed to a short, deep/dark "bop". Less annoying.

beep_div	dw	2710		; 440 Hz
beep_len	dw	2		; 2/18.2 sec, pretty short

beep	proc	near
	push	ax

	mov	al, 10110110b		; select 8253 (0b6h)
	out	43h, al			; control port address
	mov	ax, cs:beep_div
	jmp	$+2
	out	42h, al			; timer 2 port: low byte of divisor
	xchg	ah, al
	jmp	$+2
	out	42h, al			; high byte of divisor
	jmp	$+2
					; speaker / ... control port
	in	al, 61h			; get current value of control bits
	push	ax			; save speaker state
	or	al, 3
	jmp	$+2
	out	61h, al			; turn speaker on

	; Wait for desired duration by monitoring time-of-day 18 Hz clock
	push	bx
; ---	push	cx
	push	es
	mov	ax, abs40
	mov	es, ax
	assume	es:abs40
	mov	bx, timer_low		; current time
beepl0:	cmp	bx, timer_low		; wait until it changes
	in	al, 61h
	jz 	beepl0
	inc	bx			; now we have waited 0 to 1 ticks
	add	bx, cs:beep_len		; N ticks later (with wraparound)
; ---	mov	cx, -1			; emergency, in case clock dead
beeplp:
	cmp	bx, timer_low		; time reached? Or beyond?
	jae	beepover		; then stop beeping
	in	al, 61h			; delay (could count cycles here)
	; *** could insert HLT here to save energy ;-) ***
	jmp	short beeplp		; maybe better check only BL ?
; ---	mov	ax, timer_low
; ---	cmp	ax, bx
; ---	jg	beepover		; "jg" was wrong
; ---	loop	beeplp
beepover:
	pop	es
	assume	es:code		; *** "code" or better "none" ?
; ---	pop	cx
	pop	bx

	; Turn off speaker	; *** changed: RESTORE speaker state
	pop	ax		; restore speaker state
; ---	and	al, not 3		; turn speaker off
	out	61h, al

	pop	ax
	ret
beep	endp

;---- beep ------------------------------------------------------
; Beep speaker; Use BIOS TTY output to trigger a default beep.
; Preserves all registers. The BIOS bell /Q switch disables this.
biosbeep	proc near
	push	bp			; old (more portable) version:
	push	ax
	push	bx
	mov	ax, 0e07h		; "write bell to tty simulator"
	mov	bx, 0			; "page zero, color black"
	int	10h			; call BIOS
	pop	bx
	pop	ax
	pop	bp			; bp was saved for "buggy BIOS" case
; --	mov	ah, cs:cur_attrib	; restore current attribute
; --	mov	bx, cs:xlate_tab_ptr	; restore translate table address
	ret
biosbeep	endp

CODE	ends

	end				; of nansi.asm
