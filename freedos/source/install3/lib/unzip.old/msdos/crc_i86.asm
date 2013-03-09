; Not copyrighted by Christian Spieler, 11 Feb 1996.
;
	TITLE   crc_i86.asm
	NAME    crc_i86
;
; Optimized 8086 assembler version of the CRC32 calculation loop, intended
; for real mode Info-Zip programs (Zip 2.1, UnZip 5.2, and later versions).
; Supported compilers are Microsoft C (DOS real mode) and Borland C(++)
; (Turbo C). Watcom C (16bit) should also work.
; This module was inspired by a similar module for the Amiga (Paul Kienitz).
;
; It replaces the `ulg crc32(ulg c, char *s, extent n)' function
; in crc32.c.
;
; The code in this module should work with all kinds of C memory models
; (except Borland's __HUGE__ model), as long as the following
; restrictions are not violated:
;
; - The implementation assumes that the char buffer is confined to a
;   64k segment. The pointer `s' to the buffer must be in a format that
;   all bytes can be accessed by manipulating the offset part, only.
;   This means:
;   + no huge pointers
;   + char buffer size < 64 kByte
;
; - Since the buffer size argument `n' is of type `size_t' (= unsigned short)
;   for this routine, the char buffer size is limited to less than 64 kByte,
;   anyway. So, the assumption above should be easily fulfilled.
;
;==============================================================================
;
; Do NOT assemble this source if external crc32 routine from zlib gets used.
;
ifndef USE_ZLIB
;
; Setup of amount of assemble time informational messages:
;
ifdef     DEBUG
  VERBOSE_INFO EQU 1
else
  ifdef _AS_MSG_
    VERBOSE_INFO EQU 1
  else
    VERBOSE_INFO EQU 0
  endif
endif
;
; Selection of memory model, and initialization of memory model
; related macros:
;
ifndef __SMALL__
  ifndef __COMPACT__
    ifndef __MEDIUM__
      ifndef __LARGE__
        ifndef __HUGE__
;         __SMALL__ EQU 1
        endif
      endif
    endif
  endif
endif

ifdef __HUGE__
; .MODEL Huge
   @CodeSize  EQU 1
   @DataSize  EQU 1
   Save_DS    EQU 1
   if VERBOSE_INFO
    if1
      %out Assembling for C, Huge memory model
    endif
   endif
else
   ifdef __LARGE__
;      .MODEL Large
      @CodeSize  EQU 1
      @DataSize  EQU 1
      if VERBOSE_INFO
       if1
         %out Assembling for C, Large memory model
       endif
      endif
   else
      ifdef __COMPACT__
;         .MODEL Compact
         @CodeSize  EQU 0
         @DataSize  EQU 1
         if VERBOSE_INFO
          if1
            %out Assembling for C, Compact memory model
          endif
         endif
      else
         ifdef __MEDIUM__
;            .MODEL Medium
            @CodeSize  EQU 1
            @DataSize  EQU 0
            if VERBOSE_INFO
             if1
               %out Assembling for C, Medium memory model
             endif
            endif
         else
;            .MODEL Small
            @CodeSize  EQU 0
            @DataSize  EQU 0
            if VERBOSE_INFO
             if1
               %out Assembling for C, Small memory model
             endif
            endif
         endif
      endif
   endif
endif

if @CodeSize
	LCOD_OFS	EQU	2
else
	LCOD_OFS	EQU	0
endif

IF @DataSize
	LDAT_OFS	EQU	2
else
	LDAT_OFS	EQU	0
endif

ifdef Save_DS
;			(di,si,ds)+(size, return address)
	SAVE_REGS	EQU	6+(4+LCOD_OFS)
else
;			(di,si)+(size, return address)
	SAVE_REGS	EQU	4+(4+LCOD_OFS)
endif

;
; Selection of the supported CPU instruction set and initialization
; of CPU type related macros:
;
ifdef __586
	Use_286_code	EQU	1
	Align_Size	EQU	16	; paragraph alignment on Pentium
        Alig_PARA	EQU	1	; paragraph aligned code segment
else
ifdef __486
	Use_286_code	EQU	1
	Align_Size	EQU	4	; dword alignment on 32 bit processors
        Alig_PARA	EQU	1	; paragraph aligned code segment
else
ifdef __386
	Use_286_code	EQU	1
	Align_Size	EQU	4	; dword alignment on 32 bit processors
        Alig_PARA	EQU	1	; paragraph aligned code segment
else
ifdef __286
	Use_286_code	EQU	1
	Align_Size	EQU	2	; word alignment on 16 bit processors
        Alig_PARA	EQU	0	; word aligned code segment
else
ifdef __186
	Use_186_code	EQU	1
	Align_Size	EQU	2	; word alignment on 16 bit processors
        Alig_PARA	EQU	0	; word aligned code segment
else
	Align_Size	EQU	2	; word alignment on 16 bit processors
        Alig_PARA	EQU	0	; word aligned code segment
endif	;?__186
endif	;?__286
endif	;?__386
endif	;?__486
endif	;?__586

ifdef Use_286_code
	.286
	Have_80x86	EQU	1
else
ifdef Use_186_code
	.186
	Have_80x86	EQU	1
else
	.8086
	Have_80x86	EQU	0
endif	;?Use_186_code
endif	;?Use_286_code

;
; Declare the segments used in this module:
;
if @CodeSize
if Alig_PARA
CRC32_TEXT	SEGMENT  PARA PUBLIC 'CODE'
else
CRC32_TEXT	SEGMENT  WORD PUBLIC 'CODE'
endif
CRC32_TEXT	ENDS
else	;!@CodeSize
if Alig_PARA
_TEXT	SEGMENT  PARA PUBLIC 'CODE'
else
_TEXT	SEGMENT  WORD PUBLIC 'CODE'
endif
_TEXT	ENDS
endif	;?@CodeSize
_DATA	SEGMENT  WORD PUBLIC 'DATA'
_DATA	ENDS
_BSS	SEGMENT  WORD PUBLIC 'BSS'
_BSS	ENDS
DGROUP	GROUP	_BSS, _DATA
if @DataSize
	ASSUME  DS: nothing, SS: DGROUP
else
	ASSUME  DS: DGROUP, SS: DGROUP
endif

if @CodeSize
EXTRN	_get_crc_table:FAR
else
EXTRN	_get_crc_table:NEAR
endif


Do_CRC	MACRO
	mov	bl,al
if @DataSize
	xor	bl,BYTE PTR es:[di]
else
	xor	bl,BYTE PTR [di]
endif
	inc	di
	sub	bh,bh
if Have_80x86
	shl	bx,2
else
	shl	bx,1
	shl	bx,1
endif
	mov	al,ah
	mov	ah,dl
	mov	dl,dh
	sub	dh,dh
	xor	ax,WORD PTR [bx][si]
	xor	dx,WORD PTR [bx+2][si]
	ENDM
;
Do_2	MACRO
	Do_CRC
	Do_CRC
	ENDM
Do_4	MACRO
	Do_2
	Do_2
	ENDM
;

IF @CodeSize
CRC32_TEXT	SEGMENT
	ASSUME	CS: CRC32_TEXT
else
_TEXT	SEGMENT
	ASSUME	CS: _TEXT
endif
; Line 37

;
;ulg crc32(ulg crc,
;    uch *buf,
;    extend len)
;
	PUBLIC	_crc32
if @CodeSize
_crc32	PROC FAR
else
_crc32	PROC NEAR
endif
if Have_80x86
	enter	WORD PTR 0,0
else
	push	bp
	mov	bp,sp
endif
	push	di
	push	si
if @DataSize
;	crc = 4+LCOD_OFS	DWORD (unsigned long)
;	buf = 8+LCOD_OFS	DWORD PTR BYTE (uch *)
;	len = 12+LCOD_OFS	WORD (unsigned int)
else
;	crc = 4+LCOD_OFS	DWORD (unsigned long)
;	buf = 8+LCOD_OFS	WORD PTR BYTE (uch *)
;	len = 10+LCOD_OFS	WORD (unsigned int)
endif
;
if @DataSize
	mov	ax,WORD PTR [bp+8+LCOD_OFS]	;buf
	or	ax,WORD PTR [bp+10+LCOD_OFS]
else
	cmp	WORD PTR [bp+8+LCOD_OFS],0	;buf
endif
	jne	crc_update
	sub	ax,ax				; crc = 0
	cwd
ifndef NO_UNROLLED_LOOPS
	jmp	fine
else
	jmp	SHORT fine
endif
;
crc_update:
	call	_get_crc_table
;  When used with compilers that conform to the Microsoft/Borland standard
;  C calling convention, model-dependent handling is not needed, because
;   _get_crc_table returns NEAR pointer.
;  But Watcom C is different and does not allow one to assume DS pointing to
;  DGROUP. So, we load DS with DGROUP, to be safe.
;if @DataSize
;	push	ds
;	mov	ds,dx
;	ASSUME	DS: nothing
;endif
	mov	si,ax				;crc_table
if @DataSize
	push	ds
	mov	ax,SEG DGROUP
	mov	ds,ax
	ASSUME	DS: DGROUP
endif
;
	mov	ax,WORD PTR [bp+4+LCOD_OFS]	;crc
	mov	dx,WORD PTR [bp+6+LCOD_OFS]
	not	ax
	not	dx
if @DataSize
	les	di,DWORD PTR [bp+8+LCOD_OFS]	;s
	mov	cx,WORD PTR [bp+12+LCOD_OFS]	;n
else
	mov	di,WORD PTR [bp+8+LCOD_OFS]	;s
	mov	cx,WORD PTR [bp+10+LCOD_OFS]	;n
endif
;
ifndef NO_UNROLLED_LOOPS
if Have_80x86
	shr	cx,2
else
	shr	cx,1
	shr	cx,1
endif
	jcxz	No_Fours
;
	align	Align_Size		; align destination of branch
Next_Four:
	Do_4
	loop	Next_Four
;
No_Fours:
if @DataSize
	mov	cx,WORD PTR [bp+12+LCOD_OFS]	;n
else
	mov	cx,WORD PTR [bp+10+LCOD_OFS]	;n
endif
	and	cx,00003H
endif ; NO_UNROLLED_LOOPS
	jcxz	done
;
        align   Align_Size              ; align destination of branch
Next_Byte:
	Do_CRC
	loop	Next_Byte
;
done:
if @DataSize
	pop	ds
;	ASSUME	DS: DGROUP
	ASSUME	DS: nothing
endif
	not	ax
	not	dx
;
fine:
	pop	si
	pop	di
if Have_80x86
	leave
else
	mov	sp,bp
	pop	bp
endif
	ret

_crc32	ENDP

if @CodeSize
CRC32_TEXT	ENDS
else
_TEXT	ENDS
endif
;
endif ;!USE_ZLIB
;
END
