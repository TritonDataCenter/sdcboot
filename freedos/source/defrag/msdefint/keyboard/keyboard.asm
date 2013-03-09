;    
;   Keyboard.asm - general keyboard routines.
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
;   email me at:  imre.leber@vub.ac.be
;

segment KEYBOARD_TEXT 

;========================================================================
;===                         AltKeyDown                               ===   
;========================================================================
;=== int AltKeyDown (void)                                            ===
;===                                                                  ===
;=== Returns: 0 : alt key not down.                                   ===
;===          1 : alt key down.                                       ===
;========================================================================
        global _AltKeyDown
_AltKeyDown:

        mov  ah, 02h               ; Get keyboard status flags.
        int  16h 
        
;        xor  ah, ah                ; Return an integer.
;        and  al, 1000b             ; Get alt key flag.
        and  ax, 1000b

        retf

;========================================================================
;===                         KeyPressed                               ===   
;========================================================================
;=== int KeyPressed (void)                                            ===
;===                                                                  ===
;=== Returns: 0 : no key pressed.                                     ===
;===          1 : key pressed.                                        ===
;========================================================================
        global _KeyPressed
_KeyPressed:

        mov  ah, 01h               ; Return wether key pressed.
        int  16h

        jz   .None                 ; If key was pressed.

        mov  ax, 1                 ; return 1

        jmp  short .EndOfProc      

.None:                             ; If key was not pressed.
        xor  ax, ax                ; return 0.

.EndOfProc:
        retf

;========================================================================
;===                         ReadKey                                  ===   
;========================================================================
;=== int ReadKey (void)                                               ===
;===                                                                  ===
;=== Returns: key pressed as follows:                                 ===
;===           < 256 : ASCII code                                     ===
;===           > 256 : extended code (% 256 == 0)                     ===
;===                                                                  ===
;=== Waits until a key is pressed.                                    ===
;========================================================================

        global _ReadKey
_ReadKey:

        xor  ah, ah                ; Wait until a key was pressed.
        int  16h

        cmp  al, 0                 ; Exit if extended key code.
        je   .EndOfProc

        xor  ah, ah                ; Otherwise ignore scancode key.

.EndOfProc:
        retf

;========================================================================
;===                         ClearKeyBoard                            ===   
;========================================================================
;=== void ClearKeyboard (void)                                        ===
;===                                                                  ===
;=== Clears the keyboard buffer.                                      ===
;========================================================================

        global _ClearKeyboard
_ClearKeyboard:

.loop:                             ; As long as there are still keys
        mov  ah, 01h               ; in the queue.
        int  16h
        jz   .EndOfProc 

        xor  ah, ah                ; Remove the key from the queue.
        int  16h

        jmp  short .loop

.EndOfProc:
        retf
