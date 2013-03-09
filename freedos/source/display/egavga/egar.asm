
;   FreeDOS CGA/EGA/VGA Driver    v0.13
;
;   ===================================================================
;
;   Codepage management (Generic IOCTL)
;   for CGA, EGA, VGA graphic adapters
;
;   Copyright (C) 2002-05 Aitor Santamar¡a_Merino
;   email:  aitorsm-AT-fdos.org
;
;   Ilya V. Vasilyev aka AtH//UgF@hMoscow
;   email:  hscool-AT-netclub.ru
;
;   WWW:    http://www.freedos.org/
;
;   ===================================================================
;
;   This program is free software; you can redistribute it and/or modify
;   it under the terms of the GNU General Public License as published by
;   the Free Software Foundation; either version 2 of the License, or
;   (at your option) any later version.
;
;   This program is distributed in the hope that it will be useful,
;   but WITHOUT ANY WARRANTY; without even the implied warranty of
;   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
;   GNU General Public License for more details.
;
;   You should have received a copy of the GNU General Public License
;   along with this program; if not, write to the Free Software
;   Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
;
;   ===================================================================
;
;   ROUTINES:
;
;   SModeTest    - Ilya   - Identify the current screen mode
;   EGASSelect   - Ilya   - Select a software codepages for CGA cards
;   Old10Call    - Ilya   - Call previous int 10h handler
;   EGAHSelect   - Ilya   - Select a hardware codepages for CGA cards
;   ReadSubFont  - Aitor  - Utility to read a subfont from a CPI file
;   ReadCodepageDisplay
;                - Aitor  - Utility to prepare a EGA-type codepage

;               .       .       .       .       .       line that rules
;


%include        "videoint.asm"


EGAName         DB      "EGA     "
wNumSubFonts    DW      0               ; number of subfonts (m)

wMinFontSize:   DW      0               ; minimum font size
                                        ; 1=8x8  2=8x8,14  3=8x8,14,16


; Fn:   SModeTest
; Does: Determines wether text mode or graphic mode is enabled
; In:   nothing
; Out:  Carry set if graphic mode is set
;
;               (EGA/VGA only!)


SModeTest:
                push    ax
                mov     al,6
                push    dx
                mov     dx,3ceH
                out     dx,al
                call    sfr             ; Small delay
                inc     dx
                in      al,dx           ; Graph Controller Misc Register
                pop     dx
                shr     al,1
                pop     ax
sfr:            ret




; Fn:   EGASSelect
; Does: Set a proper localized font for a given video mode
; In:   AL Screen mode (-1 if unknown)
;       (UnknownEGASSelect is the entry point for DISPLAY's SELECT)
; Mod:  (none)
;
; Summary: self-determine rows per character, then call the
;          load-character-table function, which is int10h:
;               AX=1100h (for text mode),
;               AX=1121h (for graphics mode)
;
; NOTES:
; - AX, CX, DS, ES, DI  will be used and not preserved
;

UnknownEGASSelect:
                mov     al,-1           ; unknown vide mode!

EGASSelect:
                push    cs              ; we require ES==CS
                pop     es

                push    dx
                push    bx
                push    si
                push    bp

                ;************ initialise DX to number of lines (rows)
		;             in the screen

                xor     bx,bx                   ; BL: Zero!
                mov     ds,bx
                mov     dx,[484H]
                mov     bh,dh                   ; BH: bytes per character
                inc     dx                      ; DL: number of lines

                ;************ restore int1Fh vector with pointer to 8x8 table
                ;             (this is useful just in the 8x8 mode)

                mov     WORD [7cH],bFont8x8+1024; Kinda tradition to restore
                mov     [7eH],cs                ; vector 1fH every time

                ;************ determine the table to be installed (the number
		;             of rows)

                cmp     BYTE [cs:bAdapter],VGA  ; if VGA (or better) or BH>=16,
                jb      sfNo8x16		; then 8x16
                cmp     bh,16
                jb      sfNo8x16
                mov     bh,16                   ; force bh=16 (in case it's >16)
                jmp     SHORT sfSetIt
sfNo8x16:
                cmp     bh,14                   ; check bh>=14
                jb      sfNo8x14
                mov     bh,14                   ; force bh=14 (in case it's >14)
                jmp     SHORT sfSetIt
sfNo8x14:
                cmp     bh,8                    ; check bh>=8  If below 8,
                jb      sfpr                    ; then unsupported ->RETURN
                mov     bh,8                    ; force bh=8 (in case it's >8)

                ;************ We have checked the number of rows, then set it
                ;             First, we obtain the data from their sources
sfSetIt:
                mov     bp, bFont8x8

                cmp     bh, 8
                je      sfToG

                add     bp, 2048
                cmp     bh, 14
                je      sfToG

                add     bp, 3584

                ;             Second, we determine wether text or graphics
sfToG:
                cmp     al,13H
                ja      sfTest          ; Screen modes above 13H are unknown
                cmp     al,0dH
                jae     sfGraph         ; Screen modes 0d..13H   are graphics
                cmp     al,4
                jb      sfText          ; Screen modes  0..3     are text
                cmp     al,7
                jz      sfText          ; Screen mode    7       is  text
                jb      sfGraph         ; Screen modes  4..6     are graphics

                ;************ unknown, test if text/graphics mode
sfTest:
                call    SModeTest
                jc      sfGraph

                ;************ set the font for text mode: use int10h / ax=1100h
sfText:
                mov     cx,256          ; Load 256 characters
                mov     ax,1100H        ; Load user font, reprogram controller
                cwd                     ; DX=0, BL is already zero, BH=bpc
                jmp     SHORT sfi10

                ;************ set the font for graphics mode:
		;             use int10h / ax=1121h
sfGraph:
                mov     cl,bh           ; bytes per character
                sub     ch,ch           ; BL is already zero, DL=# of lines
                mov     ax,1121H

                ;************ call the appropriate interrupt function
sfi10:
                pushf
                call    FAR [cs:dOld10]

                ;************ end of interrupt
sfpr:
                pop     bp
                pop     si
                pop     bx
                pop     dx
                clc                     ; just in case, always SUCCEED
endEGASSelect:
                ret


; Fn:   old10call
; Does: clears BX and calls the old driver
; In:   AX = some function of the old 10handler
; Out:  results of the call

old10call:      xor     bx, bx
                pushf
                call    FAR [cs:dOld10]
                ret



; Fn:   EGAHSelect
; Does: Sets the hardware codepage, helped by the old int 10h handler
; In:   CL: index of hardware font page to select (0, 1,...)
; Out:  -

EGAHSelect:

                push    ax
                push    bx

                ;*********************** try Arabic/Hebrew first

                inc     cl
                mov     ax,0ad42h
                int     02fh

                ;*********************** now call BIOS to refresh

                mov     cx,[cs:wNumSubFonts]

                call    SModeTest
                jc      RestoreGraph

                mov     ax, 1101h       ; text mode calls
                call    old10call
                cmp     cl,2
                jb      RefreshHWcpEnd
                
                mov     ax, 1102h
                call    old10call
                cmp     cl,3
                jb      RefreshHWcpEnd

                mov     ax, 1104h
                call    old10call
                jmp     RefreshHWcpEnd

RestoreGraph:                           ; graphics mode calls
                mov     ax, 1122h
                call    old10call
                cmp     cl,2
                jb      RefreshHWcpEnd

                mov     ax, 1123h
                call    old10call
                cmp     cl,3
                jb      RefreshHWcpEnd

                mov     ax, 1124h
                call    old10call

RefreshHWcpEnd:
                clc
                pop     bx
                pop     ax

                ret

; Fn:   ReadSubfont
; Does: Reads a subfont from the appropriate section of the CPI file
; In:   DS:SI->  position in the CPI file where the subfont starts
;       DL:      number of buffer to be transferred (0,1,...)
; Out:  CF set on error, clear otherwise
;       If success: SI point to the byte immediately after the whole block
;       AX,CX: trashed
;
; FORMAT OF THE SUBFONT  (offsets respect to DI)
; ---------------------
; 0  DB  Size:     values 16, 14 or 8
; 1  DB  witdth:   MUST be 8
; 2  DW  (ignored)
; 4  DW  n.chars:  MUST be 255
; 6  <data>
;

ReadSubfont:
                ;**************  check DB 8 annd DW 255
                mov     al,[ds:si+1]
                cmp     al,8
                jne     ReadSubfontError
                mov     ax,[si+4]
                cmp     ax,256
                jne     ReadSubfontError

                ;************** check the number of subfonts
                mov     al,[si]         ; get the subfont size to ax
                mov     cx,[cs:wNumSubFonts]

                cmp     al,8
                je      ComputeSizeToMove
                cmp     cx,2
                jb      ReadSubfontError

                cmp     al,14
                je      ComputeSizeToMove
                cmp     cx,3
                jb      ReadSubfontError

                cmp     al,16
                je      ComputeSizeToMove

                ;       When size is unsupported, skip it, but don't
                ;       return an error

                mov     ah,al
                xor     al,al
                add     ax,6
                add     si,ax
                clc
                ret

                
                ;************** add the appropriate offset

ComputeSizeToMove:
                add     si,6            ; now points to the data

                xor     di,di           ; relative offset=0
                mov     cx,2048         ; initialize to 2048 bytes (8x256)
                
                cmp     al,8
                je      ReadSubfontMoveData

                add     di,cx           ; destination now skips 8x8
                mov     cx,3584         ; set to 3584 bytes (14x256)
                cmp     al,14
                je      ReadSubfontMoveData

                cmp     al,16
                jne     ReadSubfontError
                add     di,cx           ; destination now skips 8x14
                mov     cx,4096         ; set to 4096 bytes (16x256)

                ;************** Move the data
                ; CX: number of bytes to move
                ; DI: relative offset in the file
                
ReadSubfontMoveData:
                call    MoveDataToBuffer
                ret                     ; returns with carry as appropriate

ReadSubfontError:

                stc
                ret

; Fn:   ReadCodepageDisplay
; Does: Reads a codepage of DISPLAY type
; In:   DS:SI -> position in the CPI file where the font header starts
;       DX:      number of the buffer where to store the font (0,1,...)
; Out:  CF=1     failure (PREPARE must abort)
;       CF=0     success
;                DS:SI-> byte immediately after the font
;
; FONT HEADER
; ===========
;  0  DW   *  Signature (1) (FONT)
;  2  DW   *  Number of subfonts (usually 3)
;  4  DW   *  Size of the whole subfonts block (usually 9746 bytes)
;
; (*)  to be tested


ReadCodepageDisplay:
                mov     ax,[si]      ; it is a FONT
                cmp     ax,1
                jne     ReadCodepageDisplayError

                mov     cx,[si+2]

;                mov     ax,[si+2]      ; we force it to have 3 subfonts
;                cmp     ax,3
;                jne     ReadCodepageDisplayError

;                mov     ax,[si+4]      ; we force it to be 9746
;                cmp     ax,9746
;                jne     ReadCodepageDisplayError

                ;**************  read the data
                add     si,6

ReadSubs:

                push    cx
                call    ReadSubfont
                pop     cx
                jc      ReadCodepageDisplayError
                loop    ReadSubs

;                call    ReadSubfont
;                jc      ReadCodepageDisplayEnd
;
;                call    ReadSubfont
;                jc      ReadCodepageDisplayEnd
;
;                call    ReadSubfont

ReadCodepageDisplayEnd:
                ret
ReadCodepageDisplayError:
                stc
                ret


