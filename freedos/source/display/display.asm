
;   FreeDOS DISPLAY.SYS           v0.13b
;   FreeDOS PRINTER.SYS
;
;   ===================================================================
;
;   FreeDOS driver to add support of codepage management for the
;   default CON driver (for screen) or default PRN driver (for printer)
;
;   Copyright (C) 2002-05 Aitor Santamar¡a_Merino
;   email:  aitorsm-AT-fdos.org
;
;   (contributed code from DISPLAY 0.5b package, and patches by
;    Ilya V. Vasilyev aka AtH//UgF@hMoscow (hscool@netclub.ru)
;    in  VIDEOINT.ASM, SELECTD.ASM and DISPHW.ASM)
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
;   SOURCE CODE STRUCTURE
;   ---------------------
;
;   DISPLAY.ASM    - Main routine (Start), resident and non-resident
;                    variables, driver start
;
;   <RESIDENT>
;
;     MUXINT.ASM   - Multiplexer interrupt (2Fh) for DISPLAY
;     SELECT.ASM   - Generic codepage select routines
;     PREPARE.ASM  - Generic codepage prepare routines
;     GENIOCTL.ASM - Generic IOCTL request manager
;
;
;   <TRANSIENT>
;
;     DISPLIB.ASM  - Assembler utilities: CON I/O, memory allocation, ...
;     CMDLINE.ASM  - Commandline parsing
;
;     HWINITD.ASM  - Hardware Table and transient code for DISPLAY
;     HWINITP.ASM  - Hardware Table and transient code for PRINTER
;
;     STRINGS.ASM  - Text strings (can be translated to other languages)
;
;
;   <DRIVER SPECIFIC>
;
;     Driver specific code (e.g., for EGA/VGA, IBM Proprinter,etc)
;     should be place in separate folders
;
;     Resident code is to be included in the appropriate section below
;     Transient code is to be included in HWINIT?.ASM
;     The hardware table is to be filled in HWINIT?.ASM
;
;     Driver specific code must be licensed in a license compatible
;     with the GNU-LGPL license
;


CPU     8086

;
;===================================================================
; DRIVER IDENTIFICATION
;===================================================================
;
; NOTE: You should uncomment ONLY ONE of the preceding.
;
; DIFFERENCES:
; - DISPLAY uses the font driver signature 1 in CPI files
;   PRINTER uses the font driver signature 2 in CPI files
; - DISPLAY selection routine calls KEYB to request the change of codepage
;   PRINTER selection routine calls PRINT to freeze printing
; - Only DISPLAY hooks int 2Fh, thus only DISPLAY can check wether it
;   was installed
;

%define DISPLAY
;%define PRINTER

;       .       .       .       .       .       .       line that rules
;
;===================================================================
; VERSION CONSTANT
;===================================================================
;
; NOTE: To change it, please also modify the output string at the
;       bottom of the file

Version                 EQU     000DH   ; Version of the driver (0.12)


;===================================================================
; DRIVER ERROR CONSTANTS
;===================================================================
;
; Explained the functions where errors are admissible

ERR_UnknownFunction     EQU     1       ; (all)
ERR_CPNotPrepared       EQU     26      ; Select (SW)
ERR_CPNotSelected       EQU     26      ; Query
ERR_KEYBFailed          EQU     27      ; Select (HW/SW)
ERR_QueryDeviceError    EQU     27      ; Query
ERR_DevCPNotFound       EQU     27      ; In prepare
ERR_SelPrepDeviceError  EQU     29      ; Select (HW/SW), In prepare
                                        ; NEW: also reflects problems with XMS
                                        ;      in select
ERR_FileDamaged         EQU     31      ; In prepare
                                        ; NEW: also reflects problems with XMS
                                        ;      in prepare
ERR_NoPrepareStart      EQU     31      ; End prepare

;===================================================================
; OTHER CONSTANTS
;===================================================================
;

CommandLine_Offset      EQU     080h    ; where in PSP is commandline
MaxCPNumbers            EQU     10      ; maximum amount of CP numbers

;===================================================================
; PROGRAM HEADER
;===================================================================
;
;               .       .       .       .       .       line that rules


                ORG     100H

                jmp     NEAR Start      ; simply go to Install

DevName         DB      "        "      ; Device name

;===================================================================
; RESIDENT ROUTINES
;===================================================================
;
;       .       .       .       .       .       .       line that rules

;               Include the generic SELECT routines
%include        "select.asm"

;               Include the generic PREPARE routines
%include        "prepare.asm"

;               Include the GENERIC IOCTL routines
%include        "genioctl.asm"

;               Multiplexer interrupt (DISPLAY only)
%ifdef          DISPLAY
%include        "muxint.asm"
%endif


;===================================================================
; DEVICE DRIVER SPECIFIC CODE
;===================================================================
;
;       .       .       .       .       .       .       line that rules


;               DISPLAY hardware device drivers
%ifdef          DISPLAY
%include        "EGAr.asm"      ; EGA/VGA
%include        "CGAr.asm"      ; CGA
%endif


;===================================================================
; DISPLAY DATA SEGMENT
;===================================================================
;
;               .       .       .       .       .       line that rules

                ; ***** XMS management
lpXMSdriver     DD      0               ; entry point to current XMS driver
                                        ; (0 if none installed)

                                        ; XMS Move structure
XMSMoveLen      DD      0               ; * length in bytes
XMSMoveSrcH     DW      0               ; * source handle (0 for real pointer)
XMSMoveSrcO     DD      0               ; * source offset (or real pointer)
XMSMoveTrgH     DW      0               ; * target handle (0 for real pointer)
XMSMoveTrgO     DD      0               ; * target offset (or real pointer)

                ; ***** CPI File Constants
sCPIsignature   DB      0ffh,"FONT   "  ; signature of a CPI file


;===================================================================
; DEVICE FRAME (one per device e.g. CON=(EGA,,1) )
;===================================================================
;
;               .       .       .       .       .       line that rules

                ; ***** GENERAL PURPOSE VARIABLES
bActive         DB      0               ; SW codepage selected?
                                        ;   1: SW cp is selected, DISPLAY active
                                        ;   0: HW cp is selected, DISPLAY inactive
wCPselected     DW      -1              ; Current codepage (urrently ACTIVE)
                                        ; (can be hardware or software)
wCPbuffer       DW      0               ; CP in SELECT buffer
                                        ; (can be active or not)

                ; ***** KEEP THIS BLOCK TOGETHER! (do not alter order)
wNumSoftCPs     DW      0               ; number of software codepages
wUserParam      DW      0               ; User defined parameter
wNumHardCPs     DW      1               ; number of hardware codepages
                                        ; CPs in HW/PREPARE buffers
wCPList         times   MaxCPNumbers DW 0
                                        ; END of the fixed block

                ; ***** Driver signature (for the CPI files)
%ifdef          DISPLAY
DriverSignature DW     1
%endif
%ifdef          PRINTER
DriverSignature DW     2
%endif

                ; ***** DRIVER customized FUNCTIONS and VARIABLES
wBufferSize     DW	0		; Size of the buffer (bytes)
pRefreshHWcp    DW      0               ; Hardware codepage select procedure
pRefreshSWcp    DW      0               ; Software codepage select procedure
pReadFont       DW      0               ; Codepage prepare procedure
psCPIHardwareType
                DW      0               ; pointer to hardware type string
                                        ; in CPI files

                ; ***** Codepage Prepare IOCTL buffer
wToPrepareSize  DW      0               ; Number of cps to be prepared
wToPrepareBuf   times   12 DW  0        ; codepages to be prepared


                ; ***** Buffer pointers
fBuffersInXMS   DB      0               ; flags: bit i = buffer i in XMS?
wBufferPtrs     DW      0,0,0,0,0,0,0,0 ; buffer pointers:
                                        ;   if in XMS: handle to EMB
                                        ;   if not in XMS: offset (wrt CS)


;===================================================================
; DYNAMIC MEMORY (HEAP)
;===================================================================
;
;               .       .       .       .       .       line that rules

;               At least make room for 6 buffers
;               (size: the typical of a EGA/VGA DISPLAY buffer
;                with the 3 subfonts)
;               (The select buffer + 5 software buffers)


SelectBuffer    times 9728 DB 0
                times 9728 DB 0
                times 9728 DB 0
                times 9728 DB 0
                times 9728 DB 0
                times 9728 DB 0


;===================================================================
; NON-RESIDENT VARIABLES
;===================================================================
;

;               MEMORY and HEAP management

fIsXMS          DB      0               ; is there an XMS driver installed?
pMemTop:        DW      bLastByte       ; top of memory
pHeapBase:      DW      SelectBuffer    ; base of the heap, starts on 8x8

;               DRIVER Initialisation procedure

pInitProc       DW      0               ; Init procedure
wInitParam      DW      0               ; parameter to be passed

;               DRIVER CONFIGURATION
bMemoryUssage   DB      1               ; Possible values:
                                        ; 0: TPA memory
                                        ; 1: XMS memory
bVerbosity      DB      1               ; Possible values:
                                        ; 0: Quiet (similar to Normal)
                                        ; 1: Normal
                                        ; 2: Verbose


;               COPYRIGHT message

%ifdef           DISPLAY
copyVer          DB     "FreeDOS DISPLAY ver. 0.13b", 0dH, 0aH, "$"
%endif
%ifdef           PRINTER
copyVer          DB     "FreeDOS PRINTER ver. 0.13b", 0dH, 0aH, "$"
%endif
ReturnString     DB     0dH, 0aH, "$"

;===================================================================
; NON-RESIDENT DRIVER ROUTINES AND DRIVER TABLE
;===================================================================
;

%ifdef          DISPLAY
%include        "HWInitD.asm"
%endif

%ifdef          PRINTER
%include        "HWInitP.asm"
%endif



;===================================================================
; MAIN FOR THE .COM VERSION
;===================================================================
;
;               .       .       .       .       .       line that rules


Start:
                ;****** Show Copyright and Version

                mov     dx,copyVer
                call    OutStrDX

                ;****** DISPLAY is incompatible with some versions of DR-KEYB
%ifdef DISPLAY
                mov     ax,0ad80h
                xor     bx,bx
                mov     cx,bx
                int     02fh
                cmp     ah,0ffh         ; KEYB installed?
                jne     EndNoDRKEYB
                test    bx,0ffffh       ; MS-KEYB?
                jnz     EndNoDRKEYB
                test    cx,0ffffh       ; FD-KEYB?
                jz      EndNoDRKEYB
                cmp     ch,7
                jae     EndNoDRKEYB     ; check DR-KEYB version
                mov     dx,errNoDRDOS
                call    OutStrDX
                jmp     Done
EndNoDRKEYB:
%endif
                
                ;****** Remove segmentation

                push    cs
                pop     ds
                push    cs
                pop     es
                cld

                ;****** Parse the commandline
;                mov     cx, [CommandLine_Offset]
                mov     si, CommandLine_Offset+1

                call    ParseCommandLine
                jc      Done

                ;****** find if there are XMS drivers

                cmp     BYTE [cs:bMemoryUssage],1
                jne     doneXMS

                mov     ax,4300h
                int     2Fh

                cmp     al,80h
                jne     doneXMS

                mov     BYTE [cs:fIsXMS],1

                mov     ax, 4310h       ; get the driver entry point
                int     2Fh

                mov     [cs:lpXMSdriver],bx
                mov     [cs:lpXMSdriver+2],es
doneXMS:

                ;****** Install the driver
                call    Install
                jc      Done

                ;****** RESIDENT EXIT
                
                mov     ax, [cs:02ch]   ; Free environment space
                mov     es, ax
                mov     ah, 049h
                int     21h

                mov     dx,[cs:pHeapBase]
                int     27H             ; TSR  for .COM files

                ;****** NON-RESIDENT EXIT
Done:
                int     20H             ; Exit for .COM files
                
;===================================================================
; DRIVER RESIDENT INSTALL
;===================================================================
;
;               .       .       .       .       .       line that rules

Install:
                cli

                ;****** Run driver specific INIT
                mov     ax, [cs:wInitParam]
                mov     bx, [cs:wUserParam]
                mov     di, [cs:pInitProc]
                call    [cs:di]
                jnc     HWInitOk ; print error string and exit

                mov     dx,errDrvSpecific
                jmp     OutStrDX
HWInitOk:
                
                ;****** set multiplexer interrupt vector (DISPLAY ONLY)

%ifdef          DISPLAY

                mov     ax,352fH        ; Get vector 2f
                int     21H
                mov     [dOld2f],bx
                mov     [dOld2f+2],es

                mov     dx,New2f
                mov     ax,252fH        ; Set vector 2f
                int     21H

%endif

                clc
                ret


;===================================================================
; FINAL INCLUDES
;
;               .       .       .       .       .       line that rules

                ; ***** Commandline parsing
%include        "cmdline.asm"

                ; ***** Transient utilities
%include        "displib.asm"

                ; ***** Localised strings
%include        "strings.en"

bLastByte:
                END

                
