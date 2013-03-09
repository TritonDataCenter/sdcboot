;    
;   dpmitst.asm - routine to test wether dpmi is installed.
;   Copyright (C) 1999 Imre Leber
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

segment DPMITST_TEXT

;=========================================================================
;===                      DPMIinstalled                                ===
;===-------------------------------------------------------------------===
;===  Returns wether dpmi is installed.                                ===
;===                                                                   ===
;===  int DPMIinstalled (void);                                        ===
;=========================================================================

        global _DPMIinstalled
_DPMIinstalled:

        mov  ax, 1687h
        int  2fh

        cmp  ax, 0
        jne  .NODPMI 
        
        mov  ax, 1
        jmp  short .EndOfProc

.NODPMI:
        xor  ax, ax

.EndOfProc:
        retf
