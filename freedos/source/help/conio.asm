;   Console I/O
;
;   Copyright (c) Express Software 1998-2003
;   See docs\htmlhelp\copying for licensing terms
;
;   Created by: Joseph Cosentino.
;
;   Updated for masm by RP. Also added MONO mode checking.

.MODEL COMPACT
.DATA

PUBLIC	_ScreenWidth
PUBLIC	_ScreenHeight
PUBLIC	_MouseInstalled
PUBLIC  _WheelSupported
PUBLIC  _MonoOrColor    ; Added by RP. Mono=1, Color = 0.

_MonoOrColor    DB   ?  ; Added by RP
oldvidmod       DB   ?  ; Added by RP
oldcursorshape  DW   ?  ; Added by RP
_ScreenArea	EQU DWord Ptr _ScreenOffset
_ScreenOffset	DW   ?	; 0000h
_ScreenSegment	DW   ?	; B800h
_ScreenWidth	DB   ?	;  80
_ScreenHeight	DB   ?	;  25
_ScreenLength	DW   ?	; 80*25
_MouseInstalled	DW   ?	; 0 or 1
_WheelSupported DW   ?  ; 0 or 1
_LastKeyShifts	DW   ?	;
_LastMousePosX	DW   ?  ;
_LastMousePosY	DW   ?  ;
_LastMouseBtns	DW   ?	;
OriginalTimer1	DW   ?	;
OriginalTimer2	DW   ?	;
ExtendedKeyb    DB   ?  ;

.CODE

PUBLIC	_conio_init
PUBLIC	_conio_exit
PUBLIC	_show_mouse
PUBLIC	_hide_mouse
PUBLIC	_move_mouse
PUBLIC	_cursor_size
PUBLIC	_move_cursor
PUBLIC	_get_event
PUBLIC	_write_char
PUBLIC	_write_string
PUBLIC	_load_window
PUBLIC	_save_window
PUBLIC	_clear_window
PUBLIC	_scroll_window
PUBLIC	_border_window

Show_Mouse	MACRO
		cmp	_MouseInstalled, 1
		jne	@@nomouse1
		mov	ax, 0001h
		int	33h
    @@nomouse1:
		ENDM


Hide_Mouse	MACRO
		cmp	_MouseInstalled, 1
		jne	@@nomouse2
		mov	ax, 0002h
		int	33h
    @@nomouse2:
		ENDM


;----------------------------------------------------------------

_conio_init	PROC
     forcemono  EQU     [bp+06h]

		push	bp
		mov	bp, sp
		push	es
		push	si
		push	di

                mov     ah,0Fh          ; Get current video mode
		int	10h
                mov     oldvidmod, al   ; RP store so that it can be restored
                                        ;    later by _conio_exit.
                cmp     al, 07h
		je	@@Mono

                mov     al, forcemono   ; RP Force monochrome mode
                cmp     al, 0           ; RP regardless of current video mode
                jne     @@Force         ; RP

                mov     al, oldvidmod   ; check if it is already in text
                cmp     al, 03h         ; mode - colour.
                je      @@Color
    @@Reset:
                mov     ax, 0003h       ; If unknown set Color
                int     10h
    @@Color:  
		mov	_ScreenSegment, 0B800h
                mov     _MonoOrColor,   0 ; RP
		jmp	@@skip1
    @@Force:
                mov     ax,0007h
                int     10h
    @@Mono:
		mov	_ScreenSegment, 0B000h
                mov     _MonoOrColor,   1 ; RP
    @@skip1:
		push	ds
		mov	ax, 0
		mov	ds, ax
		mov	bx, 484h
		mov	al, [bx]
		pop	ds
		inc	al
		mov	_ScreenHeight, al
		mov	ah,0Fh
		int	10h
		mov	_ScreenWidth, ah
		mov	al, _ScreenHeight
		mov	ah, _ScreenWidth
		mul	ah
		mov	_ScreenLength, ax
		mov	_ScreenOffset, 0h

                mov     ah, 03          ; store old cursor
                mov     bh, 0
                int     10h
                mov     oldcursorshape, cx
                mov     ah, 01          ; Disable cursor - RP
                mov     al, oldvidmod   ; necessary on certain machines
                mov     cx, 211Fh
                mov     bx, 0
                int     10h

		mov	ax,0000h	; Check for a mouse
		int	33h
		cmp	ax, 0h
		je	@@No_Mouse
		cmp	ax, 3h
		je	@@_Mouse
		cmp	ax, 2h
		je	@@_Mouse
		cmp	ax, 0FFFFh
		mov	ax, 2h
		je	@@_Mouse
		mov	ax, 1h
    @@_Mouse:
		mov	_MouseInstalled, 1
		mov	ax, 0003h	; Reset mouse
		int	33h
		mov	_LastMousePosX, cx
		mov	_LastMousePosY, dx
		mov	_LastMouseBtns, bx

        mov _WheelSupported, 0
        mov ax, 0011h
        int 33h
        cmp ax, 574Dh
        jne @@skip2
        mov _WheelSupported, 1

        jmp @@skip2
    @@No_Mouse:
		mov	_MouseInstalled, 0
        
    @@skip2:    ; check for extended keyboard support in BIOS Data Area - RP
                push    ds
                mov     ax, 40h
                mov     ds, ax
                mov     bx, 96h
                mov     al, [bx]
                test    al, 10h
                pop     ds                
                jz      @@skip3
                ; some computers report a false positive by the above method
                ; so now test the enhanced-only keyboard interrupt 16.12
                ; which will modify al if it's there
                cli
                mov     ax, 1255h
                int     16h
                cmp     al, 055h
                je      @@skip3
                mov     ax, 12AAh
                int     16h                
                cmp     al, 0AAh
                je      @@skip3
                mov     ExtendedKeyb, 10h
    @@skip3:
                sti
                mov     ah, 2h          ; Get keyboard status
                add     ah, ExtendedKeyb
		int	16h
		mov	_LastKeyShifts, ax
		
		pop	di
		pop	si
		pop	es
		pop	bp
		retf
_conio_init	ENDP

;----------------------------------------------------------------

_conio_exit	PROC
		push	bp
		mov	bp, sp
		push	ds
		push	di
		push	si

                mov     ah, 01          ; Disable cursor - RP
                mov     al, oldvidmod   ; Necessary on certain machines
                mov     cx, oldcursorshape
                mov     bx, 0
                int     10h

                mov     ah, 0Fh
                int     10h
                cmp     al, oldvidmod
                je      @@modeok
                mov     al, oldvidmod
                mov     ah, 0
                int     10h

    @@modeok:   pop     di
		pop	si
                pop     es
		pop	bp
                retf
_conio_exit	ENDP


;----------------------------------------------------------------


_show_mouse	PROC
		Show_Mouse
		retf
_show_mouse	ENDP



_hide_mouse	PROC
		Hide_Mouse
		retf
_hide_mouse	ENDP



_move_mouse	PROC

	Y	EQU	[bp+08h]
	X	EQU	[bp+06h]

		push	bp
		mov	bp, sp
		mov	ax, 0004h
		mov	cx, X
		mov	dx, Y 

		dec	cx
		shl	cx, 1
		shl	cx, 1
		shl	cx, 1

		dec	dx
		shl	dx, 1
		shl	dx, 1
		shl	dx, 1

		int	33h
		pop	bp
		retf
_move_mouse  ENDP


;----------------------------------------------------------------


_move_cursor	PROC

	Y	EQU	[bp+08h]
	X	EQU	[bp+06h]

		push	bp
		mov	bp, sp
		mov	ah, 02
		mov	bh, 00
		mov	dl, X
		mov	dh, Y
		sub	dx, 0101h
		int	10h
		pop	bp
		retf
_move_cursor	ENDP


_cursor_size	PROC

      Bottom	EQU	[bp+08h]
	Top	EQU	[bp+06h]

		push	bp
		mov	bp, sp
		mov	ah, 01
		mov	cl, Top
		mov	ch, Bottom
		and	cx, 1F1Fh
		int	10h
		pop	bp
		retf

_cursor_size	ENDP


;----------------------------------------------------------------


_write_char	PROC

        Char    EQU     [bp+0Ch] ; Renamed this variable from C to Char, since
        Y       EQU     [bp+0Ah] ; C is reserved in MASM - RP
	X	EQU	[bp+08h]
    Attribute	EQU	[bp+06h]

		Hide_Mouse

		push	bp
		mov	bp, sp
		push	ds
		push	di
		push	si
		sub	ax, ax
		mov	bx, ax
		mov	cx, bx
		mov	dx, cx
	
		mov	dl, _ScreenWidth
		mov	al, Y
		mov	bl, X	 
		dec	al
		dec	bl	 
		mul	dl
		add	ax, bx
		shl	ax, 1
		les	di, _ScreenArea
		add	di, ax

                mov     al, Char
		mov	ah, Attribute
		stosw

		pop	si
		pop	di
		pop	ds
		pop	bp

		Show_Mouse
		retf
_write_char	ENDP



_write_string	PROC

	S	EQU	[bp+0Ch]
	Y	EQU	[bp+0Ah]
	X	EQU	[bp+08h]
    Attribute	EQU	[bp+06h]

		Hide_Mouse

		push	bp
		mov	bp, sp
		push	ds
		push	di
		push	si
		sub	ax, ax
		mov	bx, ax
		mov	cx, bx
		mov	dx, cx
	
		mov	dl, _ScreenWidth
		mov	al, Y
		mov	bl, X	 
		dec	al
		dec	bl	 
		mul	dl
		add	ax, bx
		shl	ax, 1
		les	di, _ScreenArea
		lds	si, S
		add	di, ax

		mov	ah, Attribute
		jmp	@@first
    @@next:
		stosw
    @@first:
		lodsb
		cmp	al, 0
		jne	@@next
		
		pop	si
		pop	di
		pop	ds
		pop	bp

		Show_Mouse
		retf
_write_string	ENDP


;----------------------------------------------------------------


_save_window	PROC

   Destination	EQU	[bp+0Eh]
	H	EQU	[bp+0Ch]
	W	EQU	[bp+0Ah]
	Y	EQU	[bp+08h]
	X	EQU	[bp+06h]

		Hide_Mouse

		push	bp
		mov	bp, sp
		push	ds
		push	di
		push	si
		sub	ax, ax
		mov	bx, ax
		mov	cx, bx
		mov	dx, cx
	
		mov	dl, _ScreenWidth
		mov	al, Y
		mov	bl, X	 
		dec	al
		dec	bl	 
		mul	dl
		add	ax, bx
		shl	ax, 1
		les	di, Destination
		lds	si, _ScreenArea
		add	si, ax
		mov	bl, W
		mov	bh, H
		sub	dl, bl
		shl	dx, 1
    @@next_row:
		mov	cl, bl
                rep	movsw
		add	si, dx
		dec	bh
		jne	@@next_row

		pop	si
		pop	di
		pop	ds
		pop	bp

		Show_Mouse
		retf
_save_window	ENDP



_load_window	PROC

     Source	EQU	[bp+0Eh]
	H	EQU	[bp+0Ch]
	W	EQU	[bp+0Ah]
	Y	EQU	[bp+08h]
	X	EQU	[bp+06h]

		Hide_Mouse

		push	bp
		mov	bp, sp
		push	ds
		push	di
		push	si
		sub	ax, ax
		mov	bx, ax
		mov	cx, bx
		mov	dx, cx
	
		mov	dl, _ScreenWidth
		mov	al, Y
		mov	bl, X	 
		dec	al
		dec	bl	 
		mul	dl
		add	ax, bx
		shl	ax, 1
		les	di, _ScreenArea
		lds	si, Source
		add	di, ax

		mov	bl, W
		mov	bh, H
		sub	dl, bl
		shl	dx, 1
		
    @@next_row:
		mov	cl, bl
                rep     movsw
		add	di, dx
		dec	bh
		jne	@@next_row
				
		pop	si
		pop	di
		pop	ds
		pop	bp

		Show_Mouse
		retf
_load_window ENDP


;----------------------------------------------------------------


_clear_window	PROC

	H	EQU	[bp+0Eh]
	W	EQU	[bp+0Ch]
	Y	EQU	[bp+0Ah]
	X	EQU	[bp+08h]
    Attribute	EQU	[bp+06h]

		Hide_Mouse

		push	bp
		mov	bp, sp
		push	ds
		push	di
		push	si
		sub	ax, ax
		mov	bx, ax
		mov	cx, bx
		mov	dx, cx
	
		mov	dl, _ScreenWidth
		mov	al, Y
		mov	bl, X	 
		dec	al
		dec	bl	 
		mul	dl
		add	ax, bx
		shl	ax, 1
		les	di, _ScreenArea
		add	di, ax

		mov	bl, W
		mov	bh, H
		sub	dl, bl
		shl	dx, 1
		
		mov	ah, Attribute
		mov	al, ' '

    @@next_row:
		mov	cl, bl
                rep     stosw
		add	di, dx
		dec	bh
		jne	@@next_row
				
		pop	si
		pop	di
		pop	ds
		pop	bp

		Show_Mouse
		retf
_clear_window	ENDP



_scroll_window	PROC

       Len	EQU	[bp+10h]
	H	EQU	[bp+0Eh]
	W	EQU	[bp+0Ch]
	Y	EQU	[bp+0Ah]
	X	EQU	[bp+08h]
    Attribute	EQU	[bp+06h]

		Hide_Mouse

		push	bp
		mov	bp, sp
		push	ds
		push	di
		push	si
		sub	ax, ax
		mov	bx, ax
		mov	cx, bx
		mov	dx, cx
	
		mov	dl, _ScreenWidth
		les	di, _ScreenArea
		lds	si, _ScreenArea
		mov	cl, dl
		sub	dl, W
		shl	dx, 1
	
		mov	al, Len
		imul	cl
		shl	ax, 1
		add	si, ax
		or	ax, ax
	
		mov	al, Y
		mov	ah, X
		mov	bl, H
		mov	bh, W
		jns	@@n12

		std
		neg	Byte Ptr Len
		neg	dx
		add	ax, bx
		sub	ax, 0101h
    @@n12:
		sub	ax, 0101h
		xchg	ah, cl
		mul	ah
		add	ax, cx
		shl	ax, 1
		add	si, ax
		add	di, ax
		sub	bl, Len
	
    @@next_row:
		mov	cl, bh
                rep     movsw
		add	di, dx
		add	si, dx
		dec	bl
		jne	@@next_row

		mov	bl, Len
		mov	ah, Attribute
		mov	al, ' '

    @@clr_row:
		mov	cl, bh
                rep     stosw
		add	di, dx
		dec	bl
		jne		@@clr_row

		cld

		pop	si
		pop	di
		pop	ds
		pop	bp

		Show_Mouse
		retf
_scroll_window	ENDP



_border_window	PROC

      Border	EQU	[bp+10h]
	 H	EQU	[bp+0Eh]
	 W	EQU	[bp+0Ch]
	 Y	EQU	[bp+0Ah]
	 X	EQU	[bp+08h]
     Attribute	EQU	[bp+06h]

		Hide_Mouse

		push	bp
		mov	bp, sp
		push	ds
		push	di
		push	si
		sub	ax, ax
		mov	bx, ax
		mov	cx, bx
		mov	dx, cx
	
		mov	dl, _ScreenWidth
		mov	al, Y
		mov	bl, X	 
		dec	al
		dec	bl	 
		mul	dl
		add	ax, bx
		shl	ax, 1
		les	di, _ScreenArea
		lds	si, Border
		add	di, ax

		mov	bl, W
		mov	bh, H
		sub	dl, bl
		shl	dx, 1
		sub	bx, 0202h
		
		mov	ah, Attribute

		lodsb			; Upper row
		stosw
		mov	cl, bl
		lodsb
                rep     stosw
		lodsb
		stosw
		add	di, dx
		cmp	bh, 00
		je	@@NoMiddleRows
	
    @@next_row:
		lodsb			; All rows in the middle
		stosw
		mov	cl, bl
		lodsb
		cmp	al, 00
		je	@@NoFill
                rep     stosw
		jmp	@@FillDone
    @@NoFill:
		add	di, cx
		add	di, cx
    @@FillDone:
		lodsb
		stosw
		add	di, dx
		sub	si, 03
		dec	bh
		jne	@@next_row

 @@NoMiddleRows:
		add	si, 03		; Bottom row
		lodsb
		stosw
		mov	cl, bl
		lodsb
                rep     stosw
		lodsb
		stosw

		pop	si
		pop	di
		pop	ds
		pop	bp

		Show_Mouse
		retf
_border_window	ENDP


;----------------------------------------------------------------


_get_event	PROC

  flags		EQU	Word Ptr [bp+0Ah]
  event		EQU	[bp+06h]

 ev_type	EQU	Word Ptr es:[si]

 key		EQU	Word Ptr es:[si+2]
 scan		EQU	Word Ptr es:[si+4]
 shift		EQU	Word Ptr es:[si+6]
 shiftX		EQU	Word Ptr es:[si+8]

 x          EQU Word Ptr es:[si+10]
 y          EQU Word Ptr es:[si+12]
 left		EQU	Word Ptr es:[si+14]
 right		EQU	Word Ptr es:[si+16]
 middle		EQU	Word Ptr es:[si+18]
 wheel      EQU Word Ptr es:[si+20]

 timer1     EQU Word Ptr es:[si+22]
 timer2     EQU Word Ptr es:[si+24]

 EV_KEY		EQU	 1
 EV_SHIFT	EQU	 2
 EV_MOUSE	EQU	 4
 EV_TIMER	EQU	 8
 EV_NONBLOCK	EQU	16

		push	bp
		mov	bp, sp
		push	ds
		push	es
		push	si
		push	di
		les	si, event
		sub	ax, ax
		mov	ev_type, ax
		mov	key, ax
		mov	scan, ax

		test	flags, EV_TIMER
		jz	@@main_loop
		mov	ah, 0h
		int	1Ah
		mov	OriginalTimer1, dx
		mov	OriginalTimer2, cx

    @@main_loop:
		test	flags, EV_KEY
		jz	@@test_shifts

                mov     ah, 1h                  ; Check for a key
                add     ah, ExtendedKeyb
		int	16h
		jz	@@test_shifts

		or	ev_type, EV_KEY		; Key was pressed
                mov     ah, 0h
                add     ah, ExtendedKeyb
		int	16h
		mov	scan, ax
		cmp	al, 0E0h		; Is it an extended key
		jne	@@normal_key
		cmp	ah, 0
		je	@@normal_key
		mov	al, 0
    @@normal_key:
		mov	ah, 0
		mov	key, ax
		jmp	@@break_out

    @@test_shifts:
		test	flags, EV_SHIFT
		jz	@@test_mouse

                mov     ah, 2h                  ; Check if shifts changed
                add     ah, ExtendedKeyb
		int	16h
		cmp	ax, _LastKeyShifts
		jz	@@test_mouse

		or	ev_type, EV_SHIFT
		mov	bx, _LastKeyShifts
		xor	bx, ax
		mov	shift, ax
		mov	shiftX, bx
		mov	_LastKeyShifts, ax
		jmp	@@break_out2

    @@test_mouse:
		test	flags, EV_MOUSE
		jz	@@test_time
		cmp	_MouseInstalled, 1
		jne	@@test_time

		mov	ax, 0003h		; Check mouse status
		int	33h
		cmp	bx, _LastMouseBtns
		jne	@@mouse
		cmp	cx, _LastMousePosX
		jne	@@mouse
		cmp	dx, _LastMousePosY
		je	@@test_time
    @@mouse:
		or	ev_type, EV_MOUSE

		mov	_LastMouseBtns, bx
		mov	_LastMousePosX, cx
		mov	_LastMousePosY, dx
		jmp	@@break_out
    @@test_time:
		test	flags, EV_TIMER
		jz	@@loop_tail

		mov	ah, 0h			; Check timer
		int	1Ah
		sub	dx, OriginalTimer1
		sbb	cx, OriginalTimer2
		cmp	cx, timer2
		jb	@@loop_tail
		cmp	dx, timer1
		jb	@@loop_tail
		or	ev_type, EV_TIMER
		jmp	@@break_out
    @@loop_tail:
		test	flags, EV_NONBLOCK
                jnz     @@break_out
		jmp	@@main_loop

    @@break_out:
                mov     ah, 2h          ; Update shift status
                add     ah, ExtendedKeyb
		int	16h
		mov	shift, ax
		mov	bx, _LastKeyShifts
		xor	bx, ax
		mov	shiftX, bx

    @@break_out2:
    		cmp	_MouseInstalled, 1
    		jne	@@end

		mov	ax, 0003h	; Update mouse status
		int	33h

		shr	cx, 1
		shr	cx, 1
		shr	cx, 1
		inc	cx
		mov	x, cx
		shr	dx, 1
		shr	dx, 1
		shr	dx, 1
		inc	dx
		mov	y, dx

        test    bl, 01h
		jne	@@left
		mov	word ptr left, 0h
		jmp	@@leftE
	@@left:
		mov	word ptr left, 1h
	@@leftE:
        test    bl, 02h
		jne	@@right
		mov	word ptr right, 0h
		jmp	@@rightE
	@@right:
		mov	word ptr right, 1h
	@@rightE:
        test    bl, 04h
		jne	@@middle
		mov	word ptr middle, 0h
		jmp	@@middleE
	@@middle:
		mov	word ptr middle, 1h
	@@middleE:
    ; Wheel counter - RP 2004:
        mov bl, bh
        xor bh, bh
        mov word ptr wheel, bx


	@@end:
		pop	di
		pop	si
		pop	es
		pop	ds
		pop	bp
		retf
_get_event	ENDP
END
