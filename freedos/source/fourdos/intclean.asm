

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


          ;
          ; INTCLEAN - Interrupt cleanup routines for loader and server.
          ;
          ; Copyright 1989, 1990, J.P. Software, All Rights Reserved
          ;
          ;
          ; Used as an include file for 4DLINIT.ASM and SERVER.ASM
          ;
          ; Requires definition of the following symbols:
          ;
          ;         OldInt2F            old interrupt 2F vector
          ;         OldInt23            old interrupt 23 vector
          ;         OldInt24            old interrupt 24 vector
          ;         OldInt2E            old interrupt 2E vector
          ;         IntFlags            interrupt flags byte
          ;
          ;
          ; INTCLEAN - Clean up interrupts
          ;
          ; On entry:
          ;         DS = local data segment
          ;
          ; On exit:
          ;         AX, BX destroyed
          ;         Other registers and interrupt state unchanged
          ;
          entry     IntClean,noframe,,local    ;clean up server interrupts
          assume    ds:@curseg          ;fix assumes
          ;
          mov       ax,(INTHOOK shl 8) + SERVINT  ;get server int and flag
          mov       bx,offset OldInt2F  ;point to old vector
          call      IntRest             ;restore the vector
          ;
          mov       ax,(I23HOOK shl 8) + 23h  ;get int 23 number and flag
          mov       bx,offset OldInt23  ;point to old vector
          call      IntRest             ;restore the vector
          ;
          mov       ax,(I24HOOK shl 8) + 24h  ;get int 24 number and flag
          mov       bx,offset OldInt24  ;point to old vector
          call      IntRest             ;restore the vector
          ;
          mov       ax,(I2EHOOK shl 8) + 2Eh  ;get int 2E number and flag
          mov       bx,offset OldInt2E  ;point to old vector
          call      IntRest             ;restore the vector
          ;
          exit                          ;all done
          ;
          ;
          ; INTREST - Restore a vector
          ;
          ; On entry:
          ;         AH = interrupt in use flag (bit in IntFlags)
          ;         AL = interrupt number
          ;         DS:BX = address of old vector info
          ;
          ; On exit:
          ;         AX destroyed
          ;         Other registers and interrupt state unchanged
          ;
          entry     IntRest,noframe,,local    ;restore an interrupt
          test      IntFlags,ah         ;check if this interrupt is in use
          jz        IRDone              ;if not, exit
          xor       IntFlags,ah         ;clear flag
          push      ds                  ;save ds
          mov       dx,[bx].foff        ;get old offset
          mov       ds,[bx].fseg        ;and segment
          calldos   SETINT              ;restore interrupt vector
          pop       ds                  ;restore ds
          ;
IRDone:   exit                          ;that's all

