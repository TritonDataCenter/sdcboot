
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
;   CGASSelect   - Aitor  - Select a software codepages for CGA cards
;   CGAHSelect   - Aitor  - Select a hardware codepages for CGA cards

; =====================================================================
;
;   CGA  codepage selection routines
;
;   Copyright (C) 9 Aug 2000 Ilya V. Vasilyev aka AtH//UgF@hMoscow
;      ( adapted from Ilya's GRAFTABL )
;   e-mail: hscool@netclub.ru
;      ( minimum touches by Aitor )
;
;               .       .       .       .       .       line that rules
;

; =====================================================================
;
;   Generic hardware-related DISPLAY variables and routines
;

dOld1F          DD      0
dGraftablBuf    DD      0               ; Graftabl table Buffer

; Fn:   GetGraftablInfo
; Does: Recovers the graftabl info to the pointer stored in its buffer
; In:   Nothing
; Out:  Info retrieved, no other info returned
;       AX, DS: destroyed

GetGraftablInfo:
                mov     ax,0b000h       ; first see if GRAFTABL is active
                int     02fh
                cmp     al,0ffh
                jne     NoGraftabl

                push    bx              ; preserve BX!
                mov     ax,0b001h       ; GRAFTABL present, import the
                push    cs              ; table to DS:BX
                pop     ds
                mov     bx,dGraftablBuf
                int     02fh
                pop     bx
                
NoGraftabl:
                ret


; Fn:   CGASSelect
; Does: Set a proper localized font for a given video mode in CGA
; In:   AL Screen mode (-1 if unknown) (UNUSED)
; Mod:  (none)
;

CGASSelect:
                push    dx

                call    GetGraftablInfo ; obtain the graftabl info to 8x8

                push    cs
                pop     ds
                mov     dx,bFont8x8+8*128
                mov     ax,251fh
                int     21H

                pop     dx
                clc
endCGASSelect:
                ret

; Fn:   CGAHSelect
; Does: Restore the original hardware font in CGA
; In:   CX: Number of codepage to be restored (IGNORED)

CGAHSelect:
                push    dx

                lds     dx,[cs:dOld1F]
                mov     ax,251Fh
                int     21H

                pop     dx
                clc
                ret


