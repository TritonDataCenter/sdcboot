;----- nansi_p.asm --------------------------------------------------------
; A state machine implementation of the mechanics of ANSI terminal control
; string parsing.
; Copyright (C) 1986,1999 Daniel Kegel
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
; Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA
; or look on the web at http://www.gnu.org/copyleft/gpl.html.
;
; Entered with a jump to f_escape when driver finds an escape, or
; to f_in_escape when the last string written to this device ended in the
; middle of an escape sequence.
;
; Exits by jumping to f_ANSI_exit when an escape sequence ends, or
; to f_not_ANSI when a bad escape sequence is found, or (after saving state)
; to f_loopdone when the write ends in the middle of an escape sequence.
;
; Parameters are stored as bytes in param_buffer.  If a parameter is
; omitted, it is stored as zero.  Each character in a keyboard reassignment
; command counts as one parameter.
;
; When a complete escape sequence has been parsed, the address of the
; ANSI routine to call is found in ansi_fn_table.
;
; Register usage during parsing:
;  DS:SI points to the incoming string.
;  CX holds the length remaining in the incoming string.
;  ES:DI points to the current location on the memory-mapped screen.
;  DX is number of characters remaining on the current screen line.
;  BX points to the current paramter byte being assembled from the incoming
;  string.  (Stored in cur_parm_ptr between device driver calls, if needed.)
;
; The registers are set as follows before calling the ANSI subroutine:
;  AX = max(1, value of first parameter)
;  CX = number of paramters
;  SI = offset of second parameter from CS
;  DS = CS
;  ES:DI points to the current location on the memory-mapped screen.
;  DX is number of characters remaining on the current screen line.
; The subroutine is free to trash AX, BX, CX, SI, and DS.
; It must preserve ES, and can alter DX and DI if it wants to move the
; cursor.
;
; Revision history:
; 7 July 85: created by DRK
; $Log:	nansi_p.asm $
; Revision 1.9  91/04/07  21:46:39  dan_kegel
; Added /s option for security.  Disables keyboard redefinition.
; 
; Revision 1.8  91/04/07  18:23:09  dan_kegel
; Now supports /t commandline switch to tell nansi about new text modes.
; 
; Revision 1.7  91/04/06  21:39:34  dan_kegel
; Added Log.
; 
;------------------------------------------------------------------------

	; From nansi.asm
	extrn	f_not_ANSI:near		; exit: abort bad ANSI cmd
	extrn	f_ANSI_exit:near	; exit: good cmd done
	extrn	f_loopdone:near		; exit: ran out of chars. State saved.
	extrn	escvector:word		; saved state: where to jump
	extrn	cur_parm_ptr:word	; saved state: where to put next param
	extrn	string_term:byte	; saved state: what ends string
	extrn	cur_x:byte, max_x:byte	; 0 <= cur_x <= max_x
	extrn	cur_attrib:byte		; current color/attribute

	; from nansi_f.asm
	extrn	ansi_fn_table:word	; ANSI subroutine table

	; Used in nansi.asm
	public	f_escape		; entry: found an escape
	public	f_in_escape		; entry: restore state, keep parsing

	; Used in nansi_i.asm and nansi_f.asm
	public	param_buffer, param_end, redef_end
	

code	segment	byte public 'CODE'
	assume	cs:code

; More saved state
in_num		db	?	; true if between a digit and a semi in parse

param_buffer	dw	3000h	; address of first byte free for new params
param_end	dw	3030h	; address of end of free area
redef_end	dw	3131h	; address of end of redefinition area
		; These initial buffer locations are only for debugging

;----- next_is -------------------------------------------------------
; Next_is is used to advance to the next state.  If there are characters
; left in the input string, we jump immediately to the new state;
; otherwise, we shut down the recognizer, and wait for the next call
; to the device driver.
next_is	macro	statename
	loop	statename
	mov	ax, offset statename
	jmp	sleep
	endm

;----- sleep --------------------------------------------------------
; Remember bx and next state, then jump to device driver exit routine.
; Device driver will re-enter at f_in_escape upon next invocation
; because escvector is nonzero; parsing will then be resumed.
sleep:	mov	cs:cur_parm_ptr, bx
	mov	cs:escvector, ax
	jmp	f_loopdone

;----- f_in_escape ---------------------------------------------------
; Main loop noticed that escvector was not zero.
; Recall value of BX saved when sleep was jumped to, and jump into parser.
f_in_escape:
	mov	bx, cs:cur_parm_ptr
	jmp	word ptr cs:escvector

fbr_syntax_error_gate:		; jumped to from inside f_bracket
	jmp	syntax_error

;----- f_escape ------------------------------------------------------
; We found an escape.  Next character should be a left bracket.
f_escape:
	next_is	f_bracket

;----- f_bracket -----------------------------------------------------
; Last char was an escape.  This one should be a [; if not, print it.
; Next char should begin a parameter string.
f_bracket:
	lodsb
	cmp	al, '['
	jnz	fbr_syntax_error_gate
	; Set up for getting a parameter string.
	mov	bx, cs:param_buffer
	mov	byte ptr cs:[bx], 0
	mov	cs:in_num, 0
	next_is	f_get_args

;----- f_get_args ---------------------------------------------------
; Last char was a [.  If the current char is a '=' or a '?', eat it.
; In any case, proceed to f_get_param.
; This is only here to strip off the strange chars that follow [ in
; the SET/RESET MODE escape sequence.
f_get_args:
	lodsb
	cmp	al, '='
	jz	fga_ignore
	cmp	al, '?'
	jz	fga_ignore
		dec	si		; let f_get_param fetch al again
		jmp	short f_get_param
fga_ignore:
	next_is	f_get_param

;----- f_get_param ---------------------------------------------------
; Last char was one of the four characters "]?=;".
; We are getting the first digit of a parameter, a quoted string,
; a ;, or a command.
f_get_param:
	lodsb
	cmp	al, '0'
	jb	fgp_may_quote
	cmp	al, '9'
	ja	fgp_may_quote
		; It's the first digit.  Initialize current parameter with it.
		sub	al, '0'
		mov	byte ptr cs:[bx], al
		mov	cs:in_num, 1	; set flag for sensing at cmd exec
		next_is	f_in_param
fgp_may_quote:
	cmp	al, '"'
	jz	fgp_isquote
	cmp	al, "'"
	jnz	fgp_semi_or_cmd		; jump to code shared with f_in_param
fgp_isquote:
	mov	cs:string_term, al	; save it for end of string
	next_is	f_get_string		; and read string into param_buffer

;----- f_get_string -------------------------------------
; Last character was a quote or a string element.
; Get characters until ending quote found.
f_get_string:
	lodsb
	cmp	al, cs:string_term
	jz	fgs_init_next_param
	mov	byte ptr cs:[bx], al
	cmp	bx, cs:param_end
	adc	bx, 0			; if bx<param_end bx++;
	next_is	f_get_string
; Ending quote was found.
fgs_init_next_param:
	mov	byte ptr cs:[bx], 0	; initialize new parameter
	; | Eat following semicolon, if any.
	next_is	f_eat_semi

;----- f_eat_semi -------------------------------------
; Last character was an ending quote.
; If this char is a semi, eat it; else unget it.
; Next state is always f_get_param.
f_eat_semi:
	lodsb
	cmp	al, ';'
	jz	fes_eaten
		inc	cx
		dec	si
fes_eaten:
	next_is	f_get_param

;----- syntax_error ---------------------------------------
; A character was rejected by the state machine.  Exit to
; main loop, and print offending character.  Let main loop
; decrement CX (length of input string).
syntax_error:
	mov	cs:escvector, 0
	mov	ah, cs:cur_attrib
	jmp	f_not_ANSI	; exit, print offending char

;------ f_in_param -------------------------------------
; Last character was a digit.
; Looking for more digits, a semicolon, or a command character.
f_in_param:
	lodsb
	cmp	al, '0'
	jb	fgp_semi_or_cmd
	cmp	al, '9'
	ja	fgp_semi_or_cmd
		; It's another digit.  Add into current parameter.
		sub	al, '0'
		xchg	byte ptr cs:[bx], al
		push	dx
		mov	dl, 10
		mul	dl
		pop	dx
		add	byte ptr cs:[bx], al
		next_is	f_in_param
	; Code common to states get_param and in_param.
	; Accepts a semicolon or a command letter.
fgp_semi_or_cmd:
	cmp	al, ';'
	jnz	fgp_not_semi
		cmp	bx, cs:param_end	; prepare for next param-
		adc	bx, 0			; if bp<param_end bp++;
		; Set new param to zero, enter state f_get_param.
		mov	cs:in_num, 0		; no longer inside number
		jmp	fgs_init_next_param	; spaghetti code attack!
fgp_not_semi:
	; It must be a command letter.
	cmp	al, '@'
	jb	syntax_error
	cmp	al, 'z'
	ja	syntax_error
	cmp	al, 'Z'
	jbe	fgp_is_cmd
	cmp	al, 'a'
	jb	syntax_error
		; It's a lower-case command letter.
		; Remove hole between Z and a to save space in table.
		sub	al, 'a'-'['
fgp_is_cmd:
	; It's a command letter.  Save registers, convert letter
	; into address of routine, set up new register usage, call routine.
	push	si			; These three registers hold info
	push	cx			; having to do with the input string,
	push	ds			; which has no interest at all to the
					; control routine.

	push	cs
	pop	ds			; ds is now cs

	sub	al, '@'			; first command is @: insert chars
	cbw
	add	ax, ax
	add	ax, offset ansi_fn_table
	; ax is now pointer to command routine address in table

	mov	cx, bx
	mov	si, param_buffer	; si is now pointer to parameters
	sub	cx, si			;
	test	in_num, 1
	jz	fip_out_num
		inc	cx
fip_out_num:				; cx is now # of parameters

	xchg	ax, bx			; save pointer to routine in bx

	; Calculate cur_x from DX.
	mov	al, max_x
	inc	ax
	sub	al, dl	
	mov	cur_x, al

	; Get first parameter into AX; if defaulted, set it to 1.
	mov	ah, 0
	lodsb
	or	al, al
	jnz	fgp_callem
		inc	ax
fgp_callem:
	; Finally, call the command subroutine.
	call	word ptr [bx]

	pop	ds
	pop	cx
	pop	si

	mov	ah, cs:cur_attrib	; Prepare for STOSW.
	mov	cs:escvector, 0		; No longer parsing escape sequence.
	; Set flags for reentry at loopnz
	or	dx, dx			; "Any columns left on line?"
	; Re-enter at bottom of main loop.
	jmp	f_ansi_exit

	
code	ends
	end                          	; of nansi_p.asm

