

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


          title     RESHIGH - Reserve a block of high memory
          ;
          page      ,132                ;make wide listing
          ;
          comment   }

          Copyright 1989, 1990, J.P. Software, All Rights Reserved

          Author:  Tom Rawson  11/12/88


          This routine reserves a block of high memory

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
          ;         AX = length of block to reserve, in paragraphs
          ;         BX = requested segment, if any
          ;
          ; On exit:
          ;         Carry flag clear if block reserved, otherwise set
          ;         AX = Reserved block segment address
          ;         BX = adjusted length
          ;         All other registers and interrupt state unchanged
          ;
          ;
          .public   ResHigh             ;create public name, with prefix
          entry     ResHigh,,,local     ;transient high memory reserve,
                                        ;  make it "local" so Spont. ASM
                                        ;  public above is used
          ;
          varW      reqseg              ;requested segment
          varW      ressize             ;reserved size
          varend
          ;
          pushm     cx,es               ;save registers
          mov       reqseg,bx           ;save requested segment
          mov       ressize,ax          ;save reserved size
          ;
ResTry:   mov       cx,ax               ;copy caller's size
          mov       bx,0FFFFh           ;reserve everything
          calldos   ALLOC               ;returns available paragraphs
          sub       bx,cx               ;available - desired = hole size
          jbe       nohole              ;if already too small just go on
          sub       bx,1                ;leave space for MCB chain
          jbe       nohole              ;if nothing left forget hole
          calldos   ALLOC               ;reserve hole
          jc        return              ;if can't do it get outta here
          mov       es,ax               ;copy hole segment
          mov       bx,cx               ;copy user's size
          calldos   ALLOC               ;allocate user's block, get seg in AX
          jc        retfree             ;complain if we can't
          push      ax                  ;save high block address
          calldos   FREE                ;now free hole
          pop       ax                  ;restore high block address
          jc        return              ;complain if we can't free hole
          mov       bx,reqseg           ;get segment request
          or        bx,bx               ;anything there?
          je        retOK               ;if not we're done
          cmp       bx,0FFFFh           ;segment request already processed?
          je        adjust              ;if so go adjust size
          cmp       ax,bx               ;room for requested segment?
          jbe       retOK               ;done if already there or no room
          mov       es,ax               ;copy old high block address to es
          sub       ax,bx               ;get segment difference
          add       ax,ressize          ;get adjusted size
          mov       reqseg,0FFFFh       ;flag request in process
          push      ax                  ;save it
          calldos   FREE                ;free original block
          pop       ax                  ;restore adjusted size
          jnc       restry              ;go on if no error
          jmp       short return        ;holler if error
          ;
adjust:   mov       es,ax               ;copy block address
          mov       bx,ressize          ;get caller's size
          calldos   REALLOC             ;adjust block size, then drop through
          ;
retOK:    clc                           ;all OK - clear carry
          jmp       short return        ;and return
          ;
retfree:  calldos   FREE                ;second reserve failed - free hole
          stc                           ;show error
          jmp       short return        ;and return
          ;
nohole:   mov       bx,cx               ;just reserve user memory, not hole
          calldos   ALLOC               ;allocate user's block, get seg in AX
                                        ;return with carry from ALLOC
          ;
return:   popm      es,cx               ;restore registers
          mov       bx,ressize          ;get reserved size
          exit                          ;all done
          ;
@curseg   ends                          ;no, close text segment
          ;
          end

