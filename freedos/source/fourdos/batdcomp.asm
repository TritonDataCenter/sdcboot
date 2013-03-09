

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


	title	BATDCOMP - Decompress a batch file
	;
	page	,132		;make wide listing
	;
	comment	}

	Copyright 1993, JP Software Inc., All Rights Reserved

	Author:  Tom Rawson  8/27/93

	This routine takes a compressed batch file and decompresses it.

	} end description

	;
	; Includes
	;
	include	product.asm	;product / platform definitions

	if (_4OS2 eq 32) or _TCMDOS2
	include	trmac32.asm	;general 32-bit macros
	else
	include	trmac.asm
	endif

	ife	_4NT
	include	model.inc 	;Spontaneous Assembly memory models
	endif
	;
	;
	; Compressed file header structures
	;
CompHead	struc			;compressed file header
CompSig	dw	?		;signature
EncFlag	dw	?		;encryption flag
ByteCnt	dw	?		;uncompressed bytes
TransTab	db	30 dup (?)	;decompression translate table
TxtStart	db	?		;first byte of text
CompHead	ends
	;
	;
	;
	ife	_FLAT
	.defcode	,1		;set up code segment, no DGROUP
	assume	cs:@curseg, ds:nothing, ss:nothing, es:nothing  
	else
CODE32	segment
	endif
	;
	;
	; BATDCOMP - Decompress a batch file
	;
	; On entry:
	;   Arguments on stack (pascal calling convention):
	;	char far *inbuf	input buffer
	;	char far *outbuf	output buffer
	;	
	; On exit:
	;	AX, BX, CX, DX, ES destroyed
	;	All other registers and interrupt state unchanged
	;
	;
	if	(_4OS2 eq 32) or _TCMDOS2
	entry	BatDComp,varframeC
	else
	entry	_BatDComp,varframeC,far
	endif
	;
	argD	inbuf		;input buffer address
	argD	outbuf		;output buffer address
	;
	varend
	;
	ife	_FLAT
	pushm	xsi,xdi,ds	;save registers used
	lds	xsi,inbuf 	;get 16-bit input buffer address
	les	xdi,outbuf	;get 16-bit output buffer address
	else
	pushm	xbx,xsi,xdi	;save registers used
	mov	xsi,inbuf 	;get 32-bit input buffer address
	mov	xdi,outbuf	;get 32-bit output buffer address
	loadseg	es,ds		;set output segment
	xor	xcx,xcx		;clear high word of counter
	endif
	;
	cld			;all moves go forward
	xor	xdx,xdx		;assume no encryption password offset
	;
	cmp	[xsi].CompSig,0EBBEh  ;new format?
	 je	@F		;if so, go on
	sub	xsi,2		;old format does not have encryption
				;  flag, adjust SI accordingly for
				;  references to TransTab, ByteCnt,
				;  etc.
	jmp	GetTable		;go on
	;
	; New format, check for encryption
	;
@@:	mov	al,bptr [xsi].EncFlag  ;get encryption flag
	mov	ah,al		;copy to AH
	shrn	ah,4,cl		;get left nibble
	and	al,0Fh		;get right nibble
	xor	al,ah		;XOR them
	 jnz	GetTable		;if non-zero, it's not encrypted
	mov	xdx,4		;get encryption password offset
	;
GetTable:	lea	xbx,[xsi].TransTab	;get address of table
	dec	xbx		;adjust for offset of 1 in nibble
				;  decoding
	mov	cx,[xsi].ByteCnt	;get byte count for decompression
	lea	xsi,[xsi].TxtStart	;get address of text
	add	xsi,xdx		;add any encryption password offset
	xor	dh,dh		;clear nibble flag
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
	ror	al,1		;rotate current byte 4 bits
	ror	al,1
	ror	al,1
	ror	al,1
	or	al,dl		;put low nibble in AL
	;
XlatOut:	stosb			;store the byte
	loop	XlatLoop		;check count and loop
	;
	ife	_FLAT
	popm	ds,xdi,xsi	;restore registers
	else
	popm	xdi,xsi,xbx	;restore registers
	endif
	exit			;that's all
	;
	;
	; NIBBLE - Get next nibble
	;
	; On entry:
	;	AH = byte in process saved from previous call
	;	DH = nibble flag saved from previous call
	;	DL = nibble counter
	;	DS:SI = message input pointer
	;
	; On exit:
	;	AL = new nibble (low 4 bits)
	;	AH = updated byte in process
	;	DH = updated nibble flag
	;	DL = updated nibble counter
	;	DS:SI = updated message input pointer
	;	All other registers and interrupt state unchanged
	;
	entry	Nibble,noframe,,local  ;get next nibble
	mov	al,ah		;copy previous nibble
	or	dh,dh		;need a new byte?
	 jnz	short NibOld	;no, go on
	lodsb			;get new byte
	;
NibOld:	ror	al,1		;rotate current byte 4 bits
	ror	al,1
	ror	al,1
	ror	al,1
	mov	ah,al		;save byte
	and	al,0Fh		;isolate new nibble
	xor	dh,1		;toggle nibble flag
	exit			;that's all
	;
	;
@curseg	ends
	;
	end


