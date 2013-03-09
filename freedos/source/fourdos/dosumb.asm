

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


          title     DOSUMB - Reserve or release a UMB via DOS
          ;
          page      ,132                ;make wide listing
          ;
          comment   }

          Copyright 1990, J.P. Software, All Rights Reserved

          Author:  Tom Rawson  12/23/90


          These routines link upper memory blocks into the DOS 5
          allocation chain using the API defined in DOS 5.0, and 
          reserve XMS upper memory blocks via DOS.

          } end description
          ;
          ;
          ; Includes
          ;
          include   product.asm         ;product / platform definitions
          include   trmac.asm           ;general macros
          include   model.inc           ;Spontaneous Assembly memory models
          ;
          ;
          .defcode  ,1                  ;set up code segment, no DGROUP
          ;
          assume    cs:@curseg, ds:nothing, es:nothing, ss:nothing
          ;
          ;
          ; DosUMB - Reserve or release a DOS UMB
          ;
          ; On entry:
          ;         AH = function code:  0 = reserve, 1 = release
          ;         BX = paragraphs to reserve if AH = 0
          ;              segment to release if AH = 1
          ;
          ; On exit:
          ;         Reserve:
          ;           Carry flag clear if block reserved, otherwise set
          ;           AX = Reserved block segment address
          ;           All other registers and interrupt state unchanged
          ;         Release:
          ;           Carry flag clear if block released, otherwise set
          ;           All other registers and interrupt state unchanged
          ;
%         ifdifi    <_CSEG>,<LOAD>      ;vary entry points based on segment
          entry     DosUMB,noframe
          else
          entry     L_DosUMB,noframe
          endif
          ;
          pushm     cx,es               ;save regs
          pushm     ax,bx               ;save arguments
          int       12h                 ;get memory size in AX
          shln      ax,6,cl             ;convert K to paragraphs
          mov       cx,ax               ;save it for later
          mov       ah,1                ;get value to set links
          call      LinkUMB             ;link in the UMBs, set high first
           jc       DULErr              ;if link didn't work give up
          popm      bx,ax               ;get back arguments
          or        ah,ah               ;get function
           jnz      DUFree              ;if it's release go do that
          calldos   ALLOC               ;try to allocate high
           jc       DUUnlink            ;if it failed go unlink UMBs
          cmp       ax,cx               ;are we above end?
           jae      DUOK                ;if not we're OK, return
          ;
DULow:    mov       es,ax               ;oops - in low memory - copy address
          calldos   FREE                ;free the low memory block
          stc                           ;got low memory, complain
          jmp       short DUUnLink      ;go return
          ;
DUFree:   mov       es,bx               ;copy address to free
          calldos   FREE                ;free the block
          ;
DUOK:     clc                           ;show no error
          ;
DUUnLink: pushf                         ;save result
          push      ax                  ;save possible address
          xor       ah,ah               ;get unlink function
          call      LinkUMB             ;unlink UMBs and reset strategy
          pop       ax                  ;restore possible address
          popf                          ;get back result
          jmp       short DURet         ;and return
          ;
DULErr:   add       sp,4                ;remove saved AX and BX
          stc                           ;return with carry set
          ;
DURet:    popm      es,cx               ;restore regs
          exit                          ;and return
          ;
          ;
          ; LinkUMB - Link or unlink DOS UMBs from the allocation chain
          ;
          ; On entry:
          ;         AH = function code:  0 = unlink, 1 = link
          ;
          ; On exit:
          ;         Carry flag clear if UMBs linked or unlinked, set if error
          ;         AX destroyed
          ;         All other registers and interrupt state unchanged
          ;
%         ifdifi    <_CSEG>,<LOAD>      ;vary entry points based on segment
          entry     LinkUMB,noframe
          else
          entry     L_LinkUMB,noframe
LinkUMB   equ       L_LinkUMB
          endif
          ;
          pushm     bx,cx               ;save registers
          push      ax                  ;save argument
;jmp temp_06
;msg_06 db "Unlinking UMBs",13,10,"$"
;msg_07 db "Linking UMBs",13,10,"$"
;temp_06:
;pushm ax,dx,ds
;loadseg ds,cs
;or ah,ah
;jnz temp_07
;mov dx,offset msg_06
;jmp temp_08
;temp_07:
;mov dx,offset msg_07
;temp_08:
;calldos message
;popm ds,dx,ax
          stc                           ;let's look for DR-DOS
          mov       ax,DRCHECK          ;get DR-DOS check code
          calldos                       ;check for it
           jc       LinkMS              ;if not DR-DOS go on
          cmp       ax,106Fh            ;is it DR-DOS 6 11/92 or before?
           jbe      LinkDR              ;if so do old DR-DOS link
          ;
          ; DR DOS 6 3/93 update and DR DOS 7 and above do MS-style links
          ;
          pop       cx                  ;restore argument
          jmp       short MSDoLink      ;try MS-style link
          ;
          ; Handle DR-DOS UMBs
          ;
LinkDR:   mov       dx,DRLNKOFF         ;assume link off
;jmp temp_01
;msg_01 db "  using DR-DOS link calls ...$"
;temp_01:
;pushm ax,dx,ds
;loadseg ds,cs
;mov dx,offset msg_01
;calldos message
;popm ds,dx,ax
          pop       ax                  ;get back function code
          or        ah,ah               ;check it
           jz       DRDoLink            ;if unlinking go on
          inc       dx                  ;convert to link on code
          ;
DRDoLink: mov       ax,DRLINK           ;get link function
          calldos                       ;link or unlink the UMBs
          jmp       short LURet         ;and return (no status set)
          ;
          ; Handle MS-DOS UMBs
          ;
LinkMS:   calldos   VERSION             ;get DOS version
          pop       cx                  ;restore argument
          cmp       al,5                ;DOS 5 or above?
           jl        LUError            ;if not go on
          cmp       al,10               ;check for OS/2 1.x DOS box
           je       LUError             ;  if in the 1.x DOS box complain
                                        ;  (allows 2.x DOS box)
          ;
MSDoLink: mov       bx,ULINKON          ;get value to set UMB links
;jmp temp_02
;msg_02 db "  using MS-DOS link calls ...$"
;temp_02:
;pushm ax,dx,ds
;loadseg ds,cs
;mov dx,offset msg_02
;calldos message
;popm ds,dx,ax
          mov       cl,HFIRST + FIRSTFIT  ;set for first fit in high mem
          or        ch,ch               ;check function code
          jnz       LUTry               ;if it's link go on
          mov       bx,ULINKOFF         ;get value to clear UMB links
          mov       cl,LFIRST + FIRSTFIT  ;set for first fit in low mem
          ;
LUTry:    mov       ax,(D_SETMETH shl 8) + SETULINK  ;set UMB link function
          calldos                       ;link or unlink the UMBs
           jc       LUError             ;holler if error
          mov       bl,cl               ;get fit value
          xor       bh,bh               ;clear high byte
          mov       ax,(D_SETMETH shl 8) + SETSTRAT  ;set strategy function
          calldos                       ;set new allocation strategy
           jc       LUError             ;holler if error
          mov       bx,0FFFFh           ;get too many paragraphs value
          calldos   ALLOC               ;force free block collapse
          clc                           ;show everything worked
          jmp       short LURet         ;all done
          ;
LUError:  stc                           ;didn't work, complain
          ;
LURet:    popm      cx,bx               ;restore registers
;pushf
;jmp temp_03
;msg_03 db " failed",13,10,"$"
;msg_04 db " succeeded",13,10,"$"
;temp_03:
;pushm ax,dx,ds
;loadseg ds,cs
;jnc temp_04
;mov dx,offset msg_03
;jmp temp_05
;temp_04:
;mov dx,offset msg_04
;temp_05:
;calldos message
;popm ds,dx,ax
;popf
          exit                          ;and return
          ;
          ;
%         ifdifi    <_CSEG>,<LOAD>      ;don't assemble in loader, just
                                        ;  in server
          ;
          ; FLinkUMB - Far interface to LinkUMB
          ;
          ; On entry:
          ;         Same as for LinkUMB
          ;
          ; On exit:
          ;         Same as for LinkUMB
          ;
          entry     FLinkUMB,noframe,far
          call      LinkUMB             ;just pass it along
          exit                          ;that's all
          ;
          endif
          ;
@curseg   ends                          ;close text segment
          ;
          end

