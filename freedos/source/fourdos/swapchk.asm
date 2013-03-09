

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


          title     SWAPCHK - Swap one block, also perform checksums
          ;
          page      ,132                ;make wide listing
          ;
          comment   }

          Copyright 1988 - 1991, JP Software Inc., All Rights Reserved

          Author:  Tom Rawson  5/30/91
                   (Slightly revised from earlier SERVER.ASM code)

          These routines swap a single block of 4DOS in or out.  The public
          interface is via SwapOne below.  All routines rely heavily on the
          SwapCtrl structure in 4DLSTRUC.ASM.

          NOTE:  These routines may NOT assume that SS = DS!  This is
          because the server switches to the resident portion's stack
          before swapping 4DOS out.

          } end description
          ;
          ;
          ; Includes
          ;
          include   product.asm         ;product / platform definitions
          include   trmac.asm           ;general macros
          .cseg     SERV                ;set server segment if not defined
                                        ;  externally
          include   model.inc           ;Spontaneous Assembly memory models
          include   4dlparms.asm        ;loader / server parameters
          include   4dlstruc.asm        ;loader / server data structures
          ;
          ;
          .defcode  ,1                  ;set up code segment, no DGROUP
          ;
          assume    cs:@curseg, ds:nothing, es:nothing, ss:nothing
          ;
          ;
          ; Local data
          ;
          ;         Swap table tells swapper to call, based on swap type
          ;
SwapTab   label     word                ;start swap table
          dw        EMSSwap             ;0 - EMS
          dw        XMSSwap             ;1 - XMS
          dw        DiskSwap            ;2 - disk
          ;
          ;
          ; SWAPONE - Call the appropriate swapper for a swap control block.
          ;           Set up as a far routine as it is used both by the
          ;           initialization code and the server.
          ;
          ; On entry:
          ;         DL = 0 to copy from base memory to swap area
          ;              1 to copy from swap area to base memory
          ;              2 to do checksums only
          ;         SI = address of swap control block
          ;         ES = PSP segment
          ;
          ; On exit:
          ;         Carry clear if all OK, set if error
          ;         AX = if carry clear:
          ;                   0 = swap performed
          ;                   1 = swap not performed due to checksum match
          ;              if carry set:
          ;                   error code
          ;         BX destroyed
          ;         Other registers and interrupt state unchanged
          ;
          entry     SwapOne,noframe,far ;define entry point
          pushm     cx,dx,si,di,ds,es   ;save registers
          ;
          mov       di,[si].DevPtr      ;get device SCB location
          mov       cx,[si].SwapMSeg    ;get main memory segment
          test      [si].SwapFlag,SEGABS  ;is segment absolute?
           jnz      SOGo                ;if so go on
          mov       bx,cs               ;copy CS as base
          add       cx,bx               ;add base to get absolute segment
          ;
SOGo:     mov       bl,[di].SwapType    ;get swap type
          xor       bh,bh               ;clear high byte
          shl       bx,1                ;make word offset
          call      wptr SwapTab[bx]    ;call the swapper for this block
          ;
          popm      es,ds,di,si,dx,cx   ;restore registers
          exit                          ;all done
          ;
          ;
          ; EMSSWAP - Swap 4DOS to/from EMS
          ;
          ; On entry:
          ;         CX = main memory segment for swap
          ;         DL =  0 to copy from base memory to EMS
          ;               1 to copy from EMS to base memory
          ;         SI = region swap control block address
          ;         DI = device swap control block address
          ;         DS = server segment
          ;
          ; On exit:
          ;         Carry clear if all OK, set if error
          ;         AX = error code if carry set, otherwise destroyed
          ;         BX, CX, DX, DI, SI, ES destroyed
          ;         Other registers and interrupt state unchanged
          ;
          entry     EMSSwap,,,local     ;swap to/from EMS
          assume    ds:@curseg          ;set assumes
          ;
          varB      EMSDir              ;saved swap direction
          varW      CurOff              ;current EMS offset (paragraphs)
          varW      CurLen              ;remaining data length (paragraphs)
          varW      EMSMSeg             ;current base memory segment
          varW      LocHdl              ;local save of handle
          varend                        ;end of variables
          ;
          mov       EMSDir,dl           ;save swap direction
          mov       EMSMSeg,cx          ;save main memory segment address
          mov       dx,[di].SwapHdl     ;get handle
          mov       LocHdl,dx           ;save it here
          callems   SMAP                ;save the current mapping state
          mov       ax,[si].SwapOff     ;get XMS offset in paragraphs
          mov       CurOff,ax           ;save as current offset
          mov       ax,[si].SwapLen     ;get length of block
          mov       CurLen,ax           ;save as current (remaining) length
          ;
EMSLoop:  mov       bx,400h             ;get paragraphs per page
          mov       ax,CurOff           ;get offset for start of swap
          xor       dx,dx               ;clear high word
          div       bx                  ;start page in AX, par. offset in DX
          mov       di,dx               ;save paragraph offset within page
          push      ax                  ;save starting logical page
          mov       ax,CurLen           ;get length remaining
          mov       cx,ax               ;assume transferring all of it
          add       ax,di               ;convert it to last xfer par. + 1
          xor       dx,dx               ;clear high word
          div       bx                  ;page count in AX, final par. in DX
          or        dx,dx               ;check remainder
          jz        ChkFrame            ;if none, don't increment
          inc       ax                  ;bump count to include last page
          ;
ChkFrame: cmp       ax,EMSMPAG          ;room for all pages?
          jle       EMSLen              ;if not using entire frame, go on
          mov       ax,EMSMPAG          ;frame overflow, limit to max pages
          mov       cx,EMSMPAR          ;limit transfer count as well
          sub       cx,di               ;adjust transfer for offset
          ;
EMSLen:   pop       bx                  ;restore starting logical page to bx
          push      cx                  ;save transfer paragraph count
          mov       cx,ax               ;put page count in cx
          mov       dx,LocHdl           ;get handle
          xor       al,al               ;clear physical page number
          ;
MapLoop:  push      ax                  ;save physical page
          callems   MAP                 ;map the page
          or        ah,ah               ;check if any error
           jne      MapErr              ;if so holler
          pop       ax                  ;get back physical page
          inc       al                  ;bump physical page
          inc       bx                  ;bump logical page
          loop      MapLoop             ;keep mapping pages
          ;
          ; Set registers as if swapping to EMS.  AX:SI is the source
          ; address; BX:DI is the destination (DI is set above).
          ;
          pop       cx                  ;get back transfer paragraph count
          pushm     cx,si,ds            ;save registers
          mov       ax,EMSMSeg          ;get current memory segment
          mov       bx,[si].DevPtr      ;get device SCB address
          mov       bx,wptr [bx].SwapSeg  ;get frame segment
          xor       si,si               ;offset in base memory is always zero
          shln      di,4                ;convert EMS offset to bytes
          cmp       bptr EMSDir,0       ;check direction
          je        EMSGo               ;if swapping to EMS go on
          ;
          ; Swapping to base memory -- switch registers
          ;
          xchg      ax,bx               ;swap segments
          xchg      si,di               ;swap offsets
          ;
          ; Set segment registers and do the swap
          ;
EMSGo:    mov       ds,ax               ;set source segment
          mov       es,bx               ;set destination segment
          assume    ds:nothing          ;fix assumes
          shln      cx,3                ;convert paragraphs to words
          rep       movsw               ;copy the block
          popm      ds,si,cx            ;restore registers
          assume    ds:@curseg          ;fix assumes
          add       CurOff,cx           ;bump EMS offset
          add       EMSMSeg,cx          ;bump memory segment
          sub       CurLen,cx           ;reduce remaining length
          jbe       EMSDone             ;quit if no more to transfer
          jmp       EMSLoop             ;otherwise loop
          ;
EMSDone:  mov       dx,LocHdl           ;get EMS handle
          callems   RMAP                ;restore the mapping state
          xor       ax,ax               ;show swap performed, clear carry
          jmp       short EMSExit       ;all done
          ;
MapErr:   mov       ax,E_EMSMAP         ;get error code
          stc                           ;show error
          ;
EMSExit:  exit                          ;all done
          ;
          ;
          ; XMSSWAP - Swap 4DOS to/from XMS
          ;
          ; On entry:
          ;         CX = main memory segment for swap
          ;         DL =  0 to copy from base memory to XMS
          ;               1 to copy from XMS to base memory
          ;         SI = region swap control block address
          ;         DI = device swap control block address
          ;
          ; On exit:
          ;         Carry clear if all OK, set if error
          ;         AX = error code if carry set, otherwise destroyed
          ;         BX, CX, DX, DI, SI, ES destroyed
          ;         Other registers and interrupt state unchanged
          ;
          entry     XMSSwap,,,local     ;swap to/from XMS
          ;
          varD      XMSDrvr             ;dword ptr to XMS driver
          var       XMSMove,<size XMovData>  ;set up XMS move control block
          varend                        ;end of local variables
          ;
          dload     ax,bx,[di].SwapDrvr ;get driver address
          dstore    XMSDrvr,ax,bx       ;save it locally
          push      di                  ;save device SCB address
          lea       bx,XMSMove.SrcHdl   ;point to source area
          lea       di,XMSMove.DestHdl  ;and to destination
          or        dl,dl               ;check direction
          jz        XMSSet              ;if swap out go on
          xchg      bx,di               ;swap source and destination pointers
          ;
          ; Now BX points to place for main memory info; DI to place
          ; for XMS info
          ;
XMSSet:   xor       ax,ax               ;get zero
          mov       ss:[bx].XHdl,ax     ;main memory handle is zero
          mov       ss:[bx].XOff,ax     ;so is offset
          mov       ss:[bx].XSeg,cx     ;set main memory segment
          mov       ax,[si].SwapOff     ;get XMS offset in paragraphs
          xor       dx,dx               ;clear high word
          dshln     ax,dx,4             ;convert XMS offset to bytes
          mov       ss:[di].XOff,ax     ;save low-order
          mov       ss:[di].XSeg,dx     ;save high-order
          mov       ax,[si].SwapLen     ;get swap length
          xor       dx,dx               ;clear high word
          dshln     ax,dx,4             ;convert length to bytes
          mov       wptr XMSMove.MoveLen,ax     ;save low-order
          mov       wptr XMSMove.MoveLen[2],dx  ;save high-order
          pop       si                  ;get back device SCB address
          mov       ax,[si].SwapHdl     ;get XMS handle
          mov       ss:[di].XHdl,ax     ;set XMS handle
          ;
          lea       si,XMSMove          ;point to control block
          push      ds                  ;save DS
          loadseg   ds,ss               ;set DS for XMS move control block
          callXMS   MOVE,XMSDrvr        ;move the data
          pop       ds                  ;restore DS
          or        ax,ax               ;check if any error
           jz       XMSErr              ;no error if non-zero returned
          xor       ax,ax               ;show swap performed, clear carry
          jmp       short XMSExit       ;all done
          ;
XMSErr:   mov       ax,E_XMSMOV         ;get error code
          stc                           ;error, show that
          ;
XMSExit:  exit                          ;all done
          ;
          ;
          ; DISKSWAP - Swap part of 4DOS to/from disk
          ;
          ; On entry:
          ;         CX = main memory segment for swap
          ;         DL =  0 to write to disk
          ;               1 to read from disk
          ;               2 to calculate checksum only
          ;         SI = region swap control block address
          ;         DI = device swap control block address
          ;         ES = PSP segment (if actually swapping -- not needed
          ;              for checksum only)
          ;
          ; On exit:
          ;         Carry clear if all OK, set if error
          ;         AX = error code if carry set, otherwise destroyed
          ;         BX, CX, DX, DI, SI, ES destroyed
          ;         Other registers and interrupt state unchanged
          ;
;NSMsg     db        "Checksum matched, swap in skipped", CR, LF, "$"
          ;
          entry     DiskSwap,,,local    ;swap to/from disk
          ;
          errset    DiskErr             ;error jumps go to DiskErr
          ;
          varB      DiskDir             ;saved swap direction
          varB      FileHdl             ;local file handle
          varW      DiskMSeg            ;local main memory segment
          varW      SaveSP              ;saved SP in case of error
          var       SigBuf,SIGBYTES     ;disk swap signature buffer
          varend                        ;end of variables
          ;
          push      ds                  ;save registers
          mov       SaveSP,SP           ;save SP
          ;
          mov       DiskDir,dl          ;save swap direction
          mov       DiskMSeg,cx         ;save for later
          test      [si].SwapFlag,SKIPTEST   ;skip checksum?
           jnz      DSGo                ;if so go on
          ;
          ; Checksum the region to be swapped; skip swap in if checksum
          ; matches; save checksum if calculation request.
          ;
          or        dl,dl               ;see if swap out
           je       DSGo                ;if so skip checksum
          push      di                  ;save device SCB address
          mov       ax,cx               ;copy main memory segment
          mov       cx,[di].SwapSum     ;get checksum shift / add counts
          mov       di,[si].SwapLen     ;get length of region
          xor       bx,bx               ;clear low checksum
          xor       dx,dx               ;clear high checksum
          call      CheckSum            ;calculate checksum
          cmp       bptr DiskDir,1      ;what are we doing?
           ja       SaveChk             ;if not swapping, must be checksum
                                        ;  only
          cmp       dx,wptr [si].SwapChk[2]  ;compare high-order
           jne      DSCkDone            ;if not the same go on
          cmp       bx,wptr [si].SwapChk     ;compare low-order
           jne      DSCkDone            ;if not the same go on
          ;
;          if        DEBUG               ;display message if debugging
;          pushm     ax,bx,cx,dx
;          mov       dx,offset NSMsg     ;get message address
;          calldos   MESSAGE             ;display it
;          popm      dx,cx,bx,ax
;          endif
          ;
          jmp       short DSCkExit      ;checksums match, no need to swap
          ;
SaveChk:  dstore    <wptr [si].SwapChk>,bx,dx  ;save checksum
          ;
DSCkExit: pop       di                  ;restore DI
          mov       ax,1                ;show swap not performed
          clc                           ;show no error
          jmp       DiskExit            ;all done
          ;
DSCkDone: pop       di                  ;restore DI
          ;
          ; Find a file handle if we need to
          ;
DSGo:     mov       bptr FileHdl,0      ;clear handle
          mov       bx,[di].SwapHdl     ;get file handle (stays in bx
                                        ;  throughout)
          test      bh,80h              ;is it a system file handle?
          jz        CheckSig            ;if not go on
          mov       dl,bl               ;save system handle
          mov       cx,FHTSIZE          ;get length of handle table
          mov       bx,PSP_FTAB         ;point to table
          ;
FTLoop:   cmp       bptr es:[bx],0FFh   ;is this entry free?
          je        GotFH               ;yes, we got a handle
          inc       bx                  ;next entry
          loop      FTLoop              ;keep looking
          error     NOHDL               ;no handle, complain
          ;
GotFH:    mov       es:[bx],dl          ;store system handle in local table
          sub       bx,PSP_FTAB         ;make it a real handle
          mov       bptr FileHdl,1      ;set handle used flag
          calldos   DUPHDL,NOHDL        ;make a copy of it (avoids problems
                                        ;  with DV closing files on us)
          mov       bptr es:[PSP_FTAB][bx],0FFh  ;release process file handle
          mov       bx,ax               ;copy duplicated handle
          ;
          ; Check if the disk file signature (at start of file) is intact
          ;
CheckSig: test      [si].SwapFlag,SKIPTEST  ;skip signature?
          jnz       DiskGo              ;if so go on
          pushm     si,es               ;save registers we need later
          xor       cx,cx               ;clear high word
          xor       dx,dx               ;clear low word
          mov       al,SEEK_BEG         ;seek is relative to start
          calldos   SEEK,SWSEEK         ;seek to beginning of file
          lea       dx,SigBuf           ;point to buffer
          mov       si,dx               ;save address for compare
          mov       cx,SIGBYTES         ;get signature length
          lea       di,[di].SwapSig     ;get signature offset
          loadseg   es,ds               ;get signature segment
          loadseg   ds,ss               ;set DS for read (buffer on stack)
          calldos   READ,SWREAD         ;read the test string from disk
          mov       cx,SIGBYTES         ;get signature length
          repe      cmpsb               ;is file OK?
          loadseg   ds,es               ;restore DS
          je        SigOK               ;continue if file is OK
          error     <SWDBAD + 8000h>    ;if not give up with fatal flag set
          ;
          ; Disk signature is OK -- swap things in or out
          ;
SigOK:    popm      es,si               ;restore registers
          ;
DiskGo:   mov       dx,[si].SwapOff     ;get starting segment offset
          xor       cx,cx               ;clear high word
          dshln     dx,cx,4             ;convert to byte offset
          add       dx,SIGBYTES         ;skip signature area
          adc       cx,0                ;add any carry
          mov       al,SEEK_BEG         ;seek is relative to start
          calldos   SEEK,SWSEEK         ;seek to transfer start
          ;
          mov       cx,[si].SwapLen     ;get swap length in paragraphs
          mov       dx,DiskMSeg         ;get starting segment
          mov       ds,dx               ;copy as base segment
          assume    ds:nothing          ;no assume any more
          ;
DiskLoop: push      cx                  ;save remaining paragraphs
          maxu      cx,DISKXMAX         ;reduce to max transfer size
          push      cx                  ;save transfer paragraph count
          shln      cx,4                ;make it a byte count
          xor       dx,dx               ;start at beginning of segment
          cmp       bptr DiskDir,0      ;check direction
          jne       DiskIn              ;if swapping in go on
          calldos   WRITE,SWWRIT        ;write the block to disk
          jmp       short DiskCont      ;and continue with loop
          ;
DiskIn:   calldos   READ,SWREAD         ;read the block from disk
          ;
DiskCont: pop       dx                  ;restore actual paragraph count
          mov       ax,ds               ;copy transfer segment
          add       ax,dx               ;increment it
          mov       ds,ax               ;put it back
          pop       cx                  ;restore remaining paragraph count
          sub       cx,dx               ;decrement it
          ja        DiskLoop            ;loop if there is more to transfer
          ;
          cmp       bptr FileHdl,0      ;file handle used?
           je       DiskOK              ;if not go on
          calldos   CLOSE               ;close the duplicated handle
          ;
DiskOK:   xor       ax,ax               ;show swap performed, no error
          jmp       short DiskExit      ;and exit
          ;
DiskErr:  mov       ax,dx               ;error code to AX
          mov       sp,SaveSP           ;reset stack
          stc                           ;show error
          ;
DiskExit: pop       ds                  ;restore registers
          assume    ds:@curseg          ;ds is back to code segment
          ;
          exit                          ;all done
          ;
          ;
          ; CHECKSUM - Calculate checksum for a region of memory
          ;           Set up as a far routine as it is used both by the
          ;           initialization code and the server.
          ;
          ; Note:  The checksum calculated is the 32-bit sum of the
          ; 16-bit words in the region to be checksummed.  The actual data
          ; checksummed is determined by the add and shift counts passed
          ; in to this code.  The add count tells how much to increment 
          ; the address before summing the next word.  The shift count tells 
          ; how many bits to shift a paragraph count to make it a count of
          ; words to be summed.  This allows for adjusting the "granularity"
          ; of the checksum.  The possible values of SumAdd and SumShift 
          ; are as follows:
          ;
          ;         Add Cnt   Shift Cnt  Words checksummed
          ;         ------    --------   -----------------
          ;           14         0       every eighth word
          ;            6         1       every fourth word
          ;            2         2       every other word
          ;            0         3       every word
          ;
          ; On entry:
          ;         AX = segment address of region to checksum
          ;         CL = add count
          ;         CH = shift count
          ;         DX:BX = old checksum to add new region to
          ;         DI = number of paragraphs to checksum
          ;         DS = local segment
          ;
          ; On exit:
          ;         DX:BX = updated checksum
          ;         AX, CX destroyed
          ;         Other registers and interrupt state unchanged
          ;
          ;
          entry     CheckSum            ;calculate checksum on a region
          assume    ds:@curseg          ;fix assumes
          ;
          varB      ShiftCnt            ;saved shift count
          varW      AddCnt              ;saved add count
          varend                        ;end of stack variables
          ;
          pushm     si,bp,ds            ;save registers
          mov       ShiftCnt,ch         ;save shift count
          xor       ch,ch               ;clear high byte of add count
          mov       AddCnt,cx           ;copy add count
          mov       ds,ax               ;set ds to checksum area
          assume    ds:nothing          ;fix assumes
          ;
SumLoop:  mov       ax,di               ;copy remaining paragraphs
          maxu      ax,SUMMAX           ;reduce to max sum size
          pushm     ax,bp               ;save sum paragraph count and BP
          mov       cl,ShiftCnt         ;get shift amount
          mov       bp,AddCnt           ;get add count
          shl       ax,cl               ;make it a loop count
          mov       cx,ax               ;copy to cx
          xor       si,si               ;start at base of segment
          ;
SumPart:  lodsw                         ;load a word
          add       bx,ax               ;add to sum
          adc       dx,0                ;add any carry
          add       si,bp               ;skip some words if needed
          loop      SumPart             ;loop until this part is done
          ;
          popm      bp,cx               ;get back BP and paragraph count
          mov       ax,ds               ;copy sum segment
          add       ax,cx               ;increment it
          mov       ds,ax               ;put it back
          sub       di,cx               ;decrement remaining paragraph count
          ja        SumLoop             ;loop if there is more to sum
          ;
          popm      ds,bp,si            ;restore registers
          exit                          ;all done
          ;
          ;
@curseg   ends                          ;close segment
          ;
          ;
          end

