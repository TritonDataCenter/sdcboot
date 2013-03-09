

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


          title     UMBREG - Provide support for UMB regions
          ;
          page      ,132                ;make wide listing
          ;
          comment   }

          Copyright 1992, JP Software Inc., All Rights Reserved

          Author:  Tom Rawson  12/23/92


          These routines allow allocation of space in specific UMB
          regions.

          } end description
          ;
          ;
          ; Includes
          ;
          include   product.asm         ;product / platform definitions
          include   trmac.asm           ;general macros
          ;
          .cseg     ASM                 ;set ASM_TEXT segment if not defined
                                        ;  externally
          ;
          include   4dlparms.asm        ;loader / server parameters
          include   4dlstruc.asm        ;data structures
          include   model.inc           ;Spontaneous Assembly memory models
          ;
          ;
          extrn     FLinkUMB:far        ;define outside all segments
          ;
          .defcode  ,1                  ;set up code segment, no DGROUP
          ;
          ;
          ; SetUReg - Set up UMBs so next program load takes place within
          ;           a particular region or region(s)
          ;
          ; On entry:
          ;         Arguments on stack, pascal calling convention:
          ;           int Region                  FFFF for UMB link only,
          ;                                       0 for multi-region load,
          ;                                       region number for single
          ;                                       region
          ;           int Shrink                  0 to leave open, 1 to
          ;                                       shrink regions to min free
          ;                                       size
          ;           int Link                    1 to leave UMBs linked, 0
          ;                                       to unlink
          ;           int MaxRegion               maximum number of regions
          ;           RegInfo _far RegionTable[]  table of RegInfo data
          ;
          ; On exit:
          ;         AX = number of UMB regions made available, 0 if any
          ;              error occurred
          ;         BX, CX, DX destroyed
          ;         All other registers and interrupt state unchanged
          ;         DOS UMBs linked into MCB chain
          ;
          ;
          entry     SetUReg,varframe,far  ;define entry point
          ;
          argD      RTAddr              ;address of region table
          argW      MaxRegn             ;max entries in region table (only
                                        ;  uses 1 byte)
          argW      Link                ;link flag (only uses 1 byte)
          argW      Shrink              ;shrink flag (only uses 1 byte)
          argW      Region              ;load into this region, 0 for mult
                                        ;  (only uses 1 byte)
          ;
          varW      PSPSeg              ;our PSP segment
          varW      CurReg              ;current region (only uses 1 byte)
          varW      RegCnt              ;count of valid regions
          varW      LBSize              ;paragraph size of largest free block in
                                        ;  region
          varW      LBAddr              ;MCB for largest free block in region
          varend
          ;
          pushm     si,di,ds,es         ;save registers
          ;
          ; Link in the UMBs
          ;
          mov       ah,1                ;get value to set links
          call      FLinkUMB            ;link in UMBs and set strategy
           jnc      URLinkOK            ;go on if OK
          xor       ax,ax               ;show error
          jmp       URExit              ;get out if error
          ;
          ; Set up for region scan
          ;
URLinkOK: mov       bx,0FFFFh           ;get max paragraphs
          calldos   ALLOC               ;collapse free blocks
          ;
          cmp       bptr MaxRegn,0      ;any regions?
           je       URExitOK            ;if not just exit
          cmp       wptr Region,0FFFFh  ;link only?
           jne      URCVer              ;if not go on
          ;
URExitOK: mov       ax,1                ;show no error
          jmp       URDone              ;and get out
          ;
URCVer:   call      URVerChk            ;check DOS version
           jnc      URSetup             ;if supported go on
          jmp       URError             ;otherwise get out
          ;
URSetup:  mov       bptr CurReg,1       ;start at region 1
          lds       si,RTAddr           ;point to region table
          calldos   GETPSP              ;get our PSP
          mov       PSPSeg,bx           ;copy it
          mov       wptr RegCnt,0       ;clear region counter
          ;
          ; Region loop.  See if we have more regions to process.  If
          ; so figure out min free space for the current region.
          ;
URRLoop:  mov       al,Region           ;get desired region
          or        al,al               ;test
           jz       URChkReg            ;if multiple regions it's already
                                        ;  in table
          mov       dx,0FFFFh           ;assume region should be locked
          cmp       al,CurReg           ;is it this region?
           jne      URPutMin            ;if not go on
          xor       dx,dx               ;use this region ==> no min free
          ;
URPutMin: mov       [si].MinFree,dx     ;save new minimum in table
          ;
          ; Set up for block scan.
          ;
URChkReg: xor       ax,ax               ;get zero
          mov       LBSize,ax           ;clear largest block size
          mov       [si].FreeList,ax    ;assume nothing free
          mov       ax,[si].BegReg      ;get first UMB MCB in region
          xor       bx,bx               ;clear back pointer
          ;
          ; Block loop.  Check block status, and size if free.  In this
          ; loop AX = current block address, BX = last free block address,
          ; CX = current block size, DX = scratchpad
          ;
URBLoop:  mov       es,ax               ;point to block
          mov       cx,es:[MCB_LEN]     ;get block length in CX
           jcxz     URBNext             ;ignore 0-byte blocks
          cmp       wptr es:[MCB_OWN],0 ;block free?
           jne      URBNext             ;if not go on
          mov       dx,PSPSeg           ;get our PSP
          mov       es:[MCB_OWN],dx     ;reserve the block for us
          mov       es:[MCB_BACK],bx    ;set the back pointer
          mov       wptr es:[MCB_FWD],0 ;assume no forward pointer
          call      AdjFwd              ;adjust forward pointer in prev block
          mov       bx,ax               ;save new back pointer
          ;
URChkBlk: cmp       wptr [si].MinFree,0FFFFh ;using whole region?
           je       URBNext             ;if so, don't care about largest blk
          cmp       cx,LBSize           ;is this bigger than largest block?
           jbe      URBNext             ;if not go on
          mov       LBSize,cx           ;save new largest size
          mov       LBAddr,ax           ;and MCB address
          ;
URBNext:  cmp       es:[MCB_FLAG],'Z'   ;end of chain?
           je       URBDone             ;if so block scan is done
          inc       ax                  ;skip our MCB
          add       ax,cx               ;skip block data
          cmp       ax,[si].EndReg      ;past end of region?
           jb       URBLoop             ;if not go on to next block
          ;
          ; All blocks in this region have been scanned.  See if largest
          ; block is big enough.
          ;
URBDone:  mov       dx,LBSize           ;get largest block size
          sub       dx,[si].MinFree     ;big enough to hold minimum?
           ja       URShrChk            ;if so go on
          jmp       URRNext             ;if not go to next region
          ;
          ; There is enough room in this region.  See if we need to
          ; reduce size of largest block.
          ; 
URShrChk: inc       wptr RegCnt         ;count good regions
          cmp       bptr Shrink,0       ;big enough -- should we shrink it?
           jz       URRFree             ;if not go free the whole thing
          cmp       dx,LBSize           ;is min free set to 0?
           je       URRFree             ;if so ignore it, free all
          ;
          ; Shrink the largest block to match the minimum free space
          ;
          mov       es,LBAddr           ;get largest block address
          cmp       dx,1                ;room for extra space + 1 MCB?
           ja       URRSplit            ;if so go split it
          ;
          ; Largest block is just barely big enough, just free it
          ;
          mov       wptr es:[MCB_OWN],0 ;free all of largest block
          mov       ax,es:[MCB_FWD]     ;get next block pointer
          mov       bx,es:[MCB_BACK]    ;get prev block pointer
          call      AdjFwd              ;adjust forward pointer in prev blk
          xchg      ax,bx               ;swap pointers
          call      AdjBack             ;adjust back pointer in next block
          jmp       short URRNext       ;and go on to next region
          ;
          ; Split the largest block into two parts -- DX has size of second
          ; part + 1
          ;
URRSplit: mov       wptr es:[MCB_OWN],0 ;free all of largest block
          mov       bx,[si].MinFree     ;get min free space size
          mov       es:[MCB_LEN],bx     ;force this block to min size
          mov       cl,'M'              ;get new flag byte
          xchg      cl,es:[MCB_FLAG]    ;set new, get old
          push      wptr es:[MCB_FWD]   ;save old forward pointer
          push      wptr es:[MCB_BACK]  ;save old back pointer
          mov       ax,es               ;get current address
          inc       ax                  ;skip current MCB
          add       ax,bx               ;skip to new block
          mov       es,ax               ;set ES to it
          mov       es:[MCB_FLAG],cl    ;set flag
          dec       dx                  ;adjust length for MCB
          mov       es:[MCB_LEN],dx     ;set length
          mov       dx,PSPSeg           ;get our PSP
          mov       es:[MCB_OWN],dx     ;set owner
          pop       bx                  ;get old back pointer
          mov       es:[MCB_BACK],bx    ;set new back pointer
          mov       ax,es               ;get current free block address
          call      AdjFwd              ;adjust forward pointer in prev blk
          pop       bx                  ;get old forward pointer
          mov       es:[MCB_FWD],bx     ;set new forward pointer
          call      AdjBack             ;adjust back pointer in next block
          jmp       short URRNext       ;go on to next region
          ;
          ; Not shrinking -- free the entire region
          ;
URRFree:  xor       ax,ax               ;get zero
          xchg      ax,[si].FreeList    ;clear free list head, get old value
          ;
URRFLoop: or        ax,ax               ;anything there?
           jz       URRNext             ;if not we're done
          mov       es,ax               ;set segment
          mov       wptr es:[MCB_OWN],0 ;free the block
          mov       ax,es:[MCB_FWD]     ;move to next block
          jmp       short URRFLoop      ;and loop for more
          ;
          ; This region is done -- go on to the next
          ;
URRNext:  inc       bptr CurReg         ;bump current region
          mov       bl,CurReg           ;get current region
          mov       ax,RegCnt           ;get return value in case we're done
          cmp       bl,MaxRegn          ;done with all regions?
           ja       URDone              ;if so get out w/ AX=0
          add       si,(size RegInfo)   ;skip to next region in table
          jmp       URRLoop             ;and go process it
          ;
          ; Error occurred
          ;
URError:  xor       ax,ax               ;get error code
          ;
          ; All done
          ;
URDone:   cmp       bptr Link,0         ;leave things linked?
           jne      URExit              ;and exit
          push      ax                  ;save exit code
          xor       ah,ah               ;get value to unlink
          call      FLinkUMB            ;unlink UMBs and reset strategy
          pop       ax                  ;restore exit code
          ;
URExit:   popm      es,ds,di,si         ;restore registers
          ;
          exit                          ;all done
          ;
          ;
          ; AdjFwd - Adjust forward pointer in a free block MCB so that it
          ;          points to a new location
          ;
          ; On entry:
          ;         AX = new pointer value
          ;         BX = free block MCB address
          ;         SI = region table pointer
          ;
          ; On exit:
          ;         All registers and interrupt state unchanged
          ;         Pointer adjusted if AX was <> 0
          ;
          ;
          entry     AdjFwd,noframe      ;define entry point
          ;
          or        bx,bx               ;is there a block there?
           jz       AFTop               ;if not we're at top of list
          push      es                  ;save ES
          mov       es,bx               ;copy block address
          mov       es:[MCB_FWD],ax     ;set pointer
          pop       es                  ;restore
          jmp       short AFDone        ;and get out
          ;
AFTop:    mov       [si].FreeList,ax    ;store as new top of list
          ;
AFDone:   exit                          ;done
          ;
          ;
          ; AdjBack - Adjust back pointer in a free block MCB so that it
          ;           points to a new location
          ;
          ; On entry:
          ;         AX = new pointer value
          ;         BX = free block MCB address
          ;         SI = region table pointer
          ;
          ; On exit:
          ;         All registers and interrupt state unchanged
          ;         Pointer adjusted if AX was <> 0
          ;
          ;
          entry     AdjBack,noframe     ;define entry point
          ;
          or        bx,bx               ;is there a block there?
           jz       ABDone              ;if not get out
          push      es                  ;save ES
          mov       es,bx               ;copy block address
          mov       es:[MCB_BACK],ax    ;set pointer
          pop       es                  ;restore
          ;
ABDone:   exit                          ;done
          ;
          ;
          ; FreeUReg - Free UMBs reserved by SetUReg
          ;
          ; On entry:
          ;         Arguments on stack, pascal calling convention:
          ;           int LinkOnly                FFFF for UMB unlink only,
          ;                                       anything else for normal
          ;                                       UMB free
          ;           int MaxRegion               maximum number of regions
          ;           RegInfo _far RegionTable[]  table of RegInfo data
          ;
          ; On exit:
          ;         AX, BX, CX, DX destroyed
          ;         All other registers and interrupt state unchanged
          ;         DOS UMBs unlinked from MCB chain
          ;
          ;
          entry     FreeUReg,argframe,far  ;define entry point
          ;
          argD      RTAddr              ;address of region table
          argW      MaxRegn             ;max entries in region table
          argW      LinkOnly            ;link only flag
          ;
          pushm     si,di,ds,es         ;save registers
          ;
          cmp       wptr LinkOnly,0FFFFh  ;link only?
           je       UFUnlink            ;if not go on
          call      URVerChk            ;check DOS version
           jc       UFDone              ;if regions not supported get out
          mov       cx,MaxRegn          ;get loop counter
           jcxz     UFUnlink            ;if no regions we're done
          lds       si,RTAddr           ;get region table address
          ;
          ; Scan regions and free all blocks reserved by UMBReg
          ;
UFRLoop:  xor       ax,ax               ;get zero
          xchg      [si].FreeList,ax    ;set none free, get free list start
          ;
UFBLoop:  or        ax,ax               ;any block to free?
           jz       UFNext              ;if not we're done
          mov       es,ax               ;set segment
          mov       wptr es:[MCB_OWN],0 ;free the block
          mov       ax,es:[MCB_FWD]     ;move to next block
          jmp       short UFBLoop       ;and loop for more
          ;
UFNext:   add       si,(size RegInfo)   ;skip to next region in table
          loop      UFRLoop             ;loop for all regions
          ;
          ; Unlink the DOS UMBs
          ;
UFUnlink: xor       ah,ah               ;get value to clear links
          call      FLinkUMB            ;unlink UMBs and reset strategy
          ;
UFDone:   popm      es,ds,di,si         ;restore registers
          ;
          exit                          ;all done
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