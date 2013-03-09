

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


          title     SRCHENV - Search environment for a string
          ;
          page      ,132                ;make wide listing
          ;
          comment   }

          Copyright 1989, 1990, J.P. Software, All Rights Reserved

          Author:  Tom Rawson  1/29/89


          This routine searches the DOS environment for a string.

          } end description
          ;
          ;
          ; Includes
          ;
          include   product.asm         ;product equates
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
          ;         CX = length of string to search for
          ;         DS:SI = pointer to string to search for
          ;         ES = Environment segment
          ;
          ; On exit:
          ;         Carry set if not found, clear if found
          ;         CX = length of string found
          ;         ES:SI = address of byte following "=" in environment
          ;         All other registers and interrupt state unchanged
          ;         Direction flag clear
          ;
          ;
          entry     SrchEnv,noframe     ;set up entry
          ;
          pushm     ax,di               ;save registers
          mov       ax,es               ;get our environment segment
          or        ax,ax               ;check if any environment
          jz        NotFound            ;if none then the string isn't there
          xor       di,di               ;start at beginning
          cld                           ;go forward
          ;
EnvScan:  cmp       bptr es:[di],0      ;end of environment?
          je        NotFound            ;if so we didn't find it
          pushm     cx,si               ;save count and address
          ;
EnvLoop:  cmp       bptr es:[di],"="    ;end of name?
          je        EnvNext             ;if so go on to next one
          cmpsb                         ;no, check a byte
          jne       EnvNext             ;different, go to next string
          loop      EnvLoop             ;loop for next byte
          cmp       bptr es:[di],"="    ;all bytes matched -- at end?
          je        FoundIt             ;yes, we got it
          ;
EnvNext:  xor       al,al               ;get 0 for scan
          mov       cx,800h             ;reasonable scan length
          repne     scasb               ;find next string
          popm      si,cx               ;restore count and address
          jmp       short EnvScan       ;go process next string
          ;
NotFound: stc                           ;couldn't find it, return
          jmp       short Return
          ;
FoundIt:  popm      si,cx               ;restore count and address
          inc       di                  ;point di to value
          mov       si,di               ;save value location
          xor       al,al               ;get 0 for scan
          mov       cx,0800h            ;reasonable scan length
          repne     scasb               ;find end of string
          mov       cx,di               ;get end address + 2
          sub       cx,si               ;get length + 1
          dec       cx                  ;get length
          ;
Return:   popm      di,ax               ;restore registers
          exit                          ;all done
          ;
@curseg   ends                          ;close segment
          ;
          end


