

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


          title     FINDEMS - See if EMS memory is present
          ;
          page      ,132                ;make wide listing
          ;
          comment   }

          Copyright 1989, 1990, J.P. Software, All Rights Reserved

          Author:  Tom Rawson  11/12/88


          This routine checks whether EMS memory is present; if so, it 
          returns the EMS page frame address and total and unallocated
          page counts.

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
          ;         No requirements
          ;
          ; On exit:
          ;         Carry flag clear if EMS present and no errors, set if 
          ;           EMS not present or error occurred
          ;         AL = 0 if no EMS error, <>0 is EMS error code, 0FFh if
          ;           EMS driver not found
          ;         BX = page frame address if no error
          ;         CX = available page count if no error
          ;         DX = total page count if no error
          ;         All other registers and interrupt state unchanged
          ;
          ;
emm_name  db        'EMMXXXX0'          ;name of EMS driver
          ;
          ;
          entry     findems,noframe     ;look for ems
          ;
          pushm     si,di,ds            ;save registers
          mov       al,67h              ;get EMS interrupt number
          calldos   GETINT              ;get interrupt vector in es:bx
          lea       si,emm_name         ;point to driver name for comparison
          mov       di,10               ;address of name field in dev header
          loadseg   ds,cs,ax            ;set ds for comparison
          mov       al,0FFh             ;get return code for no ems
          mov       cx,8                ;comparison length
          cld                           ;move forward
          repe      cmpsb               ;does driver name match?
          jne       errret              ;no, no EMS
          callems   STAT                ;get EMS status
          or        ah,ah               ;any error?
          jnz       errret              ;if error complain
          callems   UPCNT               ;get ems page counts in BX, DX
          or        ah,ah               ;any error?
          jnz       errret              ;if error complain
          mov       cx,bx               ;move unallocated page count to CX
          callems   PFAD                ;get page frame address in BX
          or        ah,ah               ;any error?
          jnz       errret              ;if error complain
          clc                           ;ems is there, clear carry
          jmp       short return        ;and return
          ;
errret:   stc                           ;not there, set carry
          ;
return:   popm      ds,di,si            ;restore registers
          exit                          ;all done
          ;
@curseg   ends                          ;close segment
          ;
          end

