

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


          title     RELOCATE - Do segment fixups for 4DOS
          ;
          page      ,132                ;make wide listing
          ;
          comment   }

          Copyright 1991, JP Software Inc., All Rights Reserved

          Author:  Tom Rawson  4/7/91


          This routine relocates the 4DOS transient code based on segment
          addresses and a relocation table passed in.  Note that the
          adjustment value passed in DX can be used as a base value the
          first time the code is relocated, and can also be used as a
          modifying value if the code is moved.

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
          ;         BX = load address of image for relocation
          ;         DX = adjustment amount for relocation
          ;         DS:SI = relocation table address
          ;
          ; On exit:
          ;         AX, DI destroyed
          ;         All other registers and interrupt state unchanged
          ;
          .public   Reloc               ;create public name, with prefix
          .proc     Reloc               ;set up procedure
          ;
          push      es                  ;save ES
          cld                           ;go forward
          lodsw                         ;get the segment count
          ;
RelSeg:   dec       ax                  ;anything left to relocate?
           jl       RLDone              ;if not go on
          push      ax                  ;save seg count
          lodsw                         ;get relative segment
          add       ax,bx               ;add load segment
          mov       es,ax               ;set ES for fix segment
          lodsw                         ;get count for this segment
          mov       cx,ax               ;copy for loop
           jcxz     NextSeg             ;go on if empty (should never happen)
          ;
RelLoop:  lodsw                         ;get offset
          mov       di,ax               ;copy as modification offset
          add       es:[di],dx          ;do the fixup
          loop      RelLoop             ;loop for more in buffer
          ;
NextSeg:  pop       ax                  ;get back segment count
          jmp       RelSeg              ;and loop for next segment
          ;
RLDone:   pop       es                  ;restore ES
          ret                           ;all done
          .endp     Reloc               ;that's all
          ;
@curseg   ends                          ;no, close text segment
          ;
          end

