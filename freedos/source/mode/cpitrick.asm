%if 0
  Open UPXed (!) CPI files - for the FreeDOS MODE C utility
  Copyright (C) 2003, 2004  Eric Auer <eric@CoLi.Uni-SB.DE>
  [This file itself became part of MODE C on 29apr2004]

  This program is free software; you can redistribute it and/or
  modify it under the terms of the GNU General Public License
  as published by the Free Software Foundation; either version 2
  of the License, or (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, 
  USA. Or check http://www.gnu.org/licenses/gpl.txt on the internet.
%endif

SEGMENT _TEXT ALIGN=4 CLASS=CODE
GLOBAL _cpx2cpi

; int cpx2cpi(void far * buffer), returns 0 plain 1 upxed -1 error
_cpx2cpi:
        push bp
        mov bp,sp
	; no need to save AX, as AX will hold the return value
        push bx
        push cx
        push dx
        push si
        push di
        push es
        push ds
	pushf
	; first argument can be found at BP+4 on stack now

	lds si,[bp+4]	; get that pointer
	cld
	lodsb
	cmp al,-1	; check CPI header (ff,"FONT   ")
	jnz noplaincpi
	lodsw
	cmp ax,'FO'
	jnz noplaincpi
	lodsw
	cmp ax,'NT'
	jnz noplaincpi
	jmp cpi_okay	; it is an uncompressed CPI file! Will be 49100
			; or more likely 58880 bytes in size, I think.

noupxcpi:
	jmp cpi_error

noplaincpi:
	lds si,[bp+4]	; get that pointer
	or si,si	; pointer must be normalized to allow deUPXing
	jnz noupxcpi

	; Next assumption: CPI file got renamed to COM and then
	; UPXed and renamed again. Then it should start with
	; cmp sp,0x100+cpisize+stubsize+stacksize (roughly 0x200+cpisize)
	; ja enoughram
	; int 20
	; enoughram: ... (here follows the "copy decompressor and compressed
	; data to end-of-the segment code, then the UPX header)
	; Binary: 81 fc xx xx 77 02 cd 20 ...
	; Because we have SS/SP somewhere else, we zap the test!

	lodsw
	cmp ax,0xfc81	; cmp sp, ... ("check if enough free RAM")
	jnz noupxcpi	; does not look like an UPXed ".com" file
	lodsw		; ignore the something
	mov al,0xeb	; opcode: jmp short unconditionally
	mov [si],al	; make the test succeed all the time
			; (the code must be: "cmp... jcc okay...")
	mov bx,'UP'
scan_upx:		; find "UPX!" signature
	dec si
	lodsw
	cmp si,100	; first stub part must be smaller than this
	ja noupxcpi	; give up
	cmp ax,bx
	jnz scan_upx
	lodsw
	cmp ax,'X!'
	jnz noupxcpi	; no second try
	lodsw
	cmp al,10
	jb noupxcpi	; not accepting ancient versions
	add si,16+2	; skip over rest of the header plus 1 word
		; first word of compressed data is one of
		; the bitstream words which are mixed with the
		; literal bytes and low-byte-of-distance bytes
		; which are the ingredients of an UPXed data stream
	mov al,0xcb	; opcode for RETF
	mov [si],al	; overwrite first byte of CPI file to make it a
	inc si		; "program" ;-)
	lodsw
	cmp ax,'FO'	; pointer and buffer contents correct?
	jnz noupxcpi	; give up, bad luck.
	
	; header starts with the first byte of the UPX! string and
	; contains in UPX 1.something versions:
	; [4] version (e.g. 0B) [5] format (1 for COM) [6] method (4)
	; [7] compressionlevel (8) [8] adler32u [C] adler32c
	; (32bit checksums of uncompressed / compressed data)
	; [10] sizeu (16bit, e.g. EC00/58880) [12] sizec (16bit)
	; (sizes are 16bit for COM and SYS, 32bit otherwise...)
	; [14] "filter" [15?] headerchecksum [16] compressed data...

	; If version is below 0A, header is 20 instead of 22 bytes
	; for COM, and if version is below 03, it is 24 bytes, But
	; internal version 09 means UPX 0.7x which is very old.
	; Find UPX source code at http://upx.sourceforge.net/ ...
	; Some of the UPX compression libs are closed source.

	push bp
	mov ax,[bp+6]		; get the buffer SEGMENT
	sub ax,byte 0x10	; move to allow offset 0x100
	mov [bp+6],ax		; denormalize pointer to .COM style
	mov word [bp+4],0x100	; denormalize pointer to .COM style
	mov es,ax		; deUPXing stub requires ES=CS
	mov ds,ax		; deUPXing stub requires DS=CS

	call far [bp+4]		; "run the COM file"
		; this will deUPX the CPI and then run the RETF
		; command which we patched into the byte which
		; will become the first byte of the deUPXed ".COM"!

	pop bp
	xor ax,ax
	mov [bp+4],ax	; renormalize pointer
	add word [bp+6], byte 0x10
	lds si,[bp+4]		; get that pointer
	mov byte [si],-1	; restore first byte to FF
	; buffer now contains the contents of the deUPXed CPI file
	
cpi_upxed:
	mov ax,1
	jmp short cpi_fetched
cpi_okay:
	xor ax,ax
	jmp short cpi_fetched
cpi_error:
	mov ax,-1
cpi_fetched:

	popf
        pop ds
        pop es
        pop di
        pop si
        pop dx
        pop cx
        pop bx
        pop bp
        ret	; ret or retf depending on memory model!

