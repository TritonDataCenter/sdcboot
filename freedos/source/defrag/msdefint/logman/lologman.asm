;    
;  Lologman.asm - low screen log routines.
;
;  Copyright (C) 2000 Imre Leber
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
; 

extern GetScreenAddress

segment LOLOGMAN_TEXT        

;=======================================================================
;===                        ShowScreenPage                           ===
;===-----------------------------------------------------------------===
;=== Shows a certain screen page.                                    ===
;===                                                                 ===
;=== void ShowScreenPage(int page);                                  ===
;=======================================================================

        global _ShowScreenPage
_ShowScreenPage:

        mov  bx, sp
        mov  ax, [ss:bx+04h]
        mov  ah, 05h

        int  10h

        retf

;=======================================================================
;===                        PrintChar1                               ===
;===-----------------------------------------------------------------===
;=== Prints a character on screen page 1.                            ===
;===                                                                 ===
;=== void PrintChar1(int asciichar);                                 ===
;=======================================================================

        global _PrintChar1
_PrintChar1:
        
        push  bp
        mov   bp, sp

        mov  bh, 1
        mov  bl, 0Fh
        mov  ax, [bp+06h]
        mov  cx, 1

        mov  ah, 09h
        int  10h
        
        mov  ah, 03h
        mov  bh, 01h
        int  10h
        inc  dl
        mov  ah, 02h
        int  10h 

        pop  bp
        retf

;=======================================================================
;===                        gotoxy1                                  ===
;===-----------------------------------------------------------------===
;=== Changes the cursor position on screen page 1.                   ===
;===                                                                 ===
;=== void gotoxy1(int x, int y);                                     ===
;=======================================================================

        global _gotoxy1
_gotoxy1:
         push bp
         mov  bp, sp

         mov  dx, [bp+06h]
         mov  ax, [bp+08h]
         mov  dh, al

         mov  ax, 0200h
         mov  bh, 1
         int  10h

         pop  bp
         retf

;==========================================================================
;===                        Scroll1Up                                   ===
;===--------------------------------------------------------------------===
;=== Scrolls screen page 1 one line up.                                 ===
;===                                                                    ===
;=== void Scroll1Up(void);                                              ===
;==========================================================================

        global _Scroll1Up
_Scroll1Up:

        push ds
        push es
        push si
        push di

        call far GetScreenAddress            ;; in ..\c_repl\gdscreen.asm 

        push ds
        pop  es

        mov  si, 4096+160
        mov  di, 4096
        cld
        mov  cx, 2000
        rep  movsw

        mov  ax, 0000h
        mov  cx, 80
        rep  stosw
        
        pop  di
        pop  si
        pop  es
        pop  ds
        retf

