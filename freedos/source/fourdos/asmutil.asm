

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


	title	ASMUTIL - Assembler utilities for branding
	;
	page	,132		;make wide listing
	;
	comment	}

	Copyright 1997, JP Software Inc., All Rights Reserved

	Author:  Tom Rawson 7/4/97

	} end description
	;
	;
	; Includes
	;
	include	product.asm	;product / platform definitions
	include	trmac.asm 	;general macros
	;
	;
	if	(_DOS ne 0) and (_WIN eq 0)
	.model	small,PASCAL	;small-model for DOS only
	elseif	_FLAT
	.model	flat,PASCAL	;flat is flat
	else
	.model	medium,PASCAL	;everyone else is medium
	endif
	;
	.data			;dummy data segment
	;
	.code			;start code segment
	;
	;
	; Str_Len - Return string length
	;
	; On entry:
	;	DS:[E]SI = string address
	;
	; On exit:
	;	[E]CX = string length
	;	All other registers unchanged
	;
	ife	_FLAT
Str_Len	proc      uses ax di es
	else
Str_Len	proc      uses eax edi es
	endif
	loadseg	es,ds		;set ES for scan
	mov	xdi,xsi		;set DI for scan
	mov	xcx,-1		;init counter to 65535
	xor	al,al		;setup to scan for a NULL
	repne	scasb		;scan for it
	not	xcx		;fix the counter
	dec	xcx		;don't count the NULL
	ret
Str_Len	endp
	;
	;
	; Str_PBrk - Return pointer to first character in a
	;            string that IS in a test character set
	;
	; On entry:
	;	DS:[E]SI = string address
	;	DS:[E]DI = test set address
	;
	; On exit:
	;	Carry clear:
	;	  Character from test set found
	;	  [E]SI = pointer to character found
	;	Carry set:
	;	  Character from test set not found
	;	  [E]SI = pointer to character found
	;	All other registers unchanged
	;
	ife	_FLAT
Str_PBrk	proc	uses ax bx cx dx di
	else
Str_PBrk	proc	uses eax ebx ecx edx edi
	endif
	;
	push	xsi		;save address in case of failure
	xchg	xsi,xdi		;swap strings
	call	Str_Len		;get length of char set string
	xchg	xsi,xdi		;swap back
	mov	xbx,xcx		;save length
	mov	xdx,xdi		;save pointer to start of set

SPGet:	lodsb			;get next char from string
	cmp	al,1		;if null, we're done -- compare to 
	 jb	SPError		;  1 sets carry
	mov	xdi,xdx		;restart from start of set
	mov	xcx,xbx		;get set length
	repne	scasb		;scan the set
	 jne	SPGet		;if not found, continue
	dec	xsi		;back up to match
          clc                           ;show we found one
	pop	xax		;remove saved pointer on stack
	jmp	SPDone		;and get out
	;
SPError:	pop	xsi		;retore string pointer
	;
SPDone:	ret			;all done
	;
Str_PBrk	endp
	;
	;
	; Str_PBrkN - Return pointer to first character in a
	;             string that is NOT in a test character
	;             set
	;
	; On entry:
	;	DS:[E]SI = string address
	;	DS:[E]DI = test set address
	;
	; On exit:
	;	Carry clear:
	;	  Character found in string that is not in test set
	;	  [E]SI = pointer to first character found
	;	Carry set:
	;	  All characters in string match test set
	;	  [E]SI unchanged
	;	All other registers unchanged
	;
	ife	_FLAT
Str_PBrkN	proc	uses ax bx cx dx di
	else
Str_PBrkN	proc	uses eax ebx ecx edx edi
	endif
	;
	push	xsi		;save address in case of failure
	xchg	xsi,xdi		;swap strings
	call	Str_Len		;get length of char set string
	xchg	xsi,xdi		;swap back
	mov	xbx,xcx		;save length
	mov	xdx,xdi		;save pointer to start of set

SPGet:	lodsb			;get next char from string
	cmp	al,1		;if null, we're done -- compare to 
	 jb	SPError		;  1 sets carry
	mov	xdi,xdx		;restart from start of set
	mov	xcx,xbx		;get set length
	repne	scasb		;scan the set
	 je	SPGet		;if we found it, continue
	dec	xsi		;back up to mismatch
          clc                           ;show we found a mismatch
	pop	xax		;remove saved pointer on stack
	jmp	SPDone		;and get out
	;
SPError:	pop	xsi		;retore string pointer
	;
SPDone:	ret			;all done
	;
Str_PBrkN	endp
	;
	;
	; IsDigit - Check if a character is a digit
	;
	; On entry:
	;	AL = character to test
	;
	; On exit:
	;	Zero flag set (for JE) if character is a digit,
	;	  clear if not
	;	All other registers unchanged
	;
IsDigit	proc
	;
	cmp	al,'9'		;set NE if over 9
	 ja	IDDOne		;if over get out
	cmp	al,'0'		;set NE if under 0
	 jb	IDDone		;if under get out
	cmp	al,al		;it's a digit, set EQ
IDDone:	ret
	;
IsDigit	endp
	;
	;
	; StrTo32 - Convert decimal string to 32-bit integer
	;
	; On entry:
	;	DS:SI = string address
	;
	; On exit:
	;	ZF set (for JE) if successful, clear if no digits found
	;	16-bit:  DX:AX = dword value
	;	32-bit:  EAX = dword value
	;	[E]SI = address of first non-digit in string
	;	All other registers unchanged
	;
	ife	_FLAT
StrTo32	proc	uses bx cx di
	local	DigFlag:BYTE
	;
	mov	DigFlag,0		;clear digit found flag
	xor	dx,dx		;clear high part
	xor	bx,bx		;clear low part
	mov	cx,10		;set multiplier
	;
S3Get:	lodsb			;get next character
	xor	ah,ah		;clear high byte
	sub	al,'0'		;less than 0?
	 jb	S3Done		;if so we're done
	cmp	al,9		;greater than 9?
	 ja	S3Done		;if not, we're done
	xor	ah,ah		;clear high byte
	push	ax		;save digit
	mov	ax,dx		;get high part
	mul	cx		;multiply by 10
	mov	di,ax		;save high part * 10
	mov	ax,bx		;get low part
	mul	cx		;get low part * 10 in DX:AX
	add	dx,di		;add high part * 10
	pop	bx		;get new digit
	add	ax,bx		;add it in
	adc	dx,0		;add any carry
	mov	bx,ax		;save new low part
	mov	DigFlag,1		;mark digit found
	jmp	S3Get		;get next char
	;
S3Done:	dec	si		;back up to last char loaded
	mov	ax,bx		;get back low word in AX
	cmp	DigFlag,1		;set return condition
	;
	ret
	;
StrTo32	endp
	;
	else
StrTo32	proc	uses ebx ecx edx
	;
	xor	cl,cl		;clear digit found flag
	xor	ebx,ebx		;clear accumulating value
	;
S3Get:	lodsb			;get next character
	xor	ah,ah		;clear high byte
	sub	al,'0'		;less than 0?
	 jb	S3Done		;if so we're done
	cmp	al,9		;greater than 9?
	 ja	S3Done		;if not, we're done
	movzx	eax,al		;clear high part
	xchg	ebx,eax		;swap digit and value
	mov	edx,10		;set multiplier
	mul	edx		;slide value over
	add	ebx,eax		;add it back to digit
	mov	cl,1		;mark digit found
	jmp	S3Get		;get next char
	;
S3Done:	dec	esi		;back up to last char loaded
	cmp	cl,1		;set return condition
	mov	ax,bx		;get back low word in AX
	;
	ret
	;
StrTo32	endp
	endif
	;
	;
	; Mul6432 - Multiply a 64-bit value by a 32-bit multiplier
	;           and return the 64-bit result
	;
	; Adapted from similar routine in Spontaneous Assembly package.
	; Used under license; source code may not be redistributed.
	;
	; On entry:
	;	DS:DI = 64-bit value address
	;	CX:BX = 32-bit multiplier
	;
	; On exit:
	;	Result stored in 64-bit data area
	;	All registers unchanged
	;
Mul6432	proc	uses ax bx cx dx si di
	local	Temp:WORD, _BX:WORD, _CX:WORD

	mov	_BX,bx		;save original BX
	mov	_CX,cx		;save original CX
	xor	bx,bx		;BX = result of mul2 for this word (0)
	mov	Temp,bx		;TEMP = high word from prior mul2 (0)
	xor	si,si		;SI = high word from prior mul1 (0)
	mov	cx,4		;CH = prev CF (0), CL = loop count (4)
MULoop:	mov	ax,[xdi]		;get next word from quadword
	mul	_BX		;mul next word by mul1 word
	shl	ch,1		;get CF from last ADD (bit 7)
	adc	ax,si		;add in high word from prior mul1
	rcr	ch,1		;save CF from this ADD (bit 7)
	mov	si,dx		;SI saves high word from this mul1
	shr	ch,1		;get CF from last ADD (bit 0)
	adc	ax,bx		;add prior comb word into result w/CF
	xchg	[xdi],ax		;store result low word, AX = qwrd word
	rcl	ch,1		;save CF from this ADD (bit 0)
	dec	cl		;all 4 words of result generated?
	 jz	MUDone		;if so we're done (skip last MUL/ADC)
	mov	bx,Temp		;move prior high mul2 into comb word
	mul	_CX		;mul this word by mul2 word
	ror	ch,1		;prepare to access bit 1
	mov	Temp,dx		;save new high word of mul2
	shr	ch,1		;get CF from last ADD (bit 1)
	adc	bx,ax		;combine last high mul2 & this lo mul2
	inc	xdi
	rcl	ch,1		;save CF from this ADD (bit 1)
	inc	xdi		;DI points to next higher word
	rol	ch,1		;restore bits 7,1,0 to their positions
	jmp	MULoop
	;
MUDone:	ret
	;
Mul6432	endp
	;
	end

