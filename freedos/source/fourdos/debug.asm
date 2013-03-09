

;  Permission is hereby granted, free of charge, to any person obtaining a copy
;  of this software and associated documentation files (the "Software"), to deal
;  in the Software without restriction, including without limitation the rights
;  to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
;  copies of the Software, and to permit persons to whom the Software is
;  furnished to do so, subject to the following conditions:
;
;  (1) The above copyright notice and this permission notice shall be included in all
;  copies or substantial portions of the Software.
;
;  (2) The Software, or any portion of it, may not be compiled for use on any
;  operating system OTHER than FreeDOS without written permission from Rex Conn
;  <rconn@jpsoft.com>
;
;  (3) The Software, or any portion of it, may not be used in any commercial
;  product without written permission from Rex Conn <rconn@jpsoft.com>
;
;  (4) THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
;  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
;  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
;  AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
;  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
;  OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
;  SOFTWARE.


          ;
          ; DEBUG - Debug macros include file (with support code)
          ;
          comment   }

          Copyright 1995, J.P. Software, All Rights Reserved

          Author:  Tom Rawson  6/15/95

          This module contains debugging macros for putting traces into
          ASM code.

          } end description
	;
	ifndef	DEBUG
DEBUG	=	0		;default to no debugging
	endif
          ;
	;
	; MACROS
          ;
DBMSG	macro	text		;;literal output
	local	otext,skip	;;labels for jump over text
	if	DEBUG		;;ignore if debugging off
	jmp	skip		;;skip over text
otext	db	text		;;define output text
	db	0		;;end of string
skip:
	push	dx		;;save registers
	push	ds
	push	cs		;;set segment
	pop	ds
	lea	dx,otext		;;set offset
	call	_DBOUT		;;output the string
	pop	ds		;;restore registers
	pop	dx
	endif
	endm
          ;
          ;
DBSTR	macro	reg		;;string output (address in DS:reg)
	if	DEBUG		;;ignore if debugging off
	push	dx		;;save registers
	mov	dx,reg		;;get offset
	call	_DBOUT		;;output the string
	pop	dx		;;restore registers
	endif
	endm
          ;
          ;
DBHEX	macro	reg		;;hex numerical output
	if	DEBUG		;;ignore if debugging off
	push	ax		;;save AX
	mov	ax,reg		;;copy value
	call	_DBHEX		;;output it
	pop	ax		;;restore AX
	endif
	endm
          ;
          ;
DBINT	macro	reg		;;hex numerical output
	if	DEBUG		;;ignore if debugging off
	push	ax		;;save AX
	mov	ax,reg		;;copy value
	call	_DBINT		;;output it
	pop	ax		;;restore AX
	endif
	endm
          ;
          ;
DBCRLF	macro			;;CRLF output
	if	DEBUG		;;ignore if debugging off
	push	dx		;;save registers
	push	ds
	push	cs		;;set segment
	pop	ds
	lea	dx,_DBCRLF	;;get buffer offset
	call	_DBOUT		;;output the string
	pop	ds		;;restore registers
	pop	dx
	endif
	endm
          ;
          ;
DBMSGL	macro	text		;;literal output with CRLF
	DBMSG	text		;;display text
	DBCRLF			;;and CR/LF
	endm
          ;
          ;
DBRESULT	macro	text		;;result output (text, AX value, CRLF)
	DBMSG	text		;;display text
	DBINT	AX		;;display result as integer
	DBCRLF			;;display CR/LF
	endm
	;
	;
	; SUPPORT CODE
	;
	if	DEBUG		;ignore if debugging disabled
	;
_DBBUF	db	6 dup (?)		;hex / decimal output buffer
_DBCRLF	db	0Dh, 0Ah, 0	;CR/LF output buffer
	;
	;
_DBOUT	proc	near		;output ASCIIZ text (address in DX)
	pushf			;save flags
	push	ax		;save registers
	push	bx
	push	cx
	push	di
	push	es
	;
	xor	al,al		;get null
	cld			;go forward
	mov	di,dx		;copy address for scan
	push	ds		;copy segment
	pop	es
	mov	cx,0FFFFh		;max length for scan
	repne	scasb		;find the null
	mov	cx,di		;copy scan end address
	sub	cx,dx		;make length + 1
	dec	cx		;make length
	mov	bx,2		;STDERR
	mov	ah,40h		;write function
	int	21h		;output the text to STDERR
          ;
	pop	es		;restore registers
	pop	di		
	pop	cx
	pop	bx
	pop	ax
	popf			;restore flags
          ret                           ;all done
	;
_DBOUT	endp
	;
	;
_DBHEX	proc	near		;hex output (value in AX)
	pushf			;save flags
	push	ax		;save registers
	push	bx
	push	cx
	push	dx
	push	di
	push	ds
	push	es
	push	cs		;set data = code
	pop	ds
	push	cs
	pop	es
	;
	mov	bx,ax		;copy value
	lea	di,(_DBBUF+4)	;point to last digit + 1
          std                           ;store backwards
	xor	al,al		;get terminating null
	stosb			;store it
	mov	cx,4		;set for 4 more digits
          ;
hexloop:  mov       al,bl               ;copy low byte
          and       al,0Fh              ;get low nibble
          add       al,'0'              ;make it ASCII
          cmp       al,'9'              ;over 9?
          jle       hexput              ;if not go store it
          add       al,'A' - '0' - 10   ;not 0 - 9, make it A - F
          ;
hexput:   stosb                         ;store digit
          shr       bx,1                ;shift to next nibble
          shr       bx,1
          shr       bx,1
          shr       bx,1
          loop      hexloop             ;go back for next digit
	;
	lea	dx,_DBBUF		;point to buffer
	call	_DBOUT		;output the value
          ;
	pop	es		;restore registers
	pop	ds
	pop	di		
	pop	dx
	pop	cx
	pop	bx
	pop	ax
	popf			;restore flags
          ret                           ;all done
	;
_DBHEX	endp
	;
	;
          ;
          ;
divtab    label     word                ;powers of 10 division table
          dw        10
          dw        100
          dw        1000
          dw        10000
	;
_DBINT	proc	near		;integer output (value in AX)
	pushf			;save flags
	push	ax		;save registers
	push	bx
	push	dx
	push	di
	push	ds
	push	es
	push	cs		;set data = code
	pop	ds
	push	cs
	pop	es
	;
	mov	bx,ax		;copy value
	lea	di,_DBBUF		;point to first digit
          cld                           ;output goes forward
	mov	bx,6		;offset to last item in division table
          ;
lzloop:   xor       dx,dx               ;clear high word
          div       divtab[bx]       	;divide
          or        ax,ax               ;check result
          jnz       digloop             ;if not a leading zero go on
          mov       ax,dx               ;remainder to ax
          sub       bx,2                ;move to next divisor
          jge       lzloop              ;keep going with leading zeros
          jmp       short lastdig       ;no more divisors, so only one digit
          ;
digloop:  add       al,'0'              ;make digit ASCII
          stosb                         ;store digit
          mov       ax,dx               ;copy remainder for next round
          sub       bx,2                ;move to next divisor
          jl        lastdig             ;if no more, last digit is in AX
          xor       dx,dx               ;clear high word
          div       divtab[bx]       	;divide again
          jmp       short digloop       ;and go do another digit
          ;
lastdig:  add       al,'0'              ;make last digit ASCII
          stosb                         ;store it
	xor	al,al		;get terminating null
	stosb			;store it
	;
	lea	dx,_DBBUF		;point to buffer
	call	_DBOUT		;output the value
          ;
	pop	es		;restore registers
	pop	ds
	pop	di		
	pop	dx
	pop	bx
	pop	ax
	popf			;restore flags
          ret                           ;all done
	;
_DBINT	endp
	;
	endif
