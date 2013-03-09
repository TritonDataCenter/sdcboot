;    
;   Gtcdex.asm - routines to tell wich drives relate to CD-ROM's.
;   Copyright (C) 2000 Imre Leber
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
;   If you have any questions, comments, suggestions, or fixes please
;   email me at:  imre.leber@worldonline.be
;

;----------------------------- DATA SEGMENT --------------------------------

segment GTCDEX_DATA
        
        installed db -1
        beenhere  db 0

%if 0
        drives times 27 db 0     ;; C-string for all the cdrom drive letters.
%endif

;----------------------------- CODE SEGMENT --------------------------------

segment GTCDEX_TEXT 

;===========================================================================
;===                          CDEXinstalled                              ===
;===========================================================================
;=== int CDEXinstalled(void);                                            ===
;===                                                                     ===
;=== See wether a MSCDEX compatible driver has been installed.           ===
;===                                                                     ===
;=== Remark: the CDEX version has to be at least 2.0.                    ===
;===========================================================================

        global _CDEXinstalled
_CDEXinstalled:
        push ds  
        
        mov ax,GTCDEX_DATA
        mov ds,ax
        
        cmp  [installed], byte -1      ;; Exit if we allready initialised.
        jne  .EndOfProc

        xor  bx, bx
        mov  ax, 1500h                 ;; See if MSCDEX installed.
        int  2fh

        cmp  bx, 0                     
        je   .NotInstalled

        mov  ax, 150Ch
        int  2fh

        cmp  bh, 02h                   ;; See if at least version 2.0
        jb   .NotInstalled

        mov  [installed], byte 1       ;; Remember that ??CDEX 2.0+ is 
        jmp  short .EndOfProc          ;; installed 

.NotInstalled:
        mov  [installed], byte 0       ;; Remember that ??CDEX 2.0+ is 
                                       ;; not installed.
.EndOfProc:
        xor  ah, ah
        mov  al, [installed]           ;; Return wether ??CDEX 2.0+ is
        cmp  ax, 0                     ;; installed.
        
        pop  ds
        retf                            

%if 0

;===========================================================================
;===                          GetCDROMLetters                            ===
;===========================================================================
;=== char* GetCDROMLetters(void);                                        ===
;===                                                                     ===
;=== Get the letters of the drives that correspond to CDROM's.           ===
;===                                                                     ===
;=== Remark: the CDEX version has to be at least 2.0.                    ===
;===                                                                     ===
;=== Not yet made to run in large model.                                 ===
;===========================================================================

        global _GetCDROMLetters
_GetCDROMLetters:
        
        cmp  [beenhere], byte 1     ;; Just return the value if we allready
        je   .EndOfProc             ;; know it.
        mov  [beenhere], byte 1
        
        call _CDEXinstalled         ;; CDROM extensions installed?
        ;;cmp  ax, 0
        je   .EndOfProc
        
        mov  ax, 1500h              ;; Get number of drives.
        int  2fh

        push bx
        mov  ax, 150Dh              ;; Get drive numbers.

        push ds
        pop  es
        mov  bx, drives
        int  2fh
        pop  cx

        mov  bx, drives             ;; Make bx point to the beginning
                                    ;; of the string.
        
.loop1:
        add [bx], byte 'A'          ;; Convert to drive letters.

        inc  bx
        dec  cx                     
        jnz  .loop1 

        mov  [bx], byte 0           ;; Add terminating 0.

.EndOfProc:
        mov  ax, drives             ;; Return address string.
        ret
 
%endif

;===========================================================================
;===                             IsCDROM                                 ===
;===========================================================================
;=== int IsCDROM(int drive);                                             ===
;===                                                                     ===
;=== See wether the drive corresponds to a CDROM (0=A:, 1=B:, etc...).   ===
;===                                                                     ===
;=== Remark: the CDEX version has to be at least 2.0.                    ===
;===========================================================================

        global _IsCDROM
_IsCDROM:

        call far _CDEXinstalled     ;; CDROM extensions installed?
        ;;cmp  ax, 0
        je   .EndOfProc

        mov  bx, sp
        mov  cx, [ss:bx+04h]        ;; Get drive number.
        mov  ax, 150bh              ;; See wether it corresponds to a
        int  2fh                    ;; CDROM.

        cmp  ax, 0                  ;; Return 0 if not CDROM.
        je   .EndOfProc

        mov  ax, 1                  ;; Return 1 if CDROM.

.EndOfProc:        
        retf
