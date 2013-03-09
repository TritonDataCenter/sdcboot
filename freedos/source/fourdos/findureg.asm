

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


          title     FINDUREG - Find all UMB regions
          ;
          page      ,132                ;make wide listing
          ;
          comment   }

          Copyright 1992, JP Software Inc., All Rights Reserved

          Author:  Tom Rawson  12/23/92


          Finds the UMB regions and saves their addresses.

          } end description
          ;
          ;
          ; Includes
          ;
          include   product.asm         ;product / platform definitions
          include   trmac.asm           ;general macros
          ;
          .cseg     LOAD                ;set LOAD_TEXT segment if not defined
                                        ;  externally
          ;
          include   4dlparms.asm        ;loader / server parameters
          include   4dlstruc.asm        ;data structures
          include   model.inc           ;Spontaneous Assembly memory models
          ;
          ;
          .defcode  ,1                  ;set up code segment, no DGROUP
          ;
          ;
          ; Flag bits in DL during FindUReg
          ;
NOREG     equ       80h                 ;no regions have been found yet
INREG     equ       40h                 ;we are currently in a UMB region
ENDCHN    equ       20h                 ;this block is the end of the chain
          ;
          ;
          ; FindUReg - Find all UMB regions
          ;
          ; On entry:
          ;         CX = maximum number of regions (size of RegionTable)
          ;         SI = address of region table (table of of RegInfo data)
          ;
          ; On exit:
          ;         AX = number of regions found, 0 if error
          ;         BX, CX, DX, SI, DI, ES destroyed
          ;         All other registers and interrupt state unchanged
          ;
          ;
          entry     FindUReg,noframe    ;define entry point
          ;
          call      URVerChk            ;check DOS version
           jc       FUErr2              ;if regions not supported get out
          ;
          pushm     cx,si               ;save incoming arguments
          int       12h                 ;get top of DOS RAM
          shln      ax,6,cl             ;convert K to paragraphs
          dec       ax                  ;top - 1
          popm      si,cx               ;restore incoming arguments
          push      cx                  ;save max region count
          mov       dl,NOREG            ;set no region found flag, clear
                                        ;  others
          ;
          ; Check next block for bad flag or end of chain.  Note a 'Z' flag
          ; is OK at the start of the chain since the memory chain into UMBs
          ; may not be open.
          ;
FULoop:   mov       es,ax               ;point to next block
          mov       bl,es:[MCB_FLAG]    ;get the flag byte
          cmp       bl,'M'              ;look like an intermediate MCB?
           je       FUChkReg            ;if so go see if region boundary
          cmp       bl,'Z'              ;how about an ending MCB?
           jne      FUError             ;if not the chain is bad
          test      dl,NOREG            ;no regions yet?
           jnz      FUChkReg            ;if on first region go on
          bset      dl,ENDCHN           ;show end of chain
          ;
          ; See if this looks like adapter ROM
          ;
FUChkReg: mov       bx,es:[MCB_OWN]     ;get owner field
          cmp       bx,8                ;is it 8?
           je       FUChkBnd            ;if so look for boundary
          cmp       bx,9                ;how about 9 (in Japan)?
           jne      FUGotUMB            ;if so look for boundary
          ;
FUChkBnd: cmp       wptr es:[MCB_NAME],'CS'  ;is it "SC" (adapter ROM)?
           jne      FUGotUMB            ;if not go on
          ;
          ; At an adapter ROM block -- if we were in a region, save its end
          ;
          test      dl,INREG            ;were we in a region?
           jz       FUNext              ;if not go on
          mov       bx,ax               ;copy address
          dec       bx                  ;point to last possible UMB MCB addr
          mov       [si].EndReg,bx      ;save as end of this region + 1
          add       si,(size RegInfo)   ;skip to next region in list
          bclr      dl,INREG            ;clear in-region
          jmp       short FUNext        ;and go on
          ;
          ; At a real UMB block (not adapter ROM) -- see if it is the
          ; first one in a region; if so, count the region and save the
          ; start address.
          ;
FUGotUMB: test      dl,INREG            ;are we already in a region?
           jnz      FUNext              ;if so go on
          dec       cx                  ;new region -- count it
           jb       FUError             ;give up if too many
          mov       [si].BegReg,ax      ;at start of region, save address
          bset      dl,INREG            ;set in region flag
          bclr      dl,NOREG            ;clear no region
          ;
FUNext:   mov       bx,es:[MCB_LEN]     ;get length
          inc       bx                  ;add to count this MCB
           jz       FUError             ;if wrap-around give up
          add       ax,bx               ;skip to next MCB
           jc       FUError             ;if overflow get out
          test      dl,ENDCHN           ;at end of chain?
           jz       FULoop              ;if not continue
          test      dl,INREG            ;were we in a region?
           jz       FUDone              ;if not get out
          dec       ax                  ;back up to last paragraph in region
          mov       [si].EndReg,ax      ;save address of last possible MCB
          jmp       short FUDone        ;and get out
          ;
FUError:  pop       ax                  ;throw away saved max region count
          ;
FUErr2:   xor       ax,ax               ;show no regions
          jmp       short FUExit        ;get out
          ;
FUDone:   pop       ax                  ;get back max region count
          sub       ax,cx               ;get number of actual regions
          ;
FUExit:   exit                          ;all done
          ;
          ;
          ; URVerChk - Check DOS version for all above routines
          ;
          ; On entry:
          ;         No requirements.
          ;
          ; On exit:
          ;         Carry clear if DOS version supports regions; set if not.
          ;         All registers and interrupt state unchanged
          ;
          ;
          entry     URVerChk,noframe,,local  ;define entry point
          ;
          pushm     ax,bx,cx            ;save registers
          ;
          mov       ax,DRCHECK          ;get DR-DOS check code
          calldos                       ;check for it
           jnc      UVError             ;if DR-DOS don't allow it
          calldos   VERSION             ;get MS-DOS / PC-DOS version
          cmp       al,5                ;DOS 5 or above?
           jb       UVError             ;if not get out w/ carry set
          cmp       al,10               ;OS/2?
           je       UVError             ;if so get out
          clc                           ;show it's OK
          jmp       short UVDone        ;get out
          ;
UVError:  stc                           ;show error
          ;
UVDone:   popm      cx,bx,ax            ;restore registers
          ;
          exit                          ;all done
          ;
          ;
@curseg   ends                          ;close text segment
          ;
          end

