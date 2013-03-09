

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


          title     PARSPATH - Parse a fully qualified file name
          ;
          page      ,132                ;make wide listing
          ;
          comment   }

          Copyright 1989, 1990, J.P. Software, All Rights Reserved

          Author:  Tom Rawson  1/29/89


          This routine parses a fully qualified file name into drive, 
          directory, filename, and extension.

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
          ;         DS:SI = pointer to file name string, terminated by null,
          ;                 blank, cr, or lf
          ;
          ; On exit:
          ;         AH = drive (0 = A, 1 = B, ...)
          ;         AL = 0 if drive missing, non-zero if present
          ;         BH = path position in string
          ;         BL = path length (0 if none)
          ;         CH = filename position in string
          ;         CL = filename length (0 if none)
          ;         DH = extension position in string (0 if none)
          ;         DL = extension length (0 if none or empty)
          ;         DS:SI = pointer to terminator
          ;         All other registers and interrupt state unchanged
          ;         Direction flag clear
          ;
          ;
          entry     ParsPath            ;set up entry
          ;
          ; Local variables
          ;
          varW      DriveDat            ;drive number and flag
          varW      PathPtr             ;path location and length
          varend
          ;
          push      di                  ;save di
          xor       ax,ax               ;get zero
          mov       DriveDat,ax         ;clear drive pointer
          mov       PathPtr,ax          ;clear path pointer
          mov       di,si               ;save original string address
          cld                           ;go forward
          xor       bx,bx               ;start out looking for drive
          xor       cx,cx               ;show period not found
          ;
ScanLoop: lodsb                         ;get a byte
          or        al,al               ;null?
          je        Done                ;if so we're done
          cmp       al,' '              ;how about blank?
          je        Done                ;that also signals end
          cmp       al,CR               ;or cr?
          je        Done
          cmp       al,LF               ;or lf?
          je        Done
          jmp       word ptr cs:PathTab[bx]  ;branch based on state
          ;
PathTab   label     word
          dw        ChkDrive            ;state 0 - check drive
          dw        StrtPath            ;state 1 - check path start
          dw        ChkPath             ;state 2 - scan for path
          dw        EndPath             ;state 3 - scan for end of path
          ;
          ; State 0 (BX = 0):  Look for "x:" at start
          ;
ChkDrive: add       bx,2                ;always go to check path (state 1)
          cmp       bptr ds:[si],":"    ;is there a colon?
          jne       StrtPath            ;if not, look for path
          mov       ah,al               ;get drive letter
          sub       ah,"A"              ;convert to numeric
          cmp       ah,25               ;over "Z"?
          jb        SaveDrv             ;if not go on
          sub       ah, ("a" - "A")     ;convert assuming lower case
          ;
SaveDrv:  mov       DriveDat,ax         ;save drive info
          inc       si                  ;skip colon
          jmp       short ScanLoop      ;and go back for more
          ;
          ; State 1 (BX = 2):  Save the start of the path or filename.  
          ; If it starts with a period or backslash, it must be the path.
          ;
StrtPath: add       bx,2                ;always go to scan path (state 2)
          mov       dx,si               ;save start of current portion
          dec       dx                  ;back up because si was incremented
          cmp       al,"."              ;period at start?
          je        GotPath             ;if so we've found the path
          cmp       al,"\"              ;backslash at start?
          je        GotPath             ;if so we've found the path
          jmp       short ScanLoop      ;and go back for more
          ;
          ; State 2 (BX = 4):  Scan to see if we are in the path
          ;
ChkPath:  cmp       al,"."              ;found a period?
          jne       ChkSlshP            ;if not go on
          mov       cx,si               ;save last period address
          dec       cx                  ;back up because si was incremented
          jmp       short ScanLoop      ;continue scan
          ;
ChkSlshP: cmp       al,"\"              ;backslash found?
          jne       ScanLoop            ;if not keep looking
          ;
GotPath:  mov       PathPtr,dx          ;save it
          mov       dx,si               ;save last backslash address (also
                                        ;  saves location of leading "." but
                                        ;  this should be OK as \ must follow
                                        ;  soon)
          dec       dx                  ;back up because si was incremented
          add       bx,2                ;now look for end of path (state 3)
          jmp       short ScanLoop      ;and go back for more
          ;
          ; State 3 (BX = 6):  Look for final backslash
          ;
EndPath:  cmp       al,"."              ;found a period?
          jne       ChkSlshE            ;if not go on
          mov       cx,si               ;save last period address
          dec       cx                  ;back up because si was incremented
          jmp       short ScanLoop      ;continue scan
          ;
ChkSlshE: cmp       al,"\"              ;look for a backslash
          jne       ScanLoop            ;if not found keep going
          mov       dx,si               ;save last backslash address
          dec       dx                  ;back up because si was incremented
          jmp       short ScanLoop      ;and keep going
          ;
          ; Found end of string -- branch based on previous state
          ;
Done:     dec       si                  ;back up to point to terminator
          jmp       word ptr cs:DoneTab[bx]  ;branch depending on state
          ;
DoneTab   label     word
          dw        NoFile              ;state 0 - string was empty
          dw        NoFile              ;state 1 - drive only
          dw        GotFile             ;state 2 - filename but no path
          dw        FullSpec            ;state 3 - full file spec
          ;
          ; Terminated in state 3 -- full file spec found
          ;
FullSpec: mov       bx,PathPtr          ;get path start address in bx
          mov       ax,bx               ;save it
          sub       bx,di               ;get path position
          mov       bh,bl               ;save in bh
          sub       ax,dx               ;subtract backslash address to get
                                        ;  negative length + 1
          neg       ax                  ;make length positive
          inc       ax                  ;bump for full length
          mov       bl,al               ;copy to bl
          mov       PathPtr,bx          ;save pointer with length
          inc       dx                  ;bump pointer to start of filename
          ;
          ; Terminated in state 2 -- no path was found -- or drop through
          ; from above.
          ;
GotFile:  mov       ax,dx               ;copy file name address
          sub       ax,di               ;get file name position
          mov       ah,al               ;copy to ah
          or        cx,cx               ;did we find a period?
          jz        FileOnly            ;if not there's no extension
          mov       bx,cx               ;copy period address
          sub       bx,dx               ;get file name length
          mov       al,bl               ;now ax = file name info
          mov       bx,cx               ;copy period address
          inc       bx                  ;point to start of extension
          mov       dx,si               ;copy scan end address
          sub       dx,bx               ;calculate extension length
          sub       bx,di               ;calculate extension position
          mov       dh,bl               ;copy to dh
          mov       cx,ax               ;put file info in cx
          jmp       short Return        ;return everything
          ;
FileOnly: mov       cx,si               ;copy scan end address
          sub       cx,dx               ;get file name length 
          mov       ch,ah               ;now cx = file name info
          jmp       short NoExt         ;go handle no extension
          ;
NoFile:   xor       cx,cx               ;clear file pointer
          ;
NoExt:    xor       dx,dx               ;clear extension pointer
          ;
Return:   mov       ax,DriveDat         ;drive info in AX
          mov       bx,PathPtr          ;path info in BX
          ;
          pop       di                  ;restore registers
          exit                          ;all done
          ;
@curseg   ends                          ;close segment
          ;
          end






