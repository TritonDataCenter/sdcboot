
;   TEST.ASM v0.5b
;
;   Test national language support in different screen modes
;
;   Copyright (C) 9 Aug 2000 Ilya V. Vasilyev aka AtH//UgF@hMoscow
;   e-mail: hscool@netclub.ru
;   WWW:    http://www.freedos.org/
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
; To do:
;   + VESA text and graphic modes
;
;               .       .       .       .       .       line that rules

                ORG     100H
                mov     si,tblModes
                mov     ah,0fH
                int     10H
                push    ax
NxtMode:        lodsw
                cmp     ax,-1
                jz      Xit
                push    ax
                mov     al,ah
                cbw
                int     10H
                pop     ax
                or      al,al
                jz      o0
                cmp     ah,7
                jz      TextMode
                cmp     ah,4
                jae     GraphMode
TextMode:
                sub     ah,ah
                add     ax,1110H
                cmp     al,13H
                jnz     tm1
                inc     ax
tm1:            sub     bx,bx
                int     10H
                jmp     o0
GraphMode:
                sub     ah,ah
                add     ax,1121H
                mov     bl,1
                int     10H
o0:             mov     dx,msg1
                mov     ah,9
                int     21H
                lodsw
                xchg    ax,dx
                mov     ah,9
                int     21H
                sub     ah,ah
                int     16H
                cmp     al,1bH
                jnz     NxtMode
Xit:            pop     ax
                pop     ax
                and     ax,7fH
                int     10H
                int     20H

tblModes:       DW      0000H,msg00,  0001H,msg01,  0002H,msg02,  0003H,msg03
                DW      0100H,msg10,  0101H,msg11,  0102H,msg12,  0103H,msg13
                DW      0200H,msg20,  0201H,msg21,  0202H,msg22,  0203H,msg23
                DW      0300H,msg30,  0301H,msg31,  0302H,msg32,  0303H,msg33
                DW      0400H,msg40,  0401H,msg41,  0402H,msg42,  0403H,msg43
                DW      0500H,msg50,  0501H,msg51,  0502H,msg52,  0503H,msg53
                DW      0600H,msg60,  0601H,msg61,  0602H,msg62,  0603H,msg63
                DW      0700H,msg70,  0701H,msg71,  0702H,msg72,  0703H,msg73
                DW      0d00H,msgd0,  0d01H,msgd1,  0d02H,msgd2,  0d03H,msgd3
                DW      0e00H,msge0,  0e01H,msge1,  0e02H,msge2,  0e03H,msge3
                DW      0f00H,msgf0,  0f01H,msgf1,  0f02H,msgf2,  0f03H,msgf3
                DW      1000H,msg100, 1001H,msg101, 1002H,msg102, 1003H,msg103
                DW      1100H,msg110, 1101H,msg111, 1102H,msg112, 1103H,msg113
                DW      1200H,msg120, 1201H,msg121, 1202H,msg122, 1203H,msg123
                DW      1300H,msg130, 1301H,msg131, 1302H,msg132, 1303H,msg133
                DW      -1

msg00:          DB      "Mode 0 (BW40), default$"
msg01:          DB      "Mode 0 (BW40), font 8x14$"
msg02:          DB      "Mode 0 (BW40), font 8x8$"
msg03:          DB      "Mode 0 (BW40), font 8x16$"
msg10:          DB      "Mode 1 (CO40), default$"
msg11:          DB      "Mode 1 (CO40), font 8x14$"
msg12:          DB      "Mode 1 (CO40), font 8x8$"
msg13:          DB      "Mode 1 (CO40), font 8x16$"
msg20:          DB      "Mode 2 (BW80), default$"
msg21:          DB      "Mode 2 (BW80), font 8x14$"
msg22:          DB      "Mode 2 (BW80), font 8x8$"
msg23:          DB      "Mode 2 (BW80), font 8x16$"
msg30:          DB      "Mode 3 (CO80), default$"
msg31:          DB      "Mode 3 (CO80), font 8x14$"
msg32:          DB      "Mode 3 (CO80), font 8x8$"
msg33:          DB      "Mode 3 (CO80), font 8x16$"
msg40:          DB      "Mode 4 (CO320x200), default$"
msg41:          DB      "Mode 4 (CO320x200), font 8x14$"
msg42:          DB      "Mode 4 (CO320x200), font 8x8$"
msg43:          DB      "Mode 4 (CO320x200), font 8x16$"
msg50:          DB      "Mode 5 (BW320x200), default$"
msg51:          DB      "Mode 5 (BW320x200), font 8x14$"
msg52:          DB      "Mode 5 (BW320x200), font 8x8$"
msg53:          DB      "Mode 5 (BW320x200), font 8x16$"
msg60:          DB      "Mode 6 (640x200), default$"
msg61:          DB      "Mode 6 (640x200), font 8x14$"
msg62:          DB      "Mode 6 (640x200), font 8x8$"
msg63:          DB      "Mode 6 (640x200), font 8x16$"
msg70:          DB      "Mode 7 (MONO), default$"
msg71:          DB      "Mode 7 (MONO), font 8x14$"
msg72:          DB      "Mode 7 (MONO), font 8x8$"
msg73:          DB      "Mode 7 (MONO), font 8x16$"

msgd0:          DB      "Mode 0d (320x200x16), default$"
msgd1:          DB      "Mode 0d (320x200x16), font 8x14$"
msgd2:          DB      "Mode 0d (320x200x16), font 8x8$"
msgd3:          DB      "Mode 0d (320x200x16), font 8x16$"
msge0:          DB      "Mode 0e (640x200x16), default$"
msge1:          DB      "Mode 0e (640x200x16), font 8x14$"
msge2:          DB      "Mode 0e (640x200x16), font 8x8$"
msge3:          DB      "Mode 0e (640x200x16), font 8x16$"
msgf0:          DB      "Mode 0f (640x350x2), default$"
msgf1:          DB      "Mode 0f (640x350x2), font 8x14$"
msgf2:          DB      "Mode 0f (640x350x2), font 8x8$"
msgf3:          DB      "Mode 0f (640x350x2), font 8x16$"
msg100:         DB      "Mode 10 (640x350x4/16), default$"
msg101:         DB      "Mode 10 (640x350x4/16), font 8x14$"
msg102:         DB      "Mode 10 (640x350x4/16), font 8x8$"
msg103:         DB      "Mode 10 (640x350x4/16), font 8x16$"
msg110:         DB      "Mode 11 (640x480x2), default$"
msg111:         DB      "Mode 11 (640x480x2), font 8x14$"
msg112:         DB      "Mode 11 (640x480x2), font 8x8$"
msg113:         DB      "Mode 11 (640x480x2), font 8x16$"
msg120:         DB      "Mode 12 (640x480x16), default$"
msg121:         DB      "Mode 12 (640x480x16), font 8x14$"
msg122:         DB      "Mode 12 (640x480x16), font 8x8$"
msg123:         DB      "Mode 12 (640x480x16), font 8x16$"
msg130:         DB      "Mode 13 (320x200x256), default$"
msg131:         DB      "Mode 13 (320x200x256), font 8x14$"
msg132:         DB      "Mode 13 (320x200x256), font 8x8$"
msg133:         DB      "Mode 13 (320x200x256), font 8x16$"

msg1:           DB      "Сообщение на русском языке.", 0dH, 0aH
                DB      "С БОЛЬШИМИ БУКВАМИ и маленькими.", 0dH, 0aH, 0aH
                DB      "If the above text is correct,",0dH, 0aH
                DB      "you are succeeded in localizing",0dH, 0aH, "$"

                END
