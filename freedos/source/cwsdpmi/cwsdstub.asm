; Copyright (C) 2000 DJ Delorie, see COPYING.DJ for details
; Copyright (C) 1995 DJ Delorie, see COPYING.DJ for details
; -*- asm -*-
;
;-----------------------------------------------------------------------------
; CWSDPMI r5 embedded stub.  This stub was original modified from djasm 
;  to tasm syntax by m.grimrath@tu-bs.de for use with PMODE/DJ.  CW Sandmann
;  upgraded it to 2.02 functionality, made the code common.
;-----------------------------------------------------------------------------
;
; KLUDGE-WARNING!
;
; So you say you want to change this file, right?  Are you really sure
; that's a good idea?  Let me tell you a bit about the pitfalls here:
;
; * Some code runs in protected mode, some in real-mode, some in both.
; * Some code must run on a 8088 without crashing it.
; * Registers and flags may be expected to survive for a long time.
; * The code is optimized for size, not for speed or readability.
; * Some comments are parsed by other programs.
;
; You still want to change it?	Oh well, go ahead, but don't come
; crying back saying you weren't warned.
;
;-----------------------------------------------------------------------------
;  djgpp stub loader
;
;  (C) Copyright 1993-1995 DJ Delorie
;
;  Redistribution and use in source and binary forms are permitted
;  provided that: (1) source distributions retain this entire copyright
;  notice and comment, (2) distributions including binaries display
;  the following acknowledgement:  ``This product includes software
;  developed by DJ Delorie and contributors to the djgpp project''
;  in the documentation or other materials provided with the distribution
;  and in all advertising materials mentioning features or use of this
;  software, and (3) binary distributions include information sufficient
;  for the binary user to obtain the sources for the binary and utilities
;  required to built and use it. Neither the name of DJ Delorie nor the
;  names of djgpp's contributors may be used to endorse or promote
;  products derived from this software without specific prior written
;  permission.
;
;  THIS SOFTWARE IS PROVIDED ``AS IS'' AND WITHOUT ANY EXPRESS OR
;  IMPLIED WARRANTIES, INCLUDING, WITHOUT LIMITATION, THE IMPLIED
;  WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
;
;  Revision history:
;
;  93/12/05 DJ Delorie	Initial version v2.00, requires DPMI 0.9
;  94/10/13 CW Sandmann v2.01, accumlated changes: 60K load bug, limits, cwsdpmi, optimization
;  94/10/29 CW Sandmann v2.03, M Welinder changes; cwsdpmi load anywhere, size decrease
;  00/09/29 CW Sandmann v2.04, Optimizations, bug fixes, stub features merge
;
; The copyright is added using an external program (PADSEC or EHDRFIX)
; db "The CWSDSTUB.EXE stub loader is Copyright (C) 1993-1995 DJ Delorie. ",13,10
; db "Permission granted to use for any purpose provided this copyright ",13,10
; db "remains present and unmodified. ",13,10
; db "This only applies to the stub, and not necessarily the whole program.",13,10

	ideal
	p386
	assume cs:_TEXT, ds:DGROUP, es:nothing, ss:nothing, fs:_TEXT
	group DGROUP _DATA, _BSS

	SEGMENT _TEXT PUBLIC USE16 'CODE'

;*****************************************************************************
;  structure definitions
;

coff_magic	= 0			; from coff header
aout_entry	= 16			; from aout header
s_paddr		= 8			; from section headers
s_vaddr		= 12
s_size		= 16
s_scnptr	= 20

stack_sz	= 100h			; bytes - must be a multiple of 16!

	EXTRN	lsetup:near, _main:near

;-----------------------------------------------------------------------------
;  Interface to 32-bit executable:
;
;    cs:eip	according to COFF header
;    ds		32-bit data segment for COFF program
;    fs		selector for our data segment (fs:0 is stubinfo)
;    ss:sp	our stack (ss to be freed)
;    <others>	All unspecified registers have unspecified values in them.
;-----------------------------------------------------------------------------
;  This is the stubinfo structure.  The presence of this structure
;  indicates that the executable is a djgpp v2.00 executable.
;  Fields will never be deleted from this structure, only obsoleted.
;
label stubinfo byte
stubinfo_magic	\		; char [16]
	db "go32stub, v 2.02"	; version may change, [0..7] won't
stubinfo_size  \		; unsigned long
	dw	stubinfo_end,0	; bytes in structure
stubinfo_minstack  \		; unsigned long
	dd	80000h		; minimum amount of DPMI stack space (512K)
stubinfo_memory_handle	\	; unsigned long
	dd	0		; DPMI memory handle
stubinfo_initial_size  \	; unsigned long
	dd	0		; size of initial segment
stubinfo_minkeep  \		; unsigned short
	dw	16384		; amount of automatic real-mode buffer
stubinfo_ds_selector  \		; unsigned short
	dw	0		; our DS selector (used for transfer buffer)
stubinfo_ds_segment  \		; unsigned short
	dw	0		; our DS segment (used for simulated calls)
stubinfo_psp_selector  \	; unsigned short
	dw	0		; PSP selector
stubinfo_cs_selector  \		; unsigned short
	dw	0		; to be freed
stubinfo_env_size  \		; unsigned short
	dw	0		; number of bytes of environment
stubinfo_basename  \		; char [8]
	db	8 dup (0)	; base name of executable to load (asciiz if < 8)
stubinfo_argv0	\		; char [16]
	db	16 dup (0)	; used ONLY by the application (asciiz if < 16)
stubinfo_dpmi_server  \		; char [16]
	db	16 dup (0)	; Not used by CWSDSTUB

	align	4
label stubinfo_end byte

;-----------------------------------------------------------------------------
;  First, set up our memory and stack environment
start:					; execution starts here
	call	lsetup			; CWSDPMI heap setup, DS setup

	mov	[dos_block_seg],es	; save the PSP segment
	cld

;-----------------------------------------------------------------------------
;  Check that we have DOS 3.00 or later.  (We need this because earlier
;  versions don't supply argv[0] to us and will scrog registers on dpmi exec).
	mov	ah, 030h
	int	021h
	cmp	al, 3
	jae	short dos3ok
	mov	al, 109
	mov	dx, offset msg_bad_dos
	jmp	error
dos3ok:
	mov	[dos_major],al

;-----------------------------------------------------------------------------
;  When we are spawned from a program which has more than 20 handles in use,
;  all the handles passed to us by DOS are taken (since only the first 20
;  handles are inherited), and opening the .exe file will fail.
;  Therefore, we forcefully close handles 18 and 19, to make sure at least two
;  handles are available.

	mov	ah, 3Eh
	mov	bx, 19
	int	21h			; don't care about errors
	mov	ah, 3Eh
	mov	bx, 18
	int	21h			; don't care about errors

;-----------------------------------------------------------------------------
;  Get DPMI information before doing anything 386-specific

	xor	bp,bp			; pass number
	jmp	short again
no_dpmi:
	call	_main 			; Here is where set up CWSDPMI
	inc	bp			; Second pass (CWSDPMI will exit)
again:
	mov	ax,1687h		; check for DPMI
	int	2fh
	or	ax,ax			; DPMI present?
	jnz	short no_dpmi
	test	bl,1			; is DPMI 32bit? (clears carry)
	jz	short no_dpmi

	mov	[word ptr modesw], di	; store the DPMI entry point
	mov	[word ptr modesw+2], es
	mov	[modesw_mem], si

	mov	cl,4			; for shifts below
	mov	si, offset stubinfo_minkeep

	or	bp,bp			; first pass?
	jz	short @@dpmi

;-----------------------------------------------------------------------------
; Embedded DPMI is active - allocate a separate transfer buffer
; Minimum TB size is end of code in this module plus stack (around 2.5Kb)
; Data (DS) stays in original image (so extra selector on PM transition)
; We transfer to code copy in TB so CS is in TB which is what DJGPP expects

	mov	bx, offset end_stub_text+stack_sz+15
	and	bl, 0f0h		; round to paragraph
	mov	ax, [cs:si]		; stubinfo_minkeep
	cmp	bx, ax
	ja	short @@w2e
	mov	bx, ax
@@w2e:	mov	[cs:si], bx		; stubinfo_minkeep
	mov	dx, bx
	call	include_umb
	mov	bx, dx
	shr	bx, cl
	mov	ah,48h
	int	21h			; Allocate transbuffer
	call	restore_umb
	jc	error_no_dos_memory

; Copy stubinfo and code to TB; set SS:SP to end of TB; jump to TB as CS
	mov	es, ax
	mov	ss, ax
	mov	sp, dx			; TB size from above

	xor	si, si			; si=0 offset for _TEXT start
	xor	di, di
	mov	cx, offset end_stub_text+1
	shr	cx, 1			; words
	rep movs [word ptr es:di],[word ptr cs:si]

	push	ax			; new cs in TB
	push	offset @@cont		; new EIP in TB
	retf
;	jmp 	short @@cont

;-----------------------------------------------------------------------------
; A DPMI host is active - this loader can be overwritten as TB
; Minimum TB size is end of code and data in this module plus stack
; currently CS = TB; set SS:SP to end of TB

@@dpmi:	mov	bx, offset end_stub_data+511	; Data size in bytes
	shr	bx, cl				; Paragraphs
	add	bx, _DATA + (stack_sz/16)	; Add stack
	sub	bx, _TEXT			; Add (DS-CS) paragraphs
	and	bl, 0e0h			; round to 512 bytes
	
	shl	bx, cl				; Convert back to bytes
	mov	ax, [cs:si]		; stubinfo_minkeep
	cmp	bx, ax
	ja	short @@w2
	mov	bx, ax
@@w2:	mov	[cs:si], bx		; stubinfo_minkeep
	push	cs
	pop	ss
	mov	sp, bx			; Move down on stack
	inc	bh			; Add extra for psp
	shr	bx, cl
	mov	es, [dos_block_seg]
	mov	ah, 04ah
	int	021h			; resize our memory block
	jc	error_no_dos_memory	; should never fail since decrease

@@cont:
	mov	[cs:stubinfo_ds_segment], cs
;-----------------------------------------------------------------------------
;  Scan environment for the stub's full name after environment
	mov	es, [dos_block_seg]
	mov	es, [es:02ch]		; get environment segment
	xor	di, di			; begin search for NUL/NUL (di = 0)
	mov	cx, 0ff04h		; effectively `infinite' loop
	xor	al, al
scan_environment:
	repne scasb			; search for NUL
	scasb
	jne short scan_environment	; no, still environment
	scasw				; adjust pointer to point to prog name

;-----------------------------------------------------------------------------
; copy the filename to loadname
	sub	al,al
	push	di
	mov	cx,-1
	repne	scasb
	not	cx
	mov	[cs:stubinfo_env_size], di
	pop	si
	mov	di,offset loadname

	push	ds
	push	es
	push	ds
	pop	es
	pop	ds
	rep	movsb
	pop	ds

	cmp	[byte ptr cs:stubinfo_basename+0], 0
	je	short no_symlink

	mov	bx,di
@@lp2:	dec	bx
	cmp	[byte ptr bx],'\'	; always succeeds, because filename
	jne	short @@lp2		; is qualified if no debugger
	inc	bx

;-----------------------------------------------------------------------------
;  Replace the stub's file name with the link's name after the directory

	mov	cx, 8			; max length of basename
	mov	di, offset stubinfo_basename ; pointer to new basename
@@b11:
	mov	al, [cs:di]		; get next character
	inc	di
	or	al, al			; end of basename?
	je	short @@f12
	mov	[bx], al		; store character
	inc	bx
	loop	@@b11			; eight characters?
@@f12:
	mov	[dword ptr bx+0], 04558452eh ; append ".EXE"
	mov	[byte ptr bx+4], 0

no_symlink:

;-----------------------------------------------------------------------------
;  Load the COFF information from the file

	mov	ax, 03d00h		; open file for reading
	mov	dx, offset loadname
	int	021h
	jc	error_no_progfile	; do rest of error message

	mov	[program_file], ax	; store for future reference

	mov	bx, ax
	mov	cx, exe_header_length
	mov	dx, offset exe_header
	mov	ah, 03fh		; read EXE header
	int	021h

	xor	dx, dx			; dx = 0
	xor	cx, cx			; offset of COFF header

	mov	ax, [exe_magic]
	cmp	ax, 0014ch		; COFF?
	je	short file_is_just_coff
	cmp	ax, 05a4dh		; EXE magic value
	jne	error_not_exe

	mov	dx, [exe_sectors]
	shl	dx, 9			; 512 bytes per sector
	mov	bx, [exe_bytes_last_page]
	or	bx, bx			; is bx = 0 ?
	je	short @@f13
	sub	dh, 2			; dx -= 512
	add	dx, bx
@@f13:

file_is_just_coff:			; cx:dx is offset
	mov	[word ptr coff_offset+0], dx
	mov	[word ptr coff_offset+2], cx
	mov	ax, 04200h		; seek from beginning
	mov	bx, [program_file]
	int	021h

	mov	cx, coff_header_length
	mov	dx, offset coff_header
	mov	ah, 03fh		; read file (bx = handle)
	int	021h

	cmp	ax, cx			; coff_header_length
	jne	short @@f21
	cmp	[word ptr coff_header + coff_magic], 0014ch
@@f21:
	jne	error_not_coff

	mov	eax, [dword ptr aout_header + aout_entry]
	mov	[start_eip], eax

	mov	ecx, [coff_offset]

	mov	eax, [dword ptr text_section + s_scnptr]
	add	eax, ecx
	mov	[text_foffset], eax

	mov	eax, [dword ptr data_section+ s_scnptr]
	add	eax, ecx
	mov	[data_foffset], eax

	mov	ebx, [dword ptr bss_section + s_vaddr]
	add	ebx, [dword ptr bss_section + s_size]
	mov	eax, 00010001h
	cmp	ebx, eax
	jae	short @@f1
	mov	ebx, eax		; ensure 32-bit segment
@@f1:
	add	ebx, 0000ffffh		; ensure 64K rounded
	xor	bx, bx			; clear rounded bits
	mov	[cs:stubinfo_initial_size], ebx

;-----------------------------------------------------------------------------
;  Set up for the DPMI environment

	call	include_umb
	mov	bx, [modesw_mem]
	or	bx, bx			; or clears carry
	jz	short no_dos_alloc

	mov	ah, 048h		; allocate memory for the DPMI host
	int	021h
	jc	error_no_dos_memory_umb
	mov	es, ax

no_dos_alloc:
	call	restore_umb
	mov	ax, 1			; indicates a 32-bit client
	call	[modesw]		; enter protected mode
	jc	error_in_modesw

;-----------------------------------------------------------------------------
; We're in protected mode at this point.

	mov	[ss:stubinfo_psp_selector],es
	mov	[ss:stubinfo_cs_selector],cs
	mov	[ss:stubinfo_ds_selector],ss
	push	ds
	pop	es

	xor	ax, ax			; AX = 0x0000
	mov	cx, 1
	int	031h			; allocate LDT descriptor
	jc	short @@f2
	mov	[client_cs], ax

	xor	ax, ax			; AX = 0x0000
;	mov	cx, 1			; already set above
	int	031h			; allocate LDT descriptor
@@f2:
	jc	perror_no_selectors
	mov	[client_ds], ax

; Try getting a DPMI 1.0 memory block first, then try DPMI 0.9
; Note:	 This causes the Borland Windows VxD to puke, commented for now with ;*
;*	mov	ax, 0x0504
;*	xor	ebx, ebx		; don't specify linear address
	mov	ecx, [ss:stubinfo_initial_size + 0]
;*	mov	edx, 1			; allocate committed pages
;*	int	0x31			; allocate memory block
;*	jc	try_old_dpmi_alloc
;*	mov	client_memory[0], ebx
;*	mov	ss:stubinfo_memory_handle[0], esi
;*	jmp	got_dpmi_memory
try_old_dpmi_alloc:
	mov	ax, 00501h
	mov	bx, [word ptr ss:stubinfo_initial_size + 2]
;	mov	cx, stubinfo_initial_size[0]		;Set above
	int	031h			; allocate memory block
	jc	perror_no_dpmi_memory
	mov	[word ptr client_memory + 2], bx
	mov	[word ptr client_memory + 0], cx
	mov	[word ptr ss:stubinfo_memory_handle + 2], si
	mov	[word ptr ss:stubinfo_memory_handle + 0], di
got_dpmi_memory:

	mov	ax, 00007h
	mov	bx, [client_cs]		; initialize client CS
	mov	cx, [word ptr client_memory + 2]
	mov	dx, [word ptr client_memory + 0]
	int	031h			; set segment base address

	mov	ax, 00009h
;	mov	bx, [client_cs]		; already set above
	mov	cx, cs			; get CPL
	and	cx, 00003h
	shl	cx, 5
	push	cx			; save shifted CPL for below
	or	cx, 0c09bh		; 32-bit, big, code, non-conforming, readable
	int	031h			; set descriptor access rights

	mov	ax, 00008h
;	mov	bx, [client_cs]		; already set above
	mov	cx, [word ptr ss:stubinfo_initial_size + 2]
	dec	cx
	mov	dx, 0ffffh
	int	031h			; set segment limit

	mov	ax, 00007h
	mov	bx, [client_ds]		; initialize client DS
	mov	cx, [word ptr client_memory + 2]
	mov	dx, [word ptr client_memory + 0]
	int	031h			; set segment base address

	mov	ax, 00009h
;	mov	bx, [client_ds]		; already set above
	pop	cx			; shifted CPL from above
	or	cx, 0c093h		; 32-bit, big, data, r/w, expand-up
	int	031h			; set descriptor access rights

	mov	ax, 00008h
;	mov	bx, [client_ds]		; already set above
	mov	cx, [word ptr ss:stubinfo_initial_size + 2]
	dec	cx
	mov	dx, 0ffffh
	int	031h			; set segment limit

;-----------------------------------------------------------------------------
;  Load the program data

	mov	ax, 00100h
	mov	bx, 00f00h		; 60K DOS block size
	int	031h			; allocate DOS memory
	jnc	short @@f1
	cmp	ax, 00008h
	jne	error_no_dos_memory
	mov	ax, 00100h		; try again with new value in bx
	int	031h			; allocate DOS memory
	jc	error_no_dos_memory
@@f1:
	mov	[dos_block_seg], ax
	mov	[dos_block_sel], dx
	shl	bx, 4			; paragraphs to bytes
	mov	[dos_block_size], bx

	mov	esi, [text_foffset]	; load text section
	mov	edi, [dword ptr text_section + s_vaddr]
	mov	ecx, [dword ptr text_section + s_size]
	call	read_section

	mov	esi, [data_foffset]	; load data section
	mov	edi, [dword ptr data_section + s_vaddr]
	mov	ecx, [dword ptr data_section + s_size]
	call	read_section

	mov	es, [client_ds]		; clear the BSS section
	mov	edi, [dword ptr bss_section + s_vaddr]
	mov	ecx, [dword ptr bss_section + s_size]
	xor	eax,eax
	shr	ecx,2
	rep stos [dword ptr es:edi]

	mov	ah,03eh
	mov	bx, [program_file]
	int	021h			; close the file

	mov	ax, 00101h
	mov	dx, [dos_block_sel]
	int	031h			; free the DOS memory

;  Original Stub code:

;	push	ds
;	pop	fs
;	mov	ds, [client_ds]
;	.opsize
;	jmpf	fs:[start_eip]		; start program

	push	[dword ptr client_cs]
	push	[dword ptr start_eip]

	push	ss
	pop	fs
	mov	bx, ds
	mov	ds, [client_ds]
	mov	ax, 1
	int	31h			; free old DS selector (_DATA)

	db	66h
	retf				; jump far double to start_eip

;-----------------------------------------------------------------------------
;  Read a section from the program file

read_section:
	mov	eax, esi		; sector alignment by default
	and	eax, 01ffh
	add	ecx, eax
	sub	si, ax			; sector align disk offset (can't carry)
	sub	edi, eax		; memory maybe not aligned!

	mov	[read_size], ecx	; store for later reference
	mov	[read_soffset], edi

	call	zero_regs
	mov	[word ptr dpmi_regs + dr_dx], si ; store file offset
	shr	esi, 16
	mov	[word ptr dpmi_regs + dr_cx], si
	mov	bx, [program_file]
	mov	[word ptr dpmi_regs + dr_bx], bx
	mov	[word ptr dpmi_regs + dr_ax], 04200h
	call	pm_dos			; seek to start of data

; Note, handle set above
	mov	ax, [dos_block_seg]
	mov	[word ptr dpmi_regs + dr_ds], ax
	mov	[word ptr dpmi_regs + dr_dx], 0	 ; store file offset
read_loop:
	mov	[byte ptr dpmi_regs + dr_ah], 03fh
	mov	ax, [word ptr read_size + 2]	 ; see how many bytes to read
	or	ax, ax
	jnz	short read_too_big
	mov	ax, [word ptr read_size + 0]
	cmp	ax, [dos_block_size]
	jna	short read_size_in_ax		 ; jna shorter than jmp
read_too_big:
	mov	ax, [dos_block_size]
read_size_in_ax:
	mov	[word ptr dpmi_regs + dr_cx], ax
	call	pm_dos			; read the next chunk of file data

	xor	ecx, ecx
	mov	cx, [word ptr dpmi_regs + dr_ax] ; get byte count
	mov	edi, [read_soffset]	; adjust pointers
	add	[read_soffset], ecx
	sub	[read_size], ecx

	xor	esi, esi		; esi=0 offset for copy data
	shr	cx, 2			; ecx < 64K
	push	ds
	push	es
	mov	es, [client_ds]
	mov	ds, [dos_block_sel]
	rep movs [dword ptr es:edi],[dword ptr ds:esi]
	pop	es
	pop	ds

	add	ecx, [read_size]	; ecx zero from the rep movsd
	jnz	read_loop

	ret

;-----------------------------------------------------------------------------
;  Most errors come here, early ones jump direct (8088 instructions)

error_no_progfile:
	mov	al, 102
	mov	dx, offset msg_no_progfile
	jmp	short error_fn

error_not_exe:
	mov	al, 103
	mov	dx, offset msg_not_exe
	jmp	short error_fn

error_not_coff:
	mov	al, 104
	mov	dx, offset msg_not_coff
;	jmp	error_fn

error_fn:
	push	dx
	mov	bx, offset loadname
	jmp	short error_in

error_no_dos_memory_umb:
	call	restore_umb
error_no_dos_memory:
	mov	al, 105
	mov	dx, offset msg_no_dos_memory
	jmp	short error

error_in_modesw:
	mov	al, 106
	mov	dx, offset msg_error_in_modesw
	jmp	short error

perror_no_selectors:
	mov	al, 107
	mov	dx, offset msg_no_selectors
	jmp	short error

perror_no_dpmi_memory:
	mov	al, 108
	mov	dx, offset msg_no_dpmi_memory
;	jmp	error

error:
	push	dx
	mov	bx, offset err_string
error_in:
	call	printstr
	pop	bx
	call	printstr

	mov	bx, offset crlfdollar
	call	printstr
	mov	ah, 04ch		; error exit - exit code is in al
	int	021h

printstr1:
	inc	bx
	push	ax			; have to preserve al set by error call
	mov	ah, 2
	int	021h
	pop	ax			; restore ax (John A.)
printstr:
	mov	dl, [bx]
	cmp	dl, 0
	jne	printstr1
	ret

;-----------------------------------------------------------------------------
;  DPMI utility functions

zero_regs:
	push	ax
	push	cx
	push	di
	xor	ax, ax
	mov	di, offset dpmi_regs
	mov	cx, 019h
	rep
	stosw
	pop	di
	pop	cx
	pop	ax
	ret

pm_dos:
	mov	ax, 00300h		; simulate interrupt
	mov	bx, 00021h		; int 21, no flags
	xor	cx, cx			; cx = 0x0000 (copy no args)
	sub	edi,edi
	mov	di, offset dpmi_regs
	int	031h
	ret

;-----------------------------------------------------------------------------
; Make upper memory allocatable.  Clobbers Ax and Bx.

include_umb:
	cmp	[dos_major], 5		; Won't work before dos 5
	jb short @f1
	mov	ax, 05800h		; get allocation strategy
	int	021h
	mov	[old_strategy],al
	mov	ax, 05802h		; Get UMB status.
	int	021h
	mov	[old_umb],al
	mov	ax, 05801h
	mov	bx, 00080h		; first fit, first high then low
	int	021h
	mov	ax, 05803h
;	mov	bx, 00001h		; include UMB in memory chain
	mov	bl, 1
	int	021h
@f1:
	ret

; Restore upper memory status.  All registers and flags preserved.

restore_umb:
	pushf
	cmp	[dos_major], 5		; Won't work before dos 5
	jb	short @f2
	push	ax
	push	bx
	mov	ax, 05803h		; restore UMB status.
	mov	bl,[old_umb]
	xor	bh, bh
	int	021h
	mov	ax, 05801h		; restore allocation strategy
	mov	bl,[old_strategy]
;	xor	bh, bh
	int	021h
	pop	bx
	pop	ax
@f2:
	popf
	ret

label	end_stub_text	byte
	ENDS

;*****************************************************************************
; Data
	SEGMENT _DATA PUBLIC USE16 'DATA'

;-----------------------------------------------------------------------------
;  Stored Data
err_string	db	"Load error: ",0
msg_no_progfile db	": can't open",0
msg_not_exe	db	": not EXE",0
msg_not_coff	db	": not COFF (Check for viruses)",0
msg_no_dos_memory db	"no DOS memory",0
msg_bad_dos	db	"need DOS 3",0
msg_error_in_modesw db	"can't switch mode",0
msg_no_selectors db	"no DPMI selectors",0
msg_no_dpmi_memory db	"no DPMI memory",0
crlfdollar	db	13,10,0

label last_generated_byte byte		; data after this isn't in file.
	ENDS

;-----------------------------------------------------------------------------
;  Unstored Data, available during and after mode switch
	SEGMENT _BSS PUBLIC USE16 'BSS'
modesw		dd	?		; address of DPMI mode switch
modesw_mem	dw	?		; amount of memory DPMI needs
program_file	dw	?		; file ID of program data
text_foffset	dd	?		; offset in file
data_foffset	dd	?		; offset in file
start_eip	dd	?		; EIP value to start at
client_cs	dw	?		; must follow start_eip
client_ds	dw	?

client_memory	dd	?

dos_block_seg	dw	?
dos_block_sel	dw	?
dos_block_size	dw	?

read_soffset	dd	?
read_size	dd	?

dos_major	db	?
old_umb		db	?
old_strategy	db	?

		ALIGN 2
dpmi_regs	db	32h dup (?)
dr_edi = 000h
dr_di  = 000h
dr_esi = 004h
dr_si  = 004h
dr_ebp = 008h
dr_bp  = 008h
dr_ebx = 010h
dr_bx  = 010h
dr_bl  = 010h
dr_bh  = 011h
dr_edx = 014h
dr_dx  = 014h
dr_dl  = 014h
dr_dh  = 015h
dr_ecx = 018h
dr_cx  = 018h
dr_cl  = 018h
dr_ch  = 019h
dr_eax = 01ch
dr_ax  = 01ch
dr_al  = 01ch
dr_ah  = 01dh
dr_efl = 020h
dr_es  = 022h
dr_ds  = 024h
dr_fs  = 026h
dr_gs  = 028h
dr_ip  = 02ah
dr_cs  = 02ch
dr_sp  = 02eh
dr_ss  = 030h

;-----------------------------------------------------------------------------
; At one time real mode only data.  Header stuff now used during image load.

loadname	db 81 dup (?)		; name of program file to load, if it
					; gets really long ok to overwrite next

label exe_header byte			; loaded from front of loadfile
exe_magic	dw	?
exe_bytes_last_page dw	?
exe_sectors	dw	?
exe_header_length = $ - exe_header

coff_offset	dd	?		; from start of file

coff_header	db	20 dup (?)	; loaded from after stub

aout_header	db	28 dup (?)
text_section	db	40 dup (?)
data_section	db	40 dup (?)
bss_section	db	40 dup (?)
coff_header_length = $ - coff_header

label end_stub_data byte
	ENDS

	SEGMENT _STACK STACK 'STACK'
	db stack_sz dup (?)
	ENDS

	END start
