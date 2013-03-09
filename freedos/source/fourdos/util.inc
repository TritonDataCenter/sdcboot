IFDEF ??Version
__TASM__	equ	-1
__MASM__	equ	0
masm51
ELSE
__TASM__	equ	0
__MASM__	equ	-1
ENDIF
.error		macro	message
  IF __MASM__
    IF2
.err
%out	**Error**  &message
    ENDIF
  ELSE
.err
%out	**Error**  &message
  ENDIF
endm
.push		macro	r0,r1,r2,r3,r4,r5,r6,r7,r8,r9,r10
  IFNB <r10>
.error	<.PUSH has more than 10 arguments>
  ENDIF
  irp $reg,<r0,r1,r2,r3,r4,r5,r6,r7,r8,r9>
    IFB <$reg>
      exitm
    ELSEIFIDNI <$reg>,<FLAGS>
		pushf
    ELSE
		push	$reg
    ENDIF
  endm
endm
.pop		macro	r0,r1,r2,r3,r4,r5,r6,r7,r8,r9,r10
  IFNB <r10>
.error	<.POP has more than 10 arguments>
  ENDIF
  irp $reg,<r9,r8,r7,r6,r5,r4,r3,r2,r1,r0>
    IFNB <$reg>
      IFIDNI <$reg>,<FLAGS>
		popf
      ELSE
		pop	$reg
      ENDIF
    ENDIF
  endm
endm
.recover	macro
		local	recover_010
		jmp	short recover_010
recover_010:
endm
.popf		macro
		local	popf_020, popf_040
		jmp	short popf_040
popf_020:	iret
popf_040:	push	cs
		call	popf_020
endm
.xchg_ptrs	macro
		.push	ds,es
		.pop	es,ds
		xchg	di,si
endm
