;
;   Lowtime.asm - routine to get the time.
;   Copyright (C) 2000 Imre Leber
;
;  This program is free software; you can redistribute it and/or modify
;  it under the terms of the GNU General Public License as published by
;  the Free Software Foundation; either version 2 of the License, or
;  (at your option) any later version.
;
;  This program is distributed in the hope that it will be useful,
;  but WITHOUT ANY WARRANTY; without even the implied warranty of
;  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
;  GNU General Public License for more details.
;
;  You should have received a copy of the GNU General Public License
;  along with this program; if not, write to the Free Software
;  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
;
;  If you have any questions, comments, suggestions, or fixes please
;  email me at:  imre.leber@worldonline.be


segment LOWTIME_TEXT

;==========================================================================
;===                             GetTime                                ===
;===--------------------------------------------------------------------===
;=== Gets the time.                                                     ===
;===                                                                    ===
;=== void GetTime(int* hours, int* minutes, int* seconds);              ===
;===                                                                    ===
;=== Note: doesn't bother with mili seconds.                            ===
;==========================================================================

        global _GetTime
_GetTime:

        push bp
        mov  bp, sp
        push ds

        mov  ah, 2Ch
        int  21h 

        mov  al, ch                  ;; Convert to integer values.
        xor  ah, ah
        xor  ch, ch
        mov  dl, dh
        xor  dh, dh

        lds  bx, [bp+06h]
        mov  [bx], ax
        lds  bx, [bp+0Ah]
        mov  [bx], cx
        lds  bx, [bp+0Eh]
        mov  [bx], dx

        pop  ds
        pop  bp
        retf
