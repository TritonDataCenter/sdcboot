

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


	title	ERRORMSG - Get an error message
	;
	page	,132		;make wide listing
	;
	comment	}

	Copyright 1988, 1989, 1990, J.P. Software, All Rights Reserved

	Author:  Tom Rawson  11/12/88, rewritten 2/90

	This routine scans an error message table, and copies the error
	message found to the caller's buffer.  It also performs decom-
	pression on the message text

	} end description

	;
	; Includes
	;
	include	product.asm	;product / platform definitions
	include	trmac.asm 	;general macros
	;
	if	_4DOS		;define loader segment for 4DOS only
	.cseg	LOAD		;set loader segment if not defined
				;  externally
	endif
	if	_WIN or _4NT
	.model	medium
	else
	include	model.inc 	;Spontaneous Assembly memory models
	endif
	;
	;
	;	ErrTable:  Gives the header structure for an error
	;	message table.
	;
ErrTable	struc			;error message table
	ife	_FLAT
BadMsg	dw	0		;offset of message to display when
				;  error message is not found
	else
BadMsg	dd	0		;offset of message to display when
				;  error message is not found
	endif
EncFlag	db	?		;encryption flag
TransTab	db	30 dup (?)	;decompression translate table
TxtStart	db	?		;first byte of text
ErrTable	ends			;error message table
	;
	;
	if	_WIN eq 16
	.code	ASM_TEXT
	else
	ife	_FLAT
	.defcode	,1		;set up code segment, no DGROUP
	else
CODE32	segment
	endif
	endif
	;
;	extrn	SEncrypt:far	;stream decrypter
	;
	assume	cs:@curseg, ds:nothing, ss:nothing, es:nothing  
	;
	;
	; ERRMSG - Retrieve an error message
	;
	; On entry:
	;	AL = error number
	;	DS:SI = error table address
	;	ES:DI = output address
	;
	; On exit:
	;	AX, BX, DX, SI destroyed
	;	CX = length of message copied
	;	ES:DI = last output byte copied + 1
	;	Carry set if message not found, clear if found (if
	;	  message was not found, the undefined message is
	;	  copied)
	;	All other registers and interrupt state unchanged
	;
	;
	if	(_4OS2 eq 16) or _TCMDOS2
	entry	ErrMsg,noframe,far
	else
	if	_WIN
	entry	ErrMsg,noframe
	else
	ife	_FLAT
	.public	ErrMsg		;create public name, with prefix
	.proc	ErrMsg,far	;set up procedure
	else
	entry	ErrMsg,noframe
	endif
	endif
	endif
	;
;	sub	[xsi].EncFlag,084h	;check for encryption
;	 ja	NEncrypt		;if translation table not currently
;				;  encrypted, go on
;	lea	xax,KeyBuf	;point to the key
;	lea	xdx,[xsi].TransTab	;point to translation table
;	mov	xcx,30		;get length
;	call	Decrypt		;decrypt the translation table
;	add	[xsi].EncFlag,60h	;show it's decrypted (sets flag
				;  to 90h)
	;
	cld			;all moves go forward
	;
NEncrypt: mov	xbx,xsi		;copy table address
	add	xsi,TxtStart	;skip translation table and bad error
				;  number message address
	mov	dl,al		;copy desired message number
	;
MsgLoop:	lodsb			;get this message number
	cmp	al,0FFh		;last message?
	 je	short NotFound	;yes, error
	mov	dh,al		;save message number
	lodsw			;get length
	mov	cx,ax		;get in cx
	if	_FLAT
	movzx	ecx,cx		;clear high part
	endif
	cmp	dh,dl		;is this what we want?
	 je	short FoundMsg	;yes, go on
	inc	xcx		;bump for roundoff
	shr	xcx,1		;make it a byte count
	add	xsi,xcx		;skip text
	jmp	short MsgLoop	;keep trying
	;
NotFound: mov	xsi,[xbx].BadMsg	;not found - use undefined error msg
	lodsb			;get message length
	xor	xcx,xcx		;clear high parts
	mov	cl,al		;get length in cl
	push	xcx		;save length
	rep	movsb		;copy message to caller's buffer
	pop	xcx		;get back length
	stc			;show error
	jmp	short EMDone	;and return
	;
FoundMsg: push	xdi		;save output base
	xor	dh,dh		;clear nibble flag
	add	xbx,(TransTab - 1)	;get table address, adjusted for
				;  offset of 1 in nibble decoding
	;
XlatLoop: call	Nibble		;get a nibble
	dec	al		;see what kind it is
	 jl	short XlatByte	;was 0, full byte coming
	 jg	short XlatTab	;was > 1, go right to table (AL now
				;  ranges from 1 - 14), so it will
				;  reference table elements 0 - 13
	call	Nibble		;get next nibble
	add	al,15		;add high table offset (AL now
				;  ranges from 15 - 30), so it will
				;  reference table elements 14 - 29
	;
XlatTab:	xlatb			;load the translated value
	jmp	short XlatOut	;and go on
	;
XlatByte: call	Nibble		;get low nibble
	mov	dl,al		;save it
	call	Nibble		;get high nibble
	rorn	al,4		;swap nibbles in AL
	or	al,dl		;put low nibble in AL
	;
XlatOut:	cmp	al,4		;is it a ctrl-D?
	 jne	XlatCkCR		;if not go on
	mov	al,LF		;if so convert to LF
	;
XlatCkCR:	cmp	al,5		;is it a ctrl-E?
	 jne	XlatStor		;if not go on
	mov	al,CR		;if so convert to CR
	;
XlatStor: stosb			;store the byte
	or	xcx,xcx		;check nibble count
	 jnz	short XlatLoop	;back for more
	;
	mov	xcx,xdi		;copy output end
	pop	xax		;get back base
	sub	xcx,xax		;get message length
	clc			;show message found
	;
EMDone:	ret			;all done

	if	_WIN or _4NT
ErrMsg	endp
	else
	.endp	ErrMsg		;that's all
endif
	;
	;
	; NIBBLE - Get next nibble
	;
	; On entry:
	;	AH = byte in process saved from previous call
	;	XCX = nibble counter
	;	DH = nibble flag saved from previous call
	;	DS:SI = message input pointer
	;
	; On exit:
	;	AL = new nibble (low 4 bits)
	;	AH = updated byte in process
	;	DH = updated nibble flag
	;	XCX = updated nibble counter
	;	DS:SI = updated message input pointer
	;	All other registers and interrupt state unchanged
	;
	entry	Nibble,noframe,,local  ;get next nibble
	mov	al,ah		;copy previous nibble
	or	dh,dh		;need a new byte?
	 jnz	short NibOld	;no, go on
	lodsb			;get new byte
	;
NibOld:	rorn	al,4		;rotate current byte 4 bits
	mov	ah,al		;save byte
	and	al,0Fh		;isolate new nibble
	xor	dh,1		;toggle nibble flag
	dec	xcx		;and reduce count
	exit			;that's all
	;
	;
	; DECRYPT - Decrypt something
	;
	; On entry:
	;	AX = key address, 0 for no new key
	;	DX = string address
	;	CX = length to decrypt
	;
	; On exit:
	;	All registers and interrupt state unchanged
	;
	entry	Decrypt,noframe,,local  ;decrypt a string
;	pushm	xsi,xdi		;save registers
;	mov	xsi,xax		;copy key address
;	mov	xdi,xdx		;copy string address
;	call	SEncrypt		;decrypt the string
;	popm	xdi,xsi		;save registers
	exit			;that's all
	;
@curseg	ends
	;
	end

