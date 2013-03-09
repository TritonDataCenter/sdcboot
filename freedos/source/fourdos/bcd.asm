

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


          title     BCD.ASM - BCD utilities for 4DOS

          page      ,132

          comment   }

          (C) Copyright 1990 - 1999  Rex C. Conn & J.P. Software
          All Rights Reserved

          Author:    Rex C. Conn

          These 4DOS support routines provide BCD addition, subtraction,
          multiplication, division, and modulo functions

          } end description

          ;
          ; Includes
          ;

          include   product.asm         ;product / platform definitions
          include   trmac.asm           ; general macros

          .model    medium
          .data

SIZE_INTEGER    equ   20
SIZE_DECIMAL    equ   10
SIZE_MANTISSA   equ   30
SIZE_DIVIDEND   equ   SIZE_MANTISSA + SIZE_DECIMAL + 1

workspace      db      2 * SIZE_MANTISSA DUP (?)
workspace_end  equ     $ - 1
               db      ?		; Dividend overflow.
dividend       db      SIZE_DIVIDEND DUP (?)

          .code     ASM_TEXT

          ;
          ; ROUND_BCD - Round up the target
          ;
          ; On entry:
          ;         DF set
          ;         DI = end of mantissa + 1
          ;         CX = number of bytes to round
          ;
          ; On exit:
          ;         Carry set if overflow
          ;         AX, CX, DI destroyed
          ;         All other registers unchanged, interrupts on
          ;

          entry     round_bcd,noframe,,local

          cmp       byte ptr [di],5	; if not 5 or greater, return
          jb        no_round
          dec       di			; get previous digit
          stc

round_loop:
          mov       al,[di]
          adc       al,0		; add carry to digit
          aaa
          mov       [di],al
          dec       di
          jnc       round_exit		; if CF = 0, return
          loop      round_loop
          jmp       short round_exit

no_round:
          clc				; if CF = 0, no overflow
round_exit:
          exit


          ;
          ; ADD_BCD - Add or subtract two BCD numbers
          ;
          ; On entry:
          ;         x = first BCD structure
          ;         y = second BCD structure
          ;
          ; On exit:
          ;         x = total
          ;         AX, CX, DX, ES destroyed
          ;         All other registers unchanged, interrupts on
          ;

          entry     add_bcd,varframe,far

          argW      y
          argW      x
          varB      save_sign		; temp storage for unary sign
          varend

          pushm     si,di
          push      ds
          pop       es

          mov       si,x
          mov       di,y

          mov       ah,[si]		; save the signs
          mov       al,[di]

          mov       cx,SIZE_MANTISSA
          add       di,cx		; set to end of BCD structure
          add       si,cx		;   (for backwards read)

          cmp       al,ah		; if signs both plus or minus, then
          je        addition		;   add, else subtract

          mov       dx,si		; save target pointer
          clc
sub_loop:
          mov       al,[si]
          sbb       al,[di]		; subtract & carry
          aas
          mov       [si],al		; put it back to the result
          dec       si
          dec       di
          loop      sub_loop
          mov       al,'+'		; if no carry, result > 0
          jnc       write_sign

          mov       si,dx		; negative - subtract result from 0
          clc
          mov       cx,SIZE_MANTISSA

negate_loop:
          mov       al,0
          sbb       al,[si]		; subtract & ASCII adjust
          aas
          mov       [si],al
          dec       si
          loop      negate_loop
          mov       al,'-'		; store the negative sign
          jmp       short write_sign

addition:
          mov       save_sign,al
          clc				; clear carry for start
add_loop:
          mov       al,[si]
          adc       al,[di]		; add & carry
          aaa
          mov       [si],al
          dec       si
          dec       di
          loop      add_loop
          jc        add_error		; carry out of max size, return overflow
          mov       al,save_sign

write_sign:
          mov       [si],al
          xor       ax,ax
          jmp       short add_bye

add_error:
          mov       ax,-1

add_bye:
          popm      di,si		; restore registers

          exit


          ;
          ; MULTIPLY_BCD - multiply two BCD numbers
          ;
          ; On entry:
          ;         x = first BCD structure
          ;         y = second BCD structure
          ;
          ; On exit:
          ;         x = total
          ;         AX, BX, CX, DX, ES destroyed
          ;         All other registers unchanged, interrupts on
          ;

          entry     multiply_bcd,varframe,far

          argW      y
          argW      x
          varW      multiplicand
          varB      save_sign		; temp storage for unary sign
          varend

          pushm     si,di               ;save registers

          push      ds
          pop       es

          mov       bx,x
          mov       di,y

          mov       ah,[bx]		; get the signs
          mov       al,[di]

          mov       byte ptr save_sign,'+'	; Assume plus result.
          cmp       ah,al		; Signs the same?
          je        init_multiply	; If so, return + result
          mov       byte ptr save_sign,'-'

init_multiply:
          add       di,SIZE_MANTISSA	; go to end of BCD structures
          mov       multiplicand,di	; save the target
          add       bx,SIZE_MANTISSA

          cld
          lea       di,workspace
          mov       ax,0 		; init workspace to 0's
          mov       cx,size workspace
          rep       stosb

          lea       di,workspace_end
          mov       cx,SIZE_MANTISSA	; # of digits to be multiplied

multiply_loop:
          push      di			; Save current product place.

          cmp       byte ptr [bx],0	; if current multiplier == 0,
          je        next_digit		;   don't bother with it
          mov       dx,0                ; clear the carry
          mov       dl,[bx]		; get the multiplier

          mov       si,multiplicand	; restore multiplicand pointer
          push      cx			; save multiplier counter
          mov       cx,SIZE_MANTISSA

inner_multiply:
          mov       al,[si]
          mul       dl			; multiply & adjust for BCD
          aam
          add       al,[di]		; accumulate & add carry
          aaa
          add       al,dh
          aaa
          mov       [di],al

          mov       dh,ah		; save carry
          dec       si
          dec       di
          loop      inner_multiply

          mov       [di],ah		; write carry
          pop       cx

next_digit:
          pop       di
          dec       di			; Next product.
          dec       bx			; Next multiplier.
          loop      multiply_loop

          mov       cx,SIZE_MANTISSA
          mov       di,offset workspace_end - SIZE_DECIMAL + 1
          call      round_bcd		; round product up

          mov       cx,SIZE_MANTISSA
          mov       si,offset workspace + SIZE_INTEGER
          mov       di,x		; store the result in structure "x"
          mov       al,save_sign	; store the sign
          stosb
          rep       movsb

          mov       ax,0		; return 0 if no overflow
          mov       cx,SIZE_INTEGER	; check workspace for overflow
          lea       di,workspace
          repe      scasb
          je        multiply_return

multiply_error:
          mov       ax,-1		; set AX to -1 for error

multiply_return:
          popm      di,si

          exit


          ;
          ; DIVIDE_BCD - Divide two BCD numbers
          ;
          ; On entry:
          ;         x = first BCD structure
          ;         y = second BCD structure
          ;         modulo_flag = flag to only do modulo division
          ;
          ; On exit:
          ;         x = total, y = dividend (modulo result)
          ;         AX, CX, DX destroyed
          ;         All other registers unchanged, interrupts on
          ;

          entry     divide_bcd,varframe,far

          argW      modulo_flag
          argW      y
          argW      x
          varW      divisor
          varB      save_sign		; temp storage for unary sign
          varend

          pushm     si,di

          push      ds
          pop       es

          mov       bx,x
          mov       di,y

          mov       ah,[bx]		; get the signs
          mov       al,[di]

          mov       byte ptr save_sign,'+'	; default positive result
          cmp       word ptr modulo_flag,0
          je        div_signs
          cmp       ah,'-'		; if modulo & source is negative,
          je        neg_result		;   then result is negative
          jmp       short init_division
div_signs:
          cmp       ah,al		; signs the same?
          je        init_division
neg_result:
          mov       byte ptr save_sign,'-'	; switch to negative result

init_division:
          cld
          mov       divisor,di		; save the divisor
          inc       word ptr divisor	; skip sign
          lea       di,workspace
          mov       al,0
          mov       cx,SIZE_DIVIDEND
          rep       stosb		; Zero out quotient to start.

          mov       cx,SIZE_MANTISSA
          mov       di,offset dividend - 1	; move dividend to work area
          stosb				; zero out the overflow byte
          mov       si,bx		
          inc       si			; skip the sign
          rep       movsb		; store the dividend

          mov       cx,SIZE_DIVIDEND - SIZE_MANTISSA
          rep       stosb		; Zero out dividend underflow.

          mov       cx,SIZE_MANTISSA
          mov       di,divisor		; Look for zero divisor.
          repz      scasb
          jnz       set_div_size
          jmp       divide_error	; can't divide by 0

set_div_size:				; set the divisor size
          lea       di,workspace	; point to quotient
          add       di,cx
          add       word ptr divisor,SIZE_MANTISSA - 1
          sub       divisor,cx		; first non-zero divisor digit
          inc       cx			; DH = subtraction counter
          mov       dx,cx		; DL = significant divisor digits
          lea       si,dividend
          mov       cx,SIZE_DIVIDEND + 1
          sub       cx,dx		; number of times thru divide loop

divide_loop:
          cmp       word ptr modulo_flag,1	; are we doing a modulo (x % y)?
          jne       not_modulo

          cmp       di,offset workspace + SIZE_MANTISSA
          jae       divide_end		; only do integer divide for modulo

not_modulo:
          push      cx			; Division count.
          push      di			; pointer to quotient
          mov       bx,si		; save pointer to dividend
          mov       dh,0		; set sub counter to zero

next_divisor:
          cmp       byte ptr [si-1],0	; Overflow in remainder?
          jne       subtract_divisor	;   if yes, divisor fits
          mov       cx,0
          mov       cl,dl		; length of divisor
          mov       di,divisor		; pointer to divisor

compare_fit:
          cmpsb				; Does it fit in remainder?
          ja        subtract_divisor	; If yes, subtract.
          jb        over_size		; If no, save subtraction count.
          loop      compare_fit

subtract_divisor:
          inc       dh			; Sub counter
          mov       si,divisor		; restore divisor pointer
          mov       di,bx		; restore dividend pointer
          mov       cx,0
          mov       cl,dl		; significant divisor digits
          dec       cx			; Adjust for a moment.
          add       si,cx		; Point to current position.
          add       di,cx
          inc       cx

          clc
subtract_remainder:
          mov       ah,[si]
          mov       al,[di]		; remainder digit
          sbb       al,ah		; subtract & adjust
          aas
          mov       [di],al
          dec       di
          dec       si
          loop      subtract_remainder

          sbb       byte ptr [di],0	; Subtract the carry, if any.

          mov       si,bx
          jmp       next_divisor	; Next divisor.

over_size:
          mov       si,bx		; get next dividend digit
          inc       si
          pop       di
          pop       cx
          mov       al,dh		; save quotient
          stosb
          loop      divide_loop

          mov       cx,SIZE_MANTISSA
          mov       di,offset workspace + SIZE_DIVIDEND - 1

          call      round_bcd		; round up the quotient
          jc        divide_error

          mov       cx,SIZE_MANTISSA
          mov       si,offset workspace + SIZE_DECIMAL
          mov       di,x		; store the result in structure "x"
          mov       al,save_sign	; store the sign
          stosb
          rep       movsb

          xor       ax,ax
          mov       cx,SIZE_DECIMAL	; check workspace for divide overflow
          lea       di,workspace

          repe      scasb
          jne       divide_error

divide_end:
          xor       ax,ax
          cmp       modulo_flag,ax
          je        divide_return

          mov       di,y		; stuff modulo into structure "y"
;          mov       [di],ah
;          inc       di

          mov       al,save_sign	; store the sign
          stosb

          lea       si,dividend		; Dividend has remainder
          mov       cx,SIZE_MANTISSA
          rep       movsb

          xor       ax,ax
          jmp       short divide_return

divide_error:
          mov       ax,-1

divide_return:
          popm      di,si

          exit

          end

