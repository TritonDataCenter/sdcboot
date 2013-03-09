

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


          title     HEXOUT - Hexadecimal output routines for assembler
          ;
          page      ,132                ;make wide listing
          ;
          comment   }

          (C) Copyright 1988, J.P. Software, All Rights Reserved

          Author:  Tom Rawson  11/5/88


          These routines do hexadecimal output of bytes, words, and
          double words.

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
          .defcode                      ;set up code segment, no DGROUP
          ;
          assume    cs:@curseg, ds:nothing, es:nothing, ss:nothing
          ;
          ;
          public    hexoutb, hexoutw, hexoutd
          ;
          ;
          ; On entry:
          ;         AL = data to output for hexoutb
          ;         AX = data to output for hexoutw
          ;         DX:AX = data to output for hexoutd
          ;         BL = separator character for hexoutd (e.g. ":"), 0 for
          ;                no separator
          ;         ES:DI = output address (first character)
          ;
          ; On exit:
          ;         AX destroyed
          ;         ES:DI = address of byte following last character
          ;         All other registers and interrupt state unchanged
          ;         Direction flag set
          ;
          ;
          entry     hexoutd,noframe     ;output double word
          ;
          push      ax                  ;save low-order
          mov       ax,dx               ;copy high-order
          call      hexoutw             ;output 4 digits
          or        bl,bl               ;separator?
          jz        hdnosep             ;if not go on
          mov       al,bl               ;copy separator
          cld                           ;move forward
          stosb                         ;stash in buffer
          ;
hdnosep:  pop       ax                  ;get back low-order
          call      hexoutw             ;output 4 digits
          exit                          ;all done
          ;
          ;
          ; Note nested procedures below cannot use entry/exit macros
          ;
          public    hexoutw, hexoutb
          ;
hexoutw   proc      near                ;output word
          ;
          pushm     bx,cx               ;save bx, cx
          mov       cx,4                ;4 digits
          jmp       short hexcom        ;OK, go do it
          ;
hexoutb   proc      near                ;output byte
          ;
          pushm     bx,cx               ;save bx, cx
          mov       cx,2                ;2 digits
          ;
hexcom:   add       di,cx               ;point to last digit + 1
          push      di                  ;save that
          dec       di                  ;addr of last digit
          std                           ;store backwards
          mov       bx,ax               ;save data
          ;
hexloop:  mov       al,bl               ;copy low byte
          and       al,0Fh              ;get low nibble
          add       al,'0'              ;make it ASCII
          cmp       al,'9'              ;over 9?
          jle       hexput              ;if not go store it
          add       al,'A' - '0' - 10   ;not 0 - 9, make it A - F
          ;
hexput:   stosb                         ;store digit
          shrn      bx,4                ;shift data over
          loop      hexloop             ;go back for next digit
          ;
          pop       di                  ;get back end addr + 1
          popm      cx, bx              ;restore registers
          ret                           ;all done
          ;
hexoutb   endp
hexoutw   endp
          ;
@curseg   ends                          ;no, close text segment
          ;
          end


