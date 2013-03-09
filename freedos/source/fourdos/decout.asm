

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


          title     DECOUT - Decimal output routines for assembler
          ;
          page      ,132                ;make wide listing
          ;
          comment   }

          Copyright 1989, 1990, J.P. Software, All Rights Reserved

          Author:  Tom Rawson  11/5/88


          These routines do decimal output of bytes, words, and double words.
          The names ending with "u" do unsigned output; the others do signed 
          output.

          } end description
          ;
          ;
          ; Includes
          ;
          include   product.asm         ;product / platform definitions
          include   trmac.asm           ;general macros
          .cseg     LOAD                ;set loader segment if not defined
                                        ;  externally
          include   model.inc           ;Spontaneous Assembly memory models
          ;
          ;
          .defcode  ,1                  ;set up code segment, no DGROUP
          ;
          assume    cs:@curseg, ds:nothing, es:nothing, ss:nothing
          ;
          ;
          ; On entry:
          ;         AL = data to output for decoutb / bu
          ;         AX = data to output for decoutw / wu
          ;         ES:DI = output address (first character)
          ;
          ; On exit:
          ;         AX destroyed
          ;         ES:DI = address of byte following last character
          ;         All other registers and interrupt state unchanged
          ;         Direction flag clear
          ;
          ;
          entry     decoutb,noframe     ;output byte, no local variables
          push      bx                  ;save bx
          cbw                           ;convert to word
          mov       bx,0103h            ;signed, 3 digits
          call      deccvt              ;convert word
          pop       bx                  ;restore bx
          exit                          ;all done
          ;
          ;
          entry     decoutbu,noframe    ;output unsigned byte, no loc var
          push      bx                  ;save bx
          xor       ah,ah               ;convert to word
          mov       bx,0003h            ;unsigned, 3 digits
          call      deccvt              ;convert word
          pop       bx                  ;restore bx
          exit                          ;all done
          ;
          ;
          entry     decoutw,noframe     ;output word, no local variables
          push      bx                  ;save bx
          mov       bx,0105h            ;signed, 5 digits
          call      deccvt              ;convert word
          pop       bx                  ;restore bx
          exit                          ;all done
          ;
          ;
          entry     decoutwu,noframe    ;output word, unsigned, no loc var
          push      bx                  ;save bx
          mov       bx,0005h            ;unsigned, 5 digits
          call      deccvt              ;convert word
          pop       bx                  ;restore bx
          exit                          ;all done
          ;
          ;
          ; DECCVT -- Do actual decimal conversion
          ;
          ; On entry:
          ;         AX = data to output
          ;         BH = 0 if unsigned, 1 if signed
          ;         BL = maximum digit count
          ;         ES:DI = output address (first character)
          ;
          ; On exit:
          ;         AX, BX destroyed
          ;         ES:DI = address of byte following last character
          ;         All other registers and interrupt state unchanged
          ;         Direction flag clear
          ;
          ;
divtab    label     word                ;powers of 10 division table
          dw        10
          dw        100
          dw        1000
          dw        10000
          ;
          ;
          entry     deccvt,noframe      ;convert word to ASCII
          ;
          push      dx                  ;save dx
          cld                           ;output goes forward
          or        bh,bh               ;signed?
          jz        outgo               ;if not go on
          or        ax,ax               ;check sign and check for zero
          je        lastdig             ;if zero just go store zero
          jg        outgo               ;if positive go on
          mov       bptr es:[di],'-'    ;output sign
          inc       di                  ;skip to next place
          neg       ax                  ;make value positive
          ;
outgo:    sub       bl,2                ;get divisor index
          xor       bh,bh               ;clear high byte
          shl       bx,1                ;make it a word offset
          ;
lzloop:   xor       dx,dx               ;clear high word
          div       cs:divtab[bx]       ;divide
          or        ax,ax               ;check result
          jnz       digloop             ;if not a leading zero go on
          mov       ax,dx               ;remainder to ax
          sub       bx,2                ;move to next divisor
          jge       lzloop              ;keep going with leading zeros
          jmp       short lastdig       ;no more divisors, so only one digit
          ;
digloop:  add       al,'0'              ;make digit ASCII
          stosb                         ;store digit
          mov       ax,dx               ;copy remainder for next round
          sub       bx,2                ;move to next divisor
          jl        lastdig             ;if no more, last digit is in AX
          xor       dx,dx               ;clear high word
          div       cs:divtab[bx]       ;divide again
          jmp       short digloop       ;and go do another digit
          ;
lastdig:  add       al,'0'              ;make last digit ASCII
          stosb                         ;store it
          ;
          pop       dx                  ;restore registers
          exit                          ;all done
          ;
          ;
@curseg   ends                          ;close segment
          ;
          end


