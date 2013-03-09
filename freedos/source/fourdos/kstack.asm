

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


          name      KSTACK
          title     'KSTACK -- TSR for 4DOS 7.50 KEYSTACK function'

; KSTACK.ASM - copyright (c) 1991-2003  Rex C. Conn
;  TSR to support the KEYSTACK function in 4DOS

          include   product.asm
          include   trmac.asm           ; general macros

code      segment   byte public 'text'
          assume cs:code, ds:nothing, es:nothing

          ifndef    SILENT
KSCODE    equ       0D44Fh
KSRCODE   equ       044DDh
          else
KSCODE    equ       0D444h
KSRCODE   equ       044DDh
          endif

org       0100h

EntryPoint:
          jmp       Initialize

oldint16  dd        ?                   ; pointers to previous interrupt handlers
oldint2F  dd        ?

k_buffer_end        dw        ?         ; end of KSTACK keyboard buffer
bufptr    dw        ?                   ; pointer to current position in buffer

delaycnt  dw        0                   ; tick counter
tickstart dw        0                   ; starting tick counter value
nchleft   dw        0                   ; number of characters left in buffer
savetype  db        0                   ; saved caller's AH


newint16  proc      far                 ; new BIOS Int 16h (Keyboard)
          ;
          sti
          cmp       cs:nchleft,0
          je        do_prev             ; if no data chain to prev routine
          push      ax
          and       ah,0EEh             ; check bits we don't want
          pop       ax
          jz        Ours                ; if no bits set it's for us
          ;
do_prev:  jmp       cs:[oldint16]       ; if not chain to previous routine
          ;
Ours:     pushm     si,ds               ; set things up
          loadseg   ds,cs
          cld
          mov       savetype,ah         ; save type of call
          mov       si,bufptr           ; get current buffer position
          cmp       delaycnt,0
          je        GetChar             ; if no delay go on
          test      ah,1
          jz        ReadDel             ; handle delay on read
          call      DelayChk            ; delay expired?
          jnb       GetChar             ; if delay done go get next character
          xor       ax,ax               ; return 0 for status call
          jmp       short StatChar
          ;
GetChar:  lodsw                         ; get character from buffer
          or        ax,ax
          je        ZeroChar            ; handle zero (buffer empty)
          cmp       ax,0FFFFh
          je        DelStart            ; handle delay startup
          test      savetype,1
          jnz       StatChar            ; have a character, return for status
          dec       nchleft             ; count character
          mov       bufptr,si           ; move to next character next time
          popm      ds,si
          iret                          ; return to calling program
          ;
ZeroChar: test      savetype,1
          jnz       ZeroRet             ; if status return zero
          dec       nchleft             ; count character
          jmp       short ReadNext      ; and try again
          ;
DelStart: dec       nchleft             ; count delay marker
          lodsw                         ; get delay length
          mov       delaycnt,ax         ; store delay count
          call      GetTicks
          mov       tickstart,ax        ; save starting BIOS tick counter
          test      savetype,1
          jz        RDStart             ; handle delay on read
          xor       ax,ax               ; return 0 for status call
          ;
ZeroRet:  dec       nchleft             ; count character (0 or delay cnt)
          mov       bufptr,si           ; move to next character next time
          ;
StatChar: or        ax,ax               ; set status flags
          popm      ds,si
          ret       2                   ; do not pop flags
          ;
RDStart:  dec       nchleft             ; count delay count
          ;
ReadDel:  call      DelayChk
          jb        ReadDel             ; loop until delay is done
          ;
ReadNext: cmp       nchleft,0           ; read next character (if any)
          jne       GetChar
          ;
          mov       bufptr,si           ; restore buffer pointer
          popm      ds,si               ; nothing to do, get out
          jmp       do_prev
          ;
newint16  endp


GetTicks  proc      near                ;gets current BIOS tick counter
          push      es
          mov       ax, 040h            ; point ES to ROM BIOS data area
          mov       es, ax
          mov       ax, es:[06Ch]       ; get original tick count
          pop       es
          ret
GetTicks  endp


DelayChk  proc      near                ; check if delay is done
          ;
          call      GetTicks            ; get BIOS tick count
          sub       ax,tickstart        ; get time elapsed
          cmp       ax,delaycnt         ; check against desired delay
          jb        DelAct              ; if still active go on
          mov       delaycnt,0          ; show not active
          ;
DelAct:   ret
          ;
DelayChk  endp


; -------------------------------------------------------------------
; INT 2F multiplex
;    AX = D44Fh
;    BX = 0   Check for installed state
;    BX = 1   Load the string in DS:DX
;--------------------------------------------------------------------
new2f     proc      far

          cmp       ax,KSCODE           ; check KEYSTACK function request
          jne       jmpold2f

          mov       ax,KSRCODE
          or        bx,bx                         ; check for installation?
          jz        new_2f_bye

          sti
          push      di
          push      si

          cld
          push      cs
          pop       es
          lea       di,k_buffer                   ; initialize destination pointer
          mov       cs:bufptr,di
          mov       cs:nchleft,cx
          mov       cs:delaycnt,0
          mov       si,dx
	cmp	cx,01FFh			; limit to 511
	jbe	cnt_ok
	mov	cx,01FFh
cnt_ok:
          rep       movsw                         ; save string (DS:SI) to keystack buffer

          pop       si
          pop       di

          xor       ax,ax
new_2f_bye:
          iret

jmpold2f:
          jmp       cs:oldint2F                   ; jump to previous INT 2F routine

new2f     endp


k_buffer label   byte            ; 1024 byte (512 scan codes) keystroke buffer


; Initialize the KEYSTACK support
Initialize          proc      near

          mov       si,PSP_TLEN
          lodsb                                   ; get length byte
          cbw                                     ; make it a work
          mov       bx,ax
          mov       bptr[si][bx],0                ; null-terminate tail

ParseTail:
          lodsb
          or        al,al                         ; end of tail?
          je        TailDone
          cmp       al,'/'                        ; switch char?
          jne       ParseTail
          lodsb
          cmp       al,'I'                        ; I(nstall) again?
          je        not_installed
          cmp       al,'i'
          je        not_installed
          jmp       short ParseTail

TailDone:
; check if we're already installed
          mov       ax,KSCODE
          xor       bx,bx
          int       02Fh
          cmp       ax,KSRCODE
          jne       not_installed

          ifndef    SILENT
          mov       ah,9
          lea       dx,ID_NO            ; display "already installed" message
          int       21h
          endif

          mov       ax,04C01h           ; terminate with errorlevel = 1
          int       21h

not_installed:

          mov       ax,cs:[PSP_ENV]     ; point to our environment block
          or        ax,ax
          jz        no_env
          mov       es,ax
          calldos   FREE                ; free it

no_env:
; Load INT 16h (BIOS keyboard) trapping
          mov       ax,03516h
          int       21h                 ; get old INT 16 address
          mov       word ptr [oldint16],bx
          mov       word ptr [oldint16+2],es

          mov       ax,02516h
          lea       dx,newint16
          int       21h                 ; set new INT 16 address

; Load INT 2Fh (Multiplex) trapping
          mov       ax,0352Fh
          int       21h                 ; get old INT 2F address
          mov       word ptr [oldint2F],bx
          mov       word ptr [oldint2F+2],es

          mov       ax,0252Fh
          lea       dx,new2F
          int       21h                 ; set new INT 2F address

          ifndef    SILENT
          mov       ah,9
          lea       dx,ID                         ; display sign-on message
          int       21h
          endif

; point to break address (free memory after driver loaded)
          lea       dx,cs:k_buffer                ; set end of keyboard buffer
          add       dx,1024                       ; max characters anybody could enter
          mov       cs:k_buffer_end,dx

          mov       cx,4
          shr       dx,cl                         ; adjust number of paragraphs to keep
          inc       dx

          mov       ax,03100h           ; terminate & stay resident
          int       21h

Initialize  endp

          ifdef     ENGLISH
          ifndef    SILENT
ID        db        '4DOS 7.50 KSTACK loaded',13,10
          db        'Copyright 1991-2003 Rex Conn & JP Software Inc.  All Rights Reserved.',13,10,'$'
ID_NO     db        'KSTACK already loaded',13,10,'$'
          else                          ;define embedded copyright if silent
          db        'Copyright 1991-2003, Rex Conn & JP Software Inc., All Rights Reserved.'
          endif
          endif

          ifdef     GERMAN
ID        db        '4DOS 7.50 KSTACK geladen',13,10
          db        'Copyright 1988-2003  JP Software Inc.',13,10
          db        'Alle Rechte vorbehalten.  Vertrieb unter Lizenz von JP Software.',13,10,'$'

ID_NO     db        'KSTACK bereits geladen',13,10,'$'
          endif

code      ends

          end       EntryPoint

