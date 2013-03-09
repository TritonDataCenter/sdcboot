
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
;   InitGraftablBuffer - Aitor  - Initialise the graftabl buffer
;   TestAdapter        - Ilya   - Identify graphics adapter
;   EGAInit            - Aitor  - Initialise the EGA/VGA cards
;   CGAInit            - Aitor  - Inisialise the CGA cargs


; =====================================================================
;
; Initialisation routines for EGA/VGA
;
; =====================================================================
;               .       .       .       .       .       line that rules

                ; ***** Constants: where to locate subfonts on the
                ;       select buffer
bFont8x8        EQU     SelectBuffer
bFont8x14       EQU     SelectBuffer+2048
bFont8x16       EQU     SelectBuffer+2048+3584

tblDCC           DB     NO,MDA,CGA,NO,EGA,EGA,NO,VGA,VGA,NO,MCGA,MCGA,MCGA

; Fn:   InitGraftablBuffer
; In:   None
; Out:  None
; Author:  Aitor Santamar¡a Merino
; Does: Initialises the GRAFTABL buffer to be used by GetGraftablInfo

InitGraftablBuffer:
                mov     WORD [cs:dGraftablBuf],bFont8x8+8*128
                mov     [cs:dGraftablBuf+2],cs
                ret


; Fn:   TestAdapter
; In:   (none)
; Out:  AL Video adapter type
; Note: No VESA check yet.
; Author: Ilya V. Vasilyev

TestAdapter:
                 mov    ax,1a00H        ; Call DCC
                 int    10H
                 cmp    al,1aH
                 jnz    taNoDCC

; Now we know, that have PS/2 Video BIOS.
; Active adapter code is now in BL register

                 cmp    bl,0cH
                 ja     taNoDCC
                 sub    bh,bh
                 mov    al,[tblDCC+bx]
                 cmp    al,bh
                 jnz    taKnowAdapter
taNoDCC:
                 mov    bl,10H          ; Get Configuration Information
                 mov    ah,12H          ; Alternate Select
                 int    10H
                 mov    al,EGA
                 cmp    bl,10H
                 jnz    taKnowAdapter   ; if bh<>0, EGA_MONO

                 int    11H             ; EquipList
                 and    al,30H
                 cmp    al,30H
                 mov    al,MDA
                 jz     taKnowAdapter
                 mov    al,CGA
taKnowAdapter:
                 mov    [bAdapter],al
                 ret



; Fn:   EGAInit
; In:   AX: parameter from table
;           0: "EGA" (use adapter type to determine)
;           1: "EGA 8", "LCD"  (8x8 only)
;           2: "EGA 14" (8x8,814 only)
;           3: "VGA" (8x8,8x14,8x16)
;           NO OTHERS ARE SUPPORTED
;       BX: User parameter specified in the commandline
;           1: 8x8 only
;           2: 8x8,814 only
;           3: 8x8,8x14,8x16
;           NO OTHERS ARE SUPPORTED
; Out:  FCarry: set on error (not found, or initialisation error)
;         DX:  offset to an error string
; NOTE: bMaxHWcpNumber:  should detect the maximum number of HW CPs, if I
;          knew of a way to get this info from ARABIC/HEBREW
;

EGAInit:
                push    ax              ; minimum font set in the driver
                                        ; parameter

                                        ; Number of subfonts requested
                                        ; by user in commandline
                mov     [cs:wNumSubFonts],bx

                ;****** Test the adapter: exit if CGA

                call    TestAdapter
                mov     [cs:bAdapter], al
                cmp     al,CGA
                ja      ComputeSubfonts

                mov     dx,errAcient	; driver requires EGA or better
                stc
                ret

                ;****** Compute the minimum subfont number (AL=bAdapter)
                ;       If not specified by the driver parameter,
                ;       it is determined from the adapter type

ComputeSubfonts:
                pop     cx              ; recover the minimum font size
                
                test    cx,0ffffh       ; if parameter non-null, it is
                jnz     MinimumComputed ; the minimum subfont number

                mov     cx,1
                cmp     al,EGA          ; remember: AL=adapter
                jb      MinimumComputed
                inc     cx
                cmp     al,VGA
                jb      MinimumComputed
                inc     cx

                ; ***** In case the user forces MORE subfonts
MinimumComputed:
                cmp     cx,[cs:wNumSubFonts]
                jbe     EGAInitSetVectors
                mov     [cs:wNumSubFonts],cx

                ;****** Exit successfuly: update the resident procs and exit

EGAInitSetVectors:
                call    InitGraftablBuffer
                
                mov     WORD [cs:pRefreshHWcp], EGAHSelect
                mov     WORD [cs:pRefreshSWcp], UnknownEGASSelect
                mov     WORD [cs:pReadFont],  ReadCodepageDisplay
                mov     WORD [cs:psCPIHardwareType], EGAName

                mov     ax,3510H        ; Get vector 10
                int     21H
                mov     [dOld10],bx
                mov     [dOld10+2],es

                mov     dx,New10
                mov     ax,2510H        ; Set vector 10
                int     21H

                ;****** Compute the size of the buffer, based on the
                ;       number of subfonts

                mov     cx, [cs:wNumSubFonts]

                cmp     cx,1
                ja      Subfonts2
                mov     ax, 256*8
                jmp     EGAInitEnd
Subfonts2:      cmp     cx,2
                ja      Subfonts3
                mov     ax, 256*(8+14)
                jmp     EGAInitEnd
Subfonts3:      mov     ax, 256*(8+14+16)

                ;       Allocate the buffer memory
                
                mov     [cs:wBufferSize], ax    ; save the buffer size
                call    AllocateBuffers

EGAInitEnd:
                clc
                ret

; Fn:   CGAInit
; In:   AX: parameter from table (IGNORED)
;       BX: Minimum subfont set by user in commandline (IGNORED)
; Out:  AX: Size of the buffer (in bytes)
;       FCarry: set on error (not found, or initialisation error)
;         DX:  offset to an error string
;


CGAInit:
                ;****** Test the adapter: exit if not CGA
                call    TestAdapter
                cmp     al,CGA
                je      CGAOk

                mov     dx,errNoCGA	; driver requires CGA
                stc
                ret

                ;****** CGA was found, set variables and interrupt vectors
CGAOk:
                mov     al,CGA
                mov     [cs:bAdapter],al

                mov     WORD [cs:wNumSubFonts],1

                push    es
                mov     ax,351Fh        ; read hardware vector
                int     21h
                mov     [cs:dOld1F],bx
                mov     [cs:dOld1F+2],es
                pop     es

                call    InitGraftablBuffer
                
                ;****** Update the resident procs and exit

                                        ; CGA procedures
                mov     WORD [cs:pRefreshHWcp], CGAHSelect
                mov     WORD [cs:pRefreshSWcp], CGASSelect

                mov     WORD [cs:pReadFont],  ReadCodepageDisplay
                mov     WORD [cs:psCPIHardwareType], EGAName

                mov     ax,3510H        ; Get vector 10
                int     21H
                mov     [dOld10],bx
                mov     [dOld10+2],es

                mov     dx,New10
                mov     ax,2510H        ; Set vector 10
                int     21H

                ;       Set the buffer size and allocate the buffers
                mov     ax, 256*8
                mov     [cs:wBufferSize], ax    ; save the buffer size
                
                call    AllocateBuffers

                clc
                ret
