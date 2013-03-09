

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


	title	SENCRYPT - Stream cipher encryption / decryption

	page	,132		;make wide listing
	;
	comment	}

	(C) Copyright 1998	JP Software Inc.
	All Rights Reserved

	Original Author:	Tom Rawson, 11/25/98

	Based on "Applying Stream Encryption" by Warren Ward.
	C/C++ Users Journal, September, 1998, p. 23.

	For complete details and full copyrighht notices see the C
	implementation in STRMENC.C.  DO NOT change the algorithm,
	polynomials, etc. without reviewing STRMENC.C!

	This implementation almost exactly mirrors the C version.  It
	is NOT at all the most efficient assembler implementation, but
	it is much more confusing to hack than an efficient version.

	} end description
	;
	;
	; Includes
	;

	;
	; Includes
	;
	include	product.asm	;product / platform definitions
	include	trmac.asm 	;general macros
	;
	;
	if	_FLAT
	.model	flat,PASCAL	;flat is flat
	else
	.model	medium,PASCAL	;everyone else is medium
	endif
	;
	.data
	;
	; The linear feedback shift registers
	;
SRegA	dd	?
SRegB	dd	?
SRegC	dd	?
	;
	; Default shift register values
	;
SRegDefA	dd	0AFCE1246h
SRegDefB	dd	0990FD227h
SRegDefC	dd	0F39BCD44h
	;
	; The polynomial masks
	;
PMaskA	dd	80000062h
PMaskB	dd	40000020h
PMaskC	dd	10000002h
	;
	; The LFSR "rotate" masks.
	;
RMask0A	dd	07FFFFFFFh
RMask0B	dd	03FFFFFFFh
RMask0C	dd	00FFFFFFFh
RMask1A	dd	080000000h
RMask1B	dd	0C0000000h
RMask1C	dd	0F0000000h
	;
	if	(_DOS ne 0) and (_WIN eq 0)
	.code	SERV_TEXT
	elseif	_WIN eq 16
	.code	ASM_TEXT
	else
	.code
	endif
	;
	;
	; SEncrypt - Stream encryption / decryption
	;
	; On entry:
	;	Arguments on stack:
	;	  XSI = address of key, NULL if using previous key
	;	  XDI = address of encryption string buffer
	;	  XCX = length of encryption string
	;
	; On exit:
	;	XSI, XDI, XCX destroyed
	;	All other registers and interrupt state unchanged
	;
	ife	_FLAT
SEncrypt	proc	USES ax bx dx es
	else
SEncrypt	proc	USES eax ebx edx es
	endif
	;
	ife	_FLAT
	local	TargLen:WORD
	else
	local	TargLen:DWORD
	endif
	local	OutB:BYTE, OutC:BYTE
	;
	cld			;everything goes forward
	;
	; Save input
	;
	push	xdi		;save target address
	mov	TargLen,xcx	;save target length
	;
	; Set the key if requested
	;
	or	xsi,xsi		;null?
	 je	DoString		;if so go on
	;
	mov	xcx,4		;four sets of three bytes to process
KeyLoop:
	lea	xdi,SRegA		;point to first register
	mov	bl,3		;number of registers
KeyLoop2:
	if	_FLAT		;32-bit
	mov	al,[si]		;get byte for LFSR A
	shl	[edi],8		;move LFSR over
	mov	[edi],al		;put in new character
	else			;16-bit
	dload	ax,dx,[di]	;get the register
	mov	dh,dl		;slide high-order left 8
	mov	dl,ah		;slide low order in
	mov	ah,al		;and slide low order left 8
	mov	al,[si]		;get new byte
	dstore	[di],ax,dx	;store the new value
	endif
	;
	add	xdi,4		;skip to next register
	add	xsi,4		;skip to next character
	dec	bl		;count registers
	 jnz	KeyLoop2		;loop for all 3 registers
	;
	sub	xsi,11		;back up to start of key (-12),
				;  but next character (+1)
	loop	KeyLoop		;loop for whole key
	;
	; Check for 0s in the key and default if they're there
	;
	mov	xcx,3		;three registers to process
	lea	xsi,SRegA		;point to first register
	lea	xdi,SRegDefA	;point to first register default
	;
KeyCheck:
	if	_FLAT		;32-bit
	cmp	[esi],0		;check if register is 0
	 jne	KCGood		;if not go on
	mov	eax,[edi]		;get default
	mov	[esi],eax		;store it
	else			;16-bit
	dload	ax,dx,[si]	;get the register
	or	ax,dx		;check if 0
	 jne	KCGood		;if not go on
	dload	ax,dx,[di]	;get default
	dstore	[si],ax,dx	;store it
	endif
	;
KCGood:	add	xsi,4		;skip to next register
	add	xdi,4		;skip to next default
	loop	KeyCheck		;loop for all registers
	;
	; Encrypt or decrypt the string
	;
DoString:	pop	xsi		;get string address
	;
	; Cycle the LFSRs eight times to get eight pseudo-random bits.
	; Assemble these into a single random character in BL.
	;
CharLoop:	mov	cl,8		;bit count
	xor	bl,bl		;clear random character
	mov	al,bptr SRegB	;low byte of LFSRB
	and	al,1		;get low bit
	mov	OutB,al		;save it
	mov	al,bptr SRegC	;low byte of LFSRC
	and	al,1		;get low bit
	mov	OutC,al		;save it
	;
RandLoop:	test	bptr SRegA,1	;test low bit of LFSRA
	 jz	SRA0		;go on if 0
	;
	; The least-significant bit of LFSR A is "1". XOR LFSR A with
	; its feedback mask.
	;
	if	_FLAT
	mov	eax,SRegA		;get the register
	xor	eax,PMaskA	;XOR with mask
	shr	eax,1		;slide it over
	or	eax,RMask1A	;OR in new bit
	mov	SRegA,eax		;save it
	;
	else
	dload	ax,dx,SRegA	;get the register
	xor	ax,wptr PMaskA	;XOR low with mask
	xor	dx,wptr PMaskA+2	;XOR high with mask
	dshr	ax,dx		;slide it over
	or	dx,wptr RMask1A+2	;OR in new bit
	dstore	SRegA,ax,dx	;save it
	endif
	;
	; Clock shift register B once
	;
	test	bptr SRegB,1	;test low bit of LFSRB
	 jz	SRB0		;go on if 0
	;
	; The least-significant bit of LFSR B is "1". XOR LFSR B with
	; its feedback mask.
	;
	if	_FLAT
	mov	eax,SRegB		;get the register
	xor	eax,PMaskB	;XOR with mask
	shr	eax,1		;slide it over
	or	eax,RMask1B	;OR in new bit
	mov	SRegB,eax		;save it
	;
	else
	dload	ax,dx,SRegB	;get the register
	xor	ax,wptr PMaskB	;XOR low with mask
	xor	dx,wptr PMaskB+2	;XOR high with mask
	dshr	ax,dx		;slide it over
	or	dx,wptr RMask1B+2	;OR in new bit
	dstore	SRegB,ax,dx	;save it
	endif
	mov	OutB,1		;save the output bit
	jmp	AddBit		;go add bit to BL
	;
	; The LSB of LFSR B is "0". Rotate the LFSR contents once.
	;
SRB0:
	if	_FLAT
	mov	eax,SRegB		;get the register
	shr	eax,1		;slide it over
	and	eax,RMask0B	;clear new bits
	mov	SRegB,eax		;save it
	;
	else
	dload	ax,dx,SRegB	;get the register
	dshr	ax,dx		;slide it over
	and	dx,wptr RMask0B+2	;clear high bits
	dstore	SRegB,ax,dx	;save it
	endif
	mov	OutB,0		;save the output bit
	jmp	AddBit		;go add bit to BL
	;
	;
	; The LSB of LFSR A is "0".  Rotate the LFSR contents once.
	;
SRA0:
	if	_FLAT
	mov	eax,SRegA		;get the register
	shr	eax,1		;slide it over
	and	eax,RMask0A	;clear new bits
	mov	SRegA,eax		;save it
	;
	else
	dload	ax,dx,SRegA	;get the register
	dshr	ax,dx		;slide it over
	and	dx,wptr RMask0A+2	;clear high bits
	dstore	SRegA,ax,dx	;save it
	endif
	;
	; Clock shift register C once
	;
	test	bptr SRegC,1	;test low bit of LFSRC
	 jz	SRC0		;go on if 0
	;
	; The least-significant bit of LFSR C is "1". XOR LFSR C with
	; its feedback mask.
	;
	if	_FLAT
	mov	eax,SRegC		;get the register
	xor	eax,PMaskC	;XOR with mask
	shr	eax,1		;slide it over
	or	eax,RMask1C	;OR in new bit
	mov	SRegC,eax		;save it
	;
	else
	dload	ax,dx,SRegC	;get the register
	xor	ax,wptr PMaskC	;XOR low with mask
	xor	dx,wptr PMaskC+2	;XOR high with mask
	dshr	ax,dx		;slide it over
	or	dx,wptr RMask1C+2	;OR in new bit
	dstore	SRegC,ax,dx	;save it
	endif
	mov	OutC,1		;save the output bit
	jmp	AddBit		;go add bit to BL
	;
	; The LSB of LFSR C is "0". Rotate the LFSR contents once.
	;
SRC0:
	if	_FLAT
	mov	eax,SRegC		;get the register
	shr	eax,1		;slide it over
	and	eax,RMask0C	;clear new bits
	mov	SRegC,eax		;save it
	;
	else
	dload	ax,dx,SRegC	;get the register
	dshr	ax,dx		;slide it over
	and	dx,wptr RMask0C+2	;clear high bits
	dstore	SRegC,ax,dx	;save it
	endif
	mov	OutC,0		;save the output bit
	;
	; XOR the output from LFSRs B and C and rotate it into the
	; right bit of BL
	;
AddBit:	
	shl	bl,1		;shift random character over
	mov	al,OutB		;get LFSRB output
	xor	al,OutC		;XOR with LFSRC output
	or	bl,al		;add to random character
	dec	cl		;count 8 bits
	 jnz	RandLoop		;do it all 8 times
	;
	; XOR the resulting character with the input character to
	; encrypt/decrypt it [Note (A | B) & (~(A & B)) is an XOR]
	;
	lodsb			;get the character, bump SI
	mov	ah,al		;copy it
	or	al,bl		;string char | random char
	and	ah,bl		;string char & random char
	not	ah		;~(string char & random char)
	and	al,ah		;(string char | random char) &
				;  ~(string char & random char)
				;  = (string char ^ random char)
	mov	[xsi-1],al	;save it
	;
	dec	TargLen		;count string
	 jnz	CharLoop		;loop for next character
	;
SEDone:	ret			;all done
	;
SEncrypt	endp
	;
	end

