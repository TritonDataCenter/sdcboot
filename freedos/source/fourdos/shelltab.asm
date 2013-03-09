

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


          title     SHELLTAB - 4DOS loader shell table
          ;
          page      ,132                ;make wide listing
          ;
          comment   }

          Copyright 1989, 1990, J.P. Software, All Rights Reserved

          Author:  Tom Rawson  11/12/88


          This module contains the shell table, which consists of one bit
          per possible 4DOS shell.  It is in a separate module because it
          only remains in memory in the original copy of the low-memory
          loader.  Depending on whether this is the root copy of the loader,
          either the beginning or end of this module will also serve to 
          define the breakpoint in the code segment between resident and 
          initialization code.  It therefore MUST be linked in the proper 
          order!

          } end description
          ;
          ;
          include   trmac.asm           ;general macros
          .cseg     LOAD                ;set loader segment if not defined
                                        ;  externally
          include   model.inc           ;Spontaneous Assembly memory models
          include   4dlparms.asm
          ;
          .defcode  ,1                  ;set up code segment, no DGROUP
          ;
          assume    cs:@curseg,ds:nothing, es:nothing, ss:nothing
          ;
          ;
          public    ShellTab, ShTabEnd
          ;
          ;
ShellTab  db        SHTBYTES dup (?)    ;define table
          ;
          ;
          ; NewShell - Scan table for first available shell number
          ;
          ;
          ; On entry:
          ;         No requirements
          ;
          ; On exit:
          ;         BL = shell number
          ;         Carry set if error (no number available), clear if all
          ;           OK
          ;         AX, CX destroyed
          ;         All other registers unchanged, interrupts on
          ;
          ; NOTE:  This routine disables interrupts temporarily while 
          ; checking for an available shell number.  This is because in
          ; a multitasking system this code must be re-entrant.
          ;
          ;
          entry     NewShell,noframe    ;set up entry
          ;
          mov       bx,offset ShellTab  ;point to table
          mov       cx,SHTBYTES         ;get table length
          mov       al,1                ;start shell counter at 1
          ;
ByteLoop: mov       ah,80h              ;start with bit 7
          push      cx                  ;save cx
          mov       cx,8                ;get bit count
          ;
BitLoop:  cli                           ;hold interrupts
          test      cs:[bx],ah          ;test bit
          jz        GotBit              ;if not set we found it
          sti                           ;allow interrupts
          shr       ah,1                ;move to next bit
          inc       al                  ;bump shell counter
          loop      BitLoop             ;and continue
          pop       cx                  ;restore cx
          ;
NextByte: inc       bx                  ;move to next byte
          loop      ByteLoop            ;try again
          ;
          stc                           ;no room, error
          jmp       short NSDone        ;all done
          ;
GotBit:   bset      cs:[bx],ah          ;set bit
          sti                           ;allow interrupts
          pop       cx                  ;restore cx
          mov       bl,al               ;copy shell number to bl
          clc                           ;show all OK
          ;
NSDone:   exit                          ;that's all folks
          ;
          ;
          ; TrmShell - Terminate a secondary shell
          ;
          ; On entry:
          ;         Segment registers set up for loader
          ;         BL = shell number to terminate
          ;
          ; On exit:
          ;         AX, BX destroyed
          ;         All other registers and interrupt state unchanged
          ;
          ; NOTE:  This routine disables interrupts temporarily while 
          ; checking to see if the shell number was in use.  This is 
          ; because in a multitasking system this code must be re-entrant.
          ;
          ;
          entry     TrmShell,noframe    ;set up entry
          ;
          dec       bl                  ;make shell number 0-based
          mov       cl,bl               ;copy it
          shrn      bl,3                ;make byte number
          xor       bh,bh               ;clear high byte
          and       cl,7                ;make shift count
          mov       ah,80h              ;get bit to shift
          shr       ah,cl               ;shift bit into place
          cli                           ;hold interrupts
          test      cs:ShellTab[bx],ah  ;is bit set?
          jz        TSErr               ;if not holler
          xor       ShellTab[bx],ah     ;clear it
          clc                           ;show no error
          jmp       short TSDone        ;all done
          ;
TSErr:    stc                           ;shell wasn't active, error
          ;
TSDone:   sti                           ;allow interrupts
          exit                          ;that's all folks
          ;
ShTabEnd  equ       $                   ;define end
          ;
          ; Allocate space for an MCB so that when 4DLINIT.ASM reduces
          ; memory the new MCB that ends up here doesn't overwrite
          ; anything.
          ;
          db        16 dup (?)
          ;
@curseg   ends
          ;
          end

