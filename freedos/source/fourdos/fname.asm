

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


          title     FNAME - File name parsing / testing routines
          ;
          page      ,132                ;make wide listing
          ;
          comment   }

          Copyright 1991, JP Software Inc., All Rights Reserved

          Author:  Tom Rawson  3/23/91


          These routines perform the following functions related to filename
          processing.  FNCheck, FNorm, and FNType generate disk accesses as
          they call DOS to get the current directory and look for the file.

             FNCheck    Check filename type, separate path part
             FNorm      "Normalize" a filename (insert drive/dir)
             FNType     Determine filename type (old file, new file, or
                          directory)
             AZLen      Get length of an ASCIIZ string


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
          ; FNCheck - Check filename type, separate path part
          ;
          ;
          ; On entry:
          ;         DS:SI = address of ASCIIZ filename string
          ;
          ; On exit:
          ;         Carry set if file not found or error, clear if found
          ;         AL = file attribute if no error
          ;         CX = full name length including trailing '\' (if any)
          ;              and null
          ;         DX = length of path part including trailing '\'
          ;         SI = offset of filename part if name was a file
          ;         AH, BX destroyed
          ;         All other registers and interrupt state unchanged
          ;
          ; Notes:
          ;         An error is generated as in FNType below.
          ;         
          ;
          entry     FNCheck,noframe     ;set up entry point
          ;
          pushm     di,es               ;save registers
          mov       di,si               ;save start address
          call      FNorm               ;normalize the name
          mov       si,di               ;get start address again
          call      FNType              ;determine the type
          jc        FCDone              ;pass on any error
          pushm     ax,cx               ;save attribute and count
          mov       si,di               ;get back start address
          dec       cx                  ;get count not including terminator
          add       si,cx               ;point to terminator
          std                           ;go backward
          ;
FCLoop:   lodsb                         ;get a character
          cmp       al,'\'              ;backslash?
          je        FCFound             ;if so we got it
          cmp       al,'/'              ;how about forward?
          je        FCFound             ;if so we got it
          loop      FCLoop              ;otherwise loop
          mov       si,di               ;get start address to force 0 length
          jmp       short FCCalc        ;and go on
          ;
FCFound:  add       si,2                ;point SI back to separator + 1
          ;
FCCalc:   mov       dx,si               ;get end address
          sub       dx,di               ;get length of path part
          popm      cx,ax               ;get back full length and attribute
          cld                           ;don't leave direction flag set!
          clc                           ;show no error
          ;
FCDone:   popm      es,di               ;restore registers
          exit                          ;all done
          ;
          ;
          ; FNorm - "Normalize" a filename (insert drive/dir)
          ;
          ;
          ; On entry:
          ;         DS:SI = address of ASCIIZ filename string; must be long
          ;                 enough to hold longest possible path + filename
          ;
          ; On exit:
          ;         Filename normalized
          ;         AX, BX, CX, DX destroyed
          ;         All other registers and interrupt state unchanged
          ;
          ;
          entry     FNorm,varframe      ;set up entry point
          ;
          varW      IPtr                ;saved input offset
          var       OName,PATHBLEN      ;output file name
          varend                        ;end of local variables
          ;
          pushm     si,di,es            ;save registers
          cld                           ;go forward
          mov       IPtr,si             ;save SI
          cmp       wptr [si],'\\'      ;does spec start with \\?
          jne       NormGo              ;if not go on
          jmp       NormDone            ;if so assume server name, get out
          ;
NormGo:   call      AZLen               ;get length of actual name + 1 in CX
          dec       cx                  ;get actual length
          mov       dx,cx               ;save it
          sub       cx,2                ;get length less 2 bytes for "d:"
          jl        GetDrive            ;if 0 or 1 characters must be no
                                        ;  drive
          je        ChkDrive            ;if 2 characters skip ":" scan
          mov       di,si               ;start at beginning
          add       di,2                ;skip possible "d:" at start
          loadseg   es,ds               ;point ES to string segment
          mov       al,':'              ;get colon
          repne     scasb               ;look for embedded colon
          je        NormDone            ;if we found one assume server name,
                                        ;  get out
          ;
ChkDrive: cmp       bptr 1[si],':'      ;drive there?
          jne       GetDrive            ;if not go get it
          lodsw                         ;get first two characters
          sub       dx,2                ;adjust count remaining
          jmp       short DriveOut      ;go save them
          ;
GetDrive: calldos   GETDRV              ;get current disk drive
          add       al,'a'              ;make it ASCII
          mov       ah,':'              ;get colon
          ;
DriveOut: loadseg   es,ss               ;set output segment
          lea       di,OName            ;point to buffer
          stosw                         ;store "d:"
          ;
          cmp       bptr [si],"\"       ;does filename already start at root?
          je        CopyUser            ;if so go copy user name
          cmp       bptr [si],"/"       ;try with "/" also
          je        CopyUser            ;if so go copy user name
          pushm     dx,si,ds            ;save registers
          stocb     '\'                 ;start path with leading "\"
          mov       si,di               ;copy path destination to si
          loadseg   ds,es               ;destination segment to ds
          mov       al,-3[si]           ;get drive letter
          cmp       al,'a'              ;is it lower case?
          jb        DriveUpr            ;if not go on
          sub       al,32               ;make it upper case
          ;
DriveUpr: sub       al,'A' - 1          ;make it 1-based (1=A etc.)
          mov       dl,al               ;copy to dl for GETDIR
;;LFN
          calldos   GETDIR              ;get current directory
          popm      ds,si,dx            ;restore registers
          jc        NormDone            ;if we can't get directory give up
          xor       al,al               ;look for end of ASCIIZ directory
          cld                           ;go forward
          cmp	[di],al		;check first character
           je       CopyUser            ;if that's null current directory is
                                        ;  root, go on
	inc	di		;skip first character
          mov       cx,PATHMAX-1        ;get length to scan
          repne     scasb               ;not root, look for end of name
          jne       NormDone            ;error if not found
          dec       di                  ;back up di to point to null
          stocb     '\'                 ;add "\" separator
          ;
CopyUser: mov       cx,dx               ;copy count
          jcxz      CopyBack            ;if nothing there go copy path back
          rep       movsb               ;copy user string to output buffer
          ;
CopyBack: lea       si,OName            ;get buffer address
          mov       cx,di               ;copy end of output + 1
          sub       cx,si               ;get count
          loadseg   es,ds               ;set output segment
          mov       di,IPtr             ;get output offset
          loadseg   ds,ss               ;source on stack
          push      cx                  ;save length
          rep       movsb               ;copy from local buffer to caller's
                                        ;  buffer
          stocb     0                   ;null termination
          pop       cx                  ;restore length
          loadseg   ds,es               ;restore DS
          ;
NormDone: popm      es,di,si            ;restore registers
          ;
          exit                          ;all done
          ;
          ;
          ; FNType - Determine filename type (old file, new file, or
          ;          directory)
          ;
          ;
          ; On entry:
          ;         DS:SI = address of ASCIIZ filename string
          ;
          ; On exit:
          ;         Carry set if file not found or error, clear if found
          ;         AL = file attribute if no error
          ;         AX = DOS error code if error occurred, -1 if not
          ;              DOS error
          ;         CX = filename length if no error
          ;         AH, BX, DX, SI destroyed
          ;         All other registers and interrupt state unchanged
          ;
          ; Notes:
          ;         An error is generated if the name refers to a file but
          ;         contains a trailing backslash.  If the name refers to a
          ;         directory but does not have a trailing backslash, one is
          ;         added.
          ;         
          ;
          entry     FNType,varframe     ;set up entry point
          ;
          var       FindBuf,<size DOSFIND>  ;DOS find first buffer
          varend                        ;end of local variables
          ;
          pushm     di,es               ;save registers
          cld                           ;go forward
          call      AZLen               ;get string length
          jc        FTIntErr            ;no terminator, give up
          mov       di,si               ;get start
          add       di,cx               ;get address of terminator + 1
          sub       di,2                ;point to last real character
          mov       bl,[di]             ;get it
          cmp       bl,'\'              ;is it a backslash?
          je        GotBS               ;if so go handle it
          cmp       bl,'/'              ;try forward too
          jne       DoFind              ;if not go on
          mov       bl,'\'              ;convert forward slash to backslash
          ;
GotBS:    cmp       cx,4                ;check if it's short enough for "d:\"
          jg        KillBS              ;if not go kill backslash
          cmp       bptr -1[di],':'     ;short enough -- colon there?
          je        GotRoot             ;if so skip tests, return root
          ;
KillBS:   mov       bptr [di],0         ;overwrite trailing '\' or '/'
          ;
          ; (NOTE:  We are assuming DS = SS here so we can set the DTA to
          ; point to a stack variable!)
          ;
DoFind:   pushm     bx,cx               ;save last character and count
          lea       dx,FindBuf          ;point to find first buffer
          calldos   SETDTA              ;set the DTA
          mov       dx,si               ;point to name buffer
          mov       cx,ATR_ALL          ;look for anything
;;LFN
          calldos   FFIRST              ;look for the directory (CAUTION:
                                        ;  don't change flags before jc
                                        ;  below!)
          popm      cx,bx               ;get back last character and count
          mov       [di],bl             ;restore last char to caller's buffer
          jc        FTError             ;if error give up
          test      FindBuf.DF_ATTR,ATR_DIR   ;is it a directory?
          jz        NotDir              ;if not go on
          cmp       bl,'\'              ;trailing backslash?
          je        TypeAtr             ;if not go on
          inc       di                  ;point to null
          mov       ax,'\'              ;get '\' and null
          stosw                         ;store it
          inc       cx                  ;increment length for new '\'
          jmp       short TypeAtr       ;and exit
          ;
NotDir:   cmp       bl,'\'              ;not a directory -- any trailing '\'?
          je        FTIntErr            ;if so complain
          ;
TypeAtr:  mov       al,FindBuf.DF_ATTR  ;get attribute byte
          jmp       short TypeOK        ;and go on
          ;
GotRoot:  mov       al,ATR_DIR          ;this is the root, set directory bit
          ;
TypeOK:   clc                           ;show no error
          jmp       short TypeDone      ;get outta here
          ;
FTIntErr: mov       ax,0FFFFh           ;show non-DOS error
          ;
FTError:  stc                           ;show error
          ;
TypeDone: popm      es,di               ;restore registers
          exit                          ;all done
          ;
          ;
          ; AZLen - Get length of an ASCIIZ string
          ;
          ;
          ; On entry:
          ;         DS:SI = address of ASCIIZ filename string
          ;
          ; On exit:
          ;         Carry set if no terminator found, clear otherwise
          ;         CX = Number of bytes in string, including null
          ;              terminator, if terminator found
          ;         AL destroyed
          ;         All other registers and interrupt state unchanged
          ;
          ; Notes:
          ;         An error is generated if the name refers to a file but
          ;         contains a trailing backslash.  If the name refers to a
          ;         dircectory but does not have a trailing backslash, one is
          ;         added.
          ;         
          ;
          entry     AZLen,noframe       ;set up entry point
          ;
          pushm     di,es               ;save registers
          loadseg   es,ds               ;point ES to string
          mov       di,si               ;and DI
          mov       cx,0FFFFh           ;get maximum length
          cld                           ;go forward
          xor       al,al               ;get terminator
          repne     scasb               ;scan for terminator
          jne       AZLErr              ;holler if it wasn't found
          mov       cx,di               ;get address of terminator + 1
          sub       cx,si               ;get string length
          clc                           ;show no error
          jmp       short AZLDone       ;and go on
          ;
AZLErr:   stc                           ;couldn't find terminator
          ;
AZLDone:  popm      es,di               ;restore registers
          exit                          ;all done
          ;
@curseg   ends                          ;close segment
          ;
          end


