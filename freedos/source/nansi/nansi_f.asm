;----- nansi_f.asm ---------------------------------------------
; The ANSI control subroutines.
; Copyright (C) 1986,1999 Daniel Kegel
; Updated 2007 by Eric Auer
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
; Each routine is called with the following register usage:
;  AX = max(1, value of first parameter)
;  Z flag is set if first parameter is zero.
;  CX = number of paramters
;  SI = offset of second parameter from CS
;  DS = CS
;  ES:DI points to the current location on the memory-mapped screen.
;  DX is number of characters remaining on the current screen line.
; The control routine is free to trash AX, BX, CX, SI, and DS.
; It must preserve ES, and can alter DX and DI if it wants to move the
; cursor.
;
; Revisions:
;  19 Aug 85: Fixed horrible bug in insert/delete line.
;  26 Aug 85: Fixed simple limit-to-one-too-few-lines bug in ins/del line;
;  anyway, it inserts 24 lines when on line 2 now.  Whether it's fixed...
;  4 Sept 85: Fixed bug created on 26 Aug 85; when limiting ins/del line
;  count, we are clearing, not scrolling; fixed BIOS call to reflect this.
;  30 Jan 86: Added EGA cursor patch
;  31 Jan 86: Disabled insert/delete char in graphics modes
;             Implemented keyboard redefinition reset
;  1 Feb 86: added video_mode and max_x test after mode set
; 10 Jun 90: Deleted stuff having to do with Output Char Translation
; 11 Jun 90: Added code to sense max_y from BIOS if ega installed.
; 23 May 07: Make mode set / reset ( sm: and rm: ) clear is_g_override.
;----------------------------------------------------------------

; *** has been on by default since 3.3 - moving to inside nansi_f.asm!
; *** if this is 1, CLS also moves the cursor to the home position.
; ***
cls_homes_too	equ     1	; set true for ANSI.SYS compatibility
; ***


	; To nansi_p.asm
	public	ansi_fn_table
	public	nul

	; From nansi.asm
	extrn	port_6845:word
	extrn	cur_coords:word, saved_coords:word
	extrn	cur_x:byte, max_x:byte
	extrn	cur_y:byte, max_y:byte
	extrn	cur_attrib:byte, wrap_flag:byte
	extrn	xy_to_regs:near
	extrn	get_blank_attrib:near
	extrn	cpr_esc:byte, cprseq:word
	extrn	video_mode:byte
	extrn	lookup:near
	extrn	lookup_skipfix:near	; was "lookup_04c5"
	extrn	in_g_mode:near
	extrn	is_g_override:word
	extrn	is_ega:byte
	extrn	get_max_y:near
	extrn	pseudo_flag:byte

	extrn	bios_putchar:byte	; ***

	; from nansi_p.asm
	extrn	param_buffer:word	; buffer for ESC parsing
	extrn	param_end:word		; boundary ESC parse buf / redef buf
	extrn	redef_end:word		; end of key redefinition buffer

	; to nansi_i.asm and nansi.asm
	public	ibm_kkr			; patch point to disable key redef

keybuf	struc				; used in making cpr sequence
len	dw	?
adr	dw	?
keybuf	ends


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
addr_6845	dw	?
crt_mode_set	db	?
crt_palette	db	?

ABS40	ends

code	segment byte public 'CODE'
	assume cs:code, ds:code

;----- byteout ---------------------------------------------------
; Converts al to a decimal ASCII string (in 0..199),
; stores it at ES:DI++.  Returns DI pointing at byte after last digit.
; Destroys DL.

byteout	proc	near
	;---- begin july 89 change ----
	; Handle special case of 100..199 by outputting a '1' here.
	; (Is this code getting ugly or what?)
	cmp	al,100
	jb	byteout_less
		sub	al, 100
		xchg	al, ah
		mov	al, '1'
		stosb
		xchg	al, ah
byteout_less:
	;---- end of july 89 change ----
	aam
	add	ax, 3030h
	xchg	ah, al
	stosb
	xchg	ah, al
	stosb
	ret
byteout	endp

;----- ansi_fn_table -----------------------------------
; Table of offsets of terminal control subroutines in order of
; the character that invokes them, @..Z, a..z.  Exactly 53 entries.
; All the subroutines are defined below in this module.
ansi_fn_table	label	word
	dw	ic,  cup, cdn, cfw, cbk		; @, A, B, C, D
	dw	nul, nul, nul, hvp, nul		; E, F, G, H, I
	dw	eid, eil, il,  d_l, nul		; J, K, L, M, N
	dw	nul, dc,  nul, nul, nul		; O, P, Q, R, S
	dw	nul, nul, nul, nul, nul		; T, U, V, W, X
	dw	nul, nul			; Y, Z
	dw	nul, nul, nul, nul, nul		; a, b, c, d, e
	dw	hvp, nul, sm,  nul, nul		; f, g, h, i, j
	dw	nul, rm,  sgr, dsr, nul		; k, l, m, n, o
	dw	key, nul, nul, scp, nul		; p, q, r, s, t
	dw	rcp, nul, nul, nul, nul		; u, v, w, x, y
	dw	nul				; z

ansi_functions	proc	near		; set return type to NEAR

;----- nul ---------------------------------------------
; No-action ansi sequence; called when unknown command given.
nul:	ret

;----- Cursor Motion -----------------------------------------------

;-- cursor to y,x
hvp:	or	al, al		; First parameter is desired Y coordinate.
	jz	hvp_yok
	dec	ax		; Convert to zero-based coordinates.
hvp_yok:mov	cur_y, al
	; Get second parameter, if it is there, and set X with it.
	xor	ax, ax
	cmp	cx, 2		; was there a second parameter?
	jb	hvp_xok
	lodsb			; yes.
	or	al, al
	jz	hvp_xok
	dec	ax		; convert to zero-based coordinates.
hvp_xok:mov	cur_x, al

	; Clip to maximum coordinates.
hvp_set:
	mov	ax, cur_coords		; al = x, ah = y
	cmp	al, max_x
	jbe	hvp_sxok
		mov	al, max_x
		mov	cur_x, al
hvp_sxok:
	cmp	ah, max_y
	jbe	hvp_syok
		mov	al, max_y
		mov	cur_y, al
hvp_syok:
	; Set values of DX and DI accordingly.
	call	xy_to_regs
	ret

;-- cursor forward --
cfw:	add	cur_x, al
	jmp	hvp_set

;-- cursor back -----
cbk:	sub	cur_x, al
	jae	cbk_ok
		mov	cur_x, 0
cbk_ok:	jmp	hvp_set

;-- cursor down -----
cdn:	add	cur_y, al
	jmp	hvp_set

;-- cursor up -------
cup:	sub	cur_y, al
	jae	cup_ok
		mov	cur_y, 0
cup_ok:	jmp	hvp_set

;-- save cursor position --------------------------------------
scp:	mov	ax, cur_coords
	mov	saved_coords, ax
	ret

;-- restore cursor position -----------------------------------
rcp:	mov	ax, saved_coords
	mov	cur_coords, ax
	jmp	hvp_set		; Clip in case we have switched video modes.

;-- set graphics rendition ------------------------------------
; Modifies the color in which new characters are written.

sgr:	dec	si		; get back pointer to first parameter
	or	cx, cx		; Did he give any parameters?
	jnz	sgr_loop
		mov	byte ptr [si], 0	; no parameters, so fake
		inc	cx			; one with the default value.
	; For each parameter
sgr_loop:
		lodsb				; al = next parameter
		; Search color table
		push	cx
		mov	cx, colors
		mov	bx, offset color_table-3
sgr_search:
			add	bx, 3
			cmp	al, byte ptr [bx]
			loopnz	sgr_search	; until match found or done
		jnz	sgr_loopx
	
		; If parameter named a known color, set the current
		; color variable.
		mov	ax, [bx+1]
		and	cur_attrib, al
		or	cur_attrib, ah
sgr_loopx:
		pop	cx
		loop	sgr_loop		; until no more parameters.

	; DRK 7 June 1990
	; Removed horrible bug to make new compile match 'classic' compile
	; It seems that this was not quite the 'classic' source.
	;; DRK 2/25/86  Thanks to Barry Simon for finding graphics mode bug
	;; Set background color in case we enter a graphics mode
	; mov	ah, 11				; color palette
	; mov	bh, 0				; background color
	; mov	bl, cur_attrib
	; mov	cl, 4
	; shr	bl, cl				; get background bits
	; int	10h				; call video bios

	ret

;-- erase in line ----------------------------------------
; Uses BIOS to scroll away a one-line rectangle
eil:	push	dx
	mov	cx, cur_coords
	mov	dh, ch
	jmp	short scrollem

;-- erase in display -------------------------------------
; Uses BIOS to scroll away all of display
; To be compatible with ANSI.SYS, ignores its parameter!
eid:
	;cmp	al, 2
	;jnz	eid_ignore	; param must be two
	if	cls_homes_too
			mov	cur_coords, 0
			call	xy_to_regs
	endif
	push	dx
	xor	cx, cx
	mov	dh, max_y
scrollem:
	call	get_blank_attrib
	mov	bh, ah
	mov	dl, max_x
	mov	ax, 600h
	push	bp	; *** ancient BIOSes trash BP when scrolling
	int	10h
	pop	bp	; ***
	pop	dx
eid_ignore:
	ret

;-- device status report --------------------------------
; Stuffs an escape, a left bracket, current Y, semicolon, current X,
; a capital R, and a carriage return into input stream.
; The coordinates are 1 to 3 decimal digits each.
 
dsr:	push	di
	push	dx
	push	es
	mov	ax, cs
	mov	es, ax
	std			; Store string in reversed order for fun
	mov	di, offset cpr_esc - 2
	mov	al, cur_y
	inc	al		; convert to one-based coords
	call	byteout		; row
	mov	al, ';'		; ;
	stosb
	mov	al, cur_x
	inc	al		; convert to one-based coords
	call	byteout		; column
	mov	al, 'R'		; R ANSI function 'Cursor Position Report'
	stosb
	mov	al, 13
	mov	word ptr cprseq.adr, di	; save pointer to last char in string
	stosb				; send a carriage return, too
	mov	ax, offset cpr_esc
	sub	ax, di			; ax is # of characters in string
	mov	word ptr cprseq.len, ax	; pass info to the getchar routine
	cld
	pop	es
	pop	dx
	pop	di
	ret

;---- Delete/Insert Lines -------------------------------
; AL is number of lines to delete/insert.
; Preserves DX, DI; does not move cursor.

d_l:	; Delete lines.
	mov	ah, 6			; BIOS: scroll up
	jmp	short il_open

il:	; Insert lines.
	mov	ah, 7			; BIOS: scroll down

il_open:
	; Whether inserting or deleting, limit him to (max_y - cur_y) lines;
	; if above that, we're just clearing; set AL=0 so BIOS doesn't burp.
	mov	bh, max_y
	sub	bh, cur_y
	cmp	al, bh
	jbe	il_ok			; DRK 9/4...
		mov	al, 0		; he tried to move too far
il_ok:
	push	ax
	call	get_blank_attrib
	mov	bh, ah			; color to use on new blank areas
	pop	ax			; AL is number of lines to scroll.

	mov	cl, 0			; upper-left-x of data to scroll
	mov	ch, cur_y		; upper-left-y of data to scroll
	push	dx
	mov	dl, max_x		; lower-rite-x
	mov	dh, max_y		; lower-rite-y (zero based)
	push	bp			; *** BIOS func 6/7 may trash BP
	int	10h			; call BIOS to scroll a rectangle.
	pop	bp			; *** ancient BIOS thing protection
	pop	dx
	ret				; done.

;-- Insert / Delete Characters ----------------------------
; AL is number of characters to insert or delete.
; Preserves DX, DI; does not move cursor.

ic:	mov	ch, 1			; 1 => swap dest & source below
	jmp	short dc_ch

dc:	mov	ch, 0

dc_ch:
	call	in_g_mode
	jnc	dc_ret			; | if in graphics mode, ignore.

	; AL = number of chars to ins or del (guarenteed nonzero).
	; Limit him to # of chars left on line.
	cmp	al, dl
	jbe	dc_cok
		mov	al, dl
dc_cok:
	push	di			; DI is current address of cursor
	xchg	ax, cx			; CX gets # of chars to ins/del
	mov	bp, cx			; BP gets # of columns to clear.

	; Set up source = destination + cx*2, count = dx - cx
	mov	ch, 0			; make it a word
	mov	si, di
	add	si, cx
	add     si, cx
	neg	cl
	add	cl, dl
	mov	ch, 0			; CX = # of words to transfer
	cld				; REP increments si & di

	; If this is an insert, then flip transfer around in both ways.
	test	ah, 1
	jz	dc_noswap
		xchg	di, si		; source <-> dest
		std			; up <-> down
		mov	ax, cx		; make move over same range
		dec	ax
		add	ax, ax		; AX=dist from 1st to last byte.
		add	di, ax		; Start transfer at high end of block
		add	si, ax		;  instead of low end.
dc_noswap:
	; Move those characters.
	push	es
	pop	ds
	rep	movsw
	mov	cx, bp
	; Figure out what color to make the new blanks.
	call	get_blank_attrib
	mov	al, ' '
	; Blank out vacated region.
	rep	stosw

	; All done.
	cld				; restore normal REP state and
	pop	di			;  cursor address.
dc_ret:	ret

;---- set / reset mode ---------------------------------------
; Sets graphics/text mode; also sets/resets "no wrap at eol" mode.
sm:	mov	cl, 0ffh	; set
sm_rs:	mov	is_g_override, 0	; stop overrides
	; Is it "graphics cursor on/off"?
	cmp	al, 99			; for compatibility with nnansi
	jnz	sm_notgc
		mov	pseudo_flag, cl
		jmp	short sm_done
sm_notgc:
	; Is it "wrap at eol" ?
	cmp	al, 7
	jnz	sm_notwrap
		mov	wrap_flag, cl	; true = wrap at EOL
		jmp	short sm_done
sm_notwrap:
	; Is it "set highest number of screen lines available"? then load 8x8 font!
	cmp	al, 43
	jnz	sm_video
		; Only valid for the Enhanced Graphics Adaptor.
		cmp	is_ega, 1
		jnz	sm_done

		; 43 line mode only allowed in text modes, for now.
		call	in_g_mode
		jnc	sm_done

		mov	ah, 0			; "Set video mode"
		mov	al, video_mode		; Re-init current mode
		int	10h

		mov	ax,1112h		; Load 8x8 font
		mov	bl,0			; (instead of 8x14)
		int	10h

		mov	ax, 1200h		; Load new printscreen
		mov	bl, 20h
		int	10h

		mov	ah,1
		mov	cx,0707h		; (Load cursor scan lines)
		int	10h

		cmp	cs:bios_putchar,1	; *** BIOS only mode?
		jz	no_hw_cursor_resize	; *** 

		; | Patch; this gotten by painful observation of
		; | IBM's professional editor.  I think there's a
		; | documented bug in Video Bios's "load cursor scan line"
		; | call; try looking in latter 1985 PC Tech Journal.
		mov	dx, port_6845		; '6845' command reg
		mov	al, 10			; cursor start
		out	dx, al
		jmp	$+2
		inc	dx
		mov	al, 7			; line 7, no special flags
		out	dx, al			; set cursor start line
		;
no_hw_cursor_resize:
		jmp	short sm_home
sm_video:
	; It must be a video mode.  Call BIOS.
	mov	ah, 0		; "set video mode"
	int	10h

sm_home:
	; Read the BIOS buffer address/cursor position variables.
	mov	ax, abs40
	push	ds
	mov	ds, ax
	assume	ds:abs40
	; Find current video mode and screen size.
	mov	ax,word ptr crt_mode	; al = crt mode; ah = # of columns
	pop	ds
	assume	ds:code
	mov	video_mode, al
	dec	ah			; ah = max column
	mov	max_x, ah

	call	get_max_y		; trashes ax, updates max_y

	; Since cursor may end up in illegal position, it's best to
	; just go home after switching video modes.
	mov	cur_coords, 0
	call	xy_to_regs
sm_done:
	ret

rm:	mov	cl, 0		; reset
	jmp	sm_rs


ansi_functions	endp	; end dummy procedure block



;-------- Color table -----------------------------------------
; Used in "set graphics rendition"
colors	equ	22			; number of colors in table
color_table:
	db	0, 000h,07h		; all attribs off; normal.
	db	1, 0ffh,08h		; bold
	db	4, 0f8h,01h		; underline
	db	5, 0ffh,80h		; blink
	db	7, 0f8h,70h		; reverse
	db	8, 088h,00h		; invisible

	db	30,0f8h,00h		; black foreground
	db	31,0f8h,04h		; red
	db	32,0f8h,02h		; green
	db	33,0f8h,06h		; yellow
	db	34,0f8h,01h		; blue
	db	35,0f8h,05h		; magenta
	db	36,0f8h,03h		; cyan
	db	37,0f8h,07h		; white

	db	40,08fh,00h		; black background
	db	41,08fh,40h		; red
	db	42,08fh,20h		; green
	db	43,08fh,60h		; yellow
	db	44,08fh,10h		; blue
	db	45,08fh,50h		; magenta
	db	46,08fh,30h		; cyan
	db	47,08fh,70h		; white

;-- keyboard reassignment -------------------------------
; Key reassignment buffer is between param_end and redef_end+2, exclusive.
; When it shrinks or grows, param_end is moved.
; Format of an entry is as follows:
;   highest address -> length:word (may be 0)
;                      key to replace:word     (either hi or low byte is zero)
;                          .
;                          .    new key value, "length" bytes long
;                          .
;   lowest address  -> next entry, or free space.
; If no arguments are given, keyboard is reset to default condition.
; Otherwise, first parameter (or first two, if first is zero) defines
; the key whose value is to be changed, and the following parameters
; define the key's new, possibly zero-length, value.

ibm_kkr:	; this byte replaced with a near return if kkr disabled
key:		; the rest of the handler can be overwritten if kkr is off
	; Is this a reset?
	or	cx, cx
	jz	key_init
	; Get the first (or first two) parameters
	cld
	dec	si	; point to first param
	dec	cx	; Assume it's a fn key, get two params
	dec	cx
	lodsw
	or	al, al	; Is it a function key?
	jz	key_fnkey
	cmp al,0e0h ; Is it an extended function key? [kludge]
	jz  key_fnkey
		; It's not a function key- put second param back
		inc	cx
		dec	si
key_fnkey:
	; Key to redefine now in AX.  If it's already redefined,
	; lookup will set Z, point SI to redef string, set CX to its length.
	push	di
	push	es
	push	cx
	push	si

	std			; moving up, must move from top down
	push	ds
	pop	es		; string move must have ES=DS
	call	lookup_skipfix	; rets Z if redefined... (was "lookup_04c5")
	jnz	key_newkey
	; It's already defined.  Erase its old definition- i.e., move
	; region param_end+1..SI-1 upwards CX+4 bytes, add CX+4 to param_end.
	add	cx, 4
	mov	bp, param_end	; save old value in bp...
	add	param_end, cx
	dec	si		; start at (SI-1)
	mov	di, si
	add	di, cx		; move to (start + CX+4)
	mov	cx, si
	sub	cx, bp		; length of region old_param_end+1..start
	rep	movsb
key_newkey:
	; Key not redefined.  See if there's enough room to redefine it.
	pop	si		; get back pointer to redef string
	pop	cx		; get back number of bytes in redef string
	mov	di, param_end	; hi byte of new redef record, hi byte of len
	sub	di, 4		; hi byte of new data field
	mov	bx, di
	sub	bx, cx		; hi byte of remaining buffer space
	sub	bx, 16		; better be at least 16 bytes room
	cmp	bx, param_buffer
	jb	key_popem	; nope- forget it.
	; Nothing in the way now!
	mov	[di+3], cx	; save length field
	mov	[di+1], ax	; save name field
	jcxz	key_nullstring
key_saveloop:			; save data field
	movsb
	add	si, 2		; input string ascending, output descending
	loop	key_saveloop
key_nullstring:
	mov	param_end, di	; save adr of new hi byte of free area
key_popem:
	pop	es
	pop	di

key_exit:
	cld
	ret

key_init:
	; Build the default redefinition table:
	;	control-printscreen -> control-P
	push	es
	push	ds
	pop	es
	std
	mov	di, redef_end
	mov	ax, 1
	stosw
	mov	ax, 7200h	; control-printscreen
	stosw
	mov	al, 16		; control P
	mov	param_end, di	; save new bottom of redef table
	stosb
	pop	es
	jmp	key_exit

	; nansi_i is next, has to start with dosfn0
code	ends
	end                          	; of nansi_f.asm

