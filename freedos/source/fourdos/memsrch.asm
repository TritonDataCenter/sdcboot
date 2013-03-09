

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


	title	MEMSRCH - Counted string search support for C
	;
	page	,132		;make wide listing
	;
	comment	}

	Copyright 1989 - 1997 JP Software Inc., All Rights Reserved

	Author:  Tom Rawson  1/31/89
	         Revised 7/4/97

	This routine searches for some text in a string.

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
	; MemSrch - Search for text in memory
	;
	; On entry:
	;	Arguments passed via pascal calling convention:
	;
	;	char *text	area to search
	;	int textlen	length of search area
	;	char *pattext	pattern to search for
	;	int patlen	number of characters in pattern
	;
	; On exit:
	;	[E]AX = address of text if found, 0 if not
	;	[E]BX, [E]CX destroyed
	;	All other registers and interrupt state unchanged

	ife	_FLAT
MemSrch	proc	USES bx cx si di es, TextAdr:PCHAR, TextLen:WORD,
		PatAdr:PCHAR, PatLen:WORD
	else
MemSrch	proc	USES ebx ecx esi edi es, TextAdr:PCHAR, TextLen:DWORD,
		PatAdr:PCHAR, PatLen:DWORD
	endif

	loadseg	es,ds,ax		;point es to data segment
	mov	xdi,TextAdr	;point to text to search
	mov	xdx,xdi		;copy address
	add	xdx,TextLen	;get end address + 1
	mov	xbx,PatLen 	;get pattern length
	dec	xbx		;get pattern length after first char
	mov	xsi,PatAdr 	;point to pattern
	lodsb			;get first character
	;
Find1:	mov	xcx,xdx		;copy end address + 1
	sub	xcx,xdi		;get remaining length
	jcxz	NotFound		;if no more characters it's not there
	repne	scasb		;look for first character
	jne	NotFound		;if not there we're done
	cmp	xcx,xbx		;room for pattern?
	jb	NotFound		;no, we're done
	mov	xcx,xbx		;get remaining pattern length
	jxcxz	Found		;if no more characters it's there
	pushm	xsi,xdi		;save si, di
	repe	cmpsb		;compare pattern to text
	popm	xdi,xsi		;restore si, di
	jne	Find1		;if not equal try again
	;
Found:	mov	xax,xdi		;found, return address
	dec	xax		;back up to first character
	jmp	short Return	;all done
	;
NotFound: xor	xax,xax		;not found, return zero
	;
Return:	ret
	;
MemSrch	endp
	;
	end

