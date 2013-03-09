

;  Permission is hereby granted, free of charge, to any person obtaining a copy
;  of this software and associated documentation files (the "Software"), to deal
;  in the Software without restriction, including without limitation the rights
;  to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
;  copies of the Software, and to permit persons to whom the Software is
;  furnished to do so, subject to the following conditions:
;
;  (1) The above copyright notice and this permission notice shall be included in all
;  copies or substantial portions of the Software.
;
;  (2) The Software, or any portion of it, may not be compiled for use on any
;  operating system OTHER than FreeDOS without written permission from Rex Conn
;  <rconn@jpsoft.com>
;
;  (3) The Software, or any portion of it, may not be used in any commercial
;  product without written permission from Rex Conn <rconn@jpsoft.com>
;
;  (4) THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
;  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
;  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
;  AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
;  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
;  OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
;  SOFTWARE.


.LIST
          ;
          ;
          ; Keystroke values as used within 4DOS
          ;
.XLIST
          ;
SCAN      equ       100h                ;value added to scan code to identify
                                        ;  extended keys
          ;
          ;         Non-printing ASCII
          ;
K_CtlAt   equ       00h                 ; Ctrl-@, NUL
K_Null    equ       K_CtlAt             ; equivalent value
K_CtlA    equ       01h                 ; Ctrl-A, SOM
K_CtlB    equ       02h                 ; Ctrl-B, STX
K_CtlC    equ       03h                 ; Ctrl-C, ETX
K_CtlD    equ       04h                 ; Ctrl-D, EOT
K_CtlE    equ       05h                 ; Ctrl-E, ENQ
K_CtlF    equ       06h                 ; Ctrl-F, ACK
K_CtlG    equ       07h                 ; Ctrl-G, BEL
K_CtlH    equ       08h                 ; Ctrl-H, Backspace
K_Bksp    equ       K_CtlH              ; equivalent value
K_CtlI    equ       09h                 ; Ctrl-I, Tab
K_Tab     equ       K_CtlI              ; equivalent value
K_CtlJ    equ       0Ah                 ; Ctrl-J, Line Feed
K_LF      equ       K_CtlJ              ; equivalent value
K_CtlEnt  equ       K_CtlJ              ; equivalent value
K_CtlK    equ       0Bh                 ; Ctrl-K, Vertical Tab
K_CtlL    equ       0Ch                 ; Ctrl-L, Form Feed
K_CtlM    equ       0Dh                 ; Ctrl-M, Return / Enter
K_Enter   equ       K_CtlM              ; equivalent value
K_CR      equ       K_CtlM              ; equivalent value
K_CtlN    equ       0Eh                 ; Ctrl-N, SO
K_CtlO    equ       0Fh                 ; Ctrl-O, SI
K_CtlP    equ       10h                 ; Ctrl-P, DLE
K_CtlQ    equ       11h                 ; Ctrl-Q, DC1
K_CtlR    equ       12h                 ; Ctrl-R, DC2
K_CtlS    equ       13h                 ; Ctrl-S, DC3
K_CtlT    equ       14h                 ; Ctrl-T, DC4
K_CtlU    equ       15h                 ; Ctrl-U, NAK
K_CtlV    equ       16h                 ; Ctrl-V, SYN
K_CtlW    equ       17h                 ; Ctrl-W, ETB
K_CtlX    equ       18h                 ; Ctrl-X, CAN
K_CtlY    equ       19h                 ; Ctrl-Y, EM
K_CtlZ    equ       1Ah                 ; Ctrl-Z, SUB, EOF
K_Esc     equ       1Bh                 ; Escape
K_FS      equ       1Ch                 ; FS
K_GS      equ       1Dh                 ; GS
K_RS      equ       1Eh                 ; RS
K_US      equ       1Fh                 ; US
          ;
          ;         Standard ASCII
          ;
K_Space   equ       20h                 ; Space
K_Excl    equ       21h                 ; '!'
K_DQuote  equ       22h                 ; '"'
K_Pound   equ       23h                 ; '#'
K_Dollar  equ       24h                 ; '$'
K_Pct     equ       25h                 ; '%'
K_And     equ       26h                 ; '&'
K_SQuote  equ       27h                 ; ''''
K_LPar    equ       28h                 ; '('
K_RPar    equ       29h                 ; ')'
K_Star    equ       2Ah                 ; '*'
K_Plus    equ       2Bh                 ; '+'
K_Comma   equ       2Ch                 ; ','
K_Dash    equ       2Dh                 ; '-'
K_Period  equ       2Eh                 ; '.'
K_Slash   equ       2Fh                 ; '/'
          ;
K_0       equ       30h                 ; '0'
K_1       equ       31h                 ; '1'
K_2       equ       32h                 ; '2'
K_3       equ       33h                 ; '3'
K_4       equ       34h                 ; '4'
K_5       equ       35h                 ; '5'
K_6       equ       36h                 ; '6'
K_7       equ       37h                 ; '7'
K_8       equ       38h                 ; '8'
K_9       equ       39h                 ; '9'
          ;
K_Colon   equ       3Ah                 ; ':'
K_Semi    equ       3Bh                 ; ';'
K_LAngle  equ       3Ch                 ; '<'
K_Equal   equ       3Dh                 ; '='
K_RAngle  equ       3Eh                 ; '>'
K_Quest   equ       3Fh                 ; '?'
K_AtSign  equ       40h                 ; '@'
          ;
K_A       equ       41h                 ; 'A'
K_B       equ       42h                 ; 'B'
K_C       equ       43h                 ; 'C'
K_D       equ       44h                 ; 'D'
K_E       equ       45h                 ; 'E'
K_F       equ       46h                 ; 'F'
K_G       equ       47h                 ; 'G'
K_H       equ       48h                 ; 'H'
K_I       equ       49h                 ; 'I'
K_J       equ       4Ah                 ; 'J'
K_K       equ       4Bh                 ; 'K'
K_L       equ       4Ch                 ; 'L'
K_M       equ       4Dh                 ; 'M'
K_N       equ       4Eh                 ; 'N'
K_O       equ       4Fh                 ; 'O'
K_P       equ       50h                 ; 'P'
K_Q       equ       51h                 ; 'Q'
K_R       equ       52h                 ; 'R'
K_S       equ       53h                 ; 'S'
K_T       equ       54h                 ; 'T'
K_U       equ       55h                 ; 'U'
K_V       equ       56h                 ; 'V'
K_W       equ       57h                 ; 'W'
K_X       equ       58h                 ; 'X'
K_Y       equ       59h                 ; 'Y'
K_Z       equ       5Ah                 ; 'Z'
           ;
K_LSqr    equ       5Bh                 ; '['
K_BSlash  equ       5Ch                 ; '\'
K_RSqr    equ       5Dh                 ; ']'
K_Caret   equ       5Eh                 ; '^'
K_Under   equ       5Fh                 ; '_'
K_BQuote  equ       60h                 ; '`'
          ;
K_al      equ       61h                 ; 'a'
K_bl      equ       62h                 ; 'b'
K_cl      equ       63h                 ; 'c'
K_dl      equ       64h                 ; 'd'
K_el      equ       65h                 ; 'e'
K_fl      equ       66h                 ; 'f'
K_gl      equ       67h                 ; 'g'
K_hl      equ       68h                 ; 'h'
K_il      equ       69h                 ; 'i'
K_jl      equ       6Ah                 ; 'j'
K_kl      equ       6Bh                 ; 'k'
K_ll      equ       6Ch                 ; 'l'
K_ml      equ       6Dh                 ; 'm'
K_nl      equ       6Eh                 ; 'n'
K_ol      equ       6Fh                 ; 'o'
K_pl      equ       70h                 ; 'p'
K_ql      equ       71h                 ; 'q'
K_rl      equ       72h                 ; 'r'
K_sl      equ       73h                 ; 's'
K_tl      equ       74h                 ; 't'
K_ul      equ       75h                 ; 'u'
K_vl      equ       76h                 ; 'v'
K_wl      equ       77h                 ; 'w'
K_xl      equ       78h                 ; 'x'
K_yl      equ       79h                 ; 'y'
K_zl      equ       7Ah                 ; 'z'
          ;
K_LBrace  equ       7Bh                 ; '{'
K_Bar     equ       7Ch                 ; '|'
K_RBrace  equ       7Dh                 ; '}'
K_Tilde   equ       7Eh                 ; '~'
K_CtlBS   equ       7Fh                 ; Ctrl-Backspace
          ;
K_255     equ       0FFh                ; Alt-2-5-5
          ;
          ;         Extended Keys - Alt + Alphanumeric
          ;
K_AltA    equ       1Eh + SCAN          ; Alt-A
K_AltB    equ       30h + SCAN          ; Alt-B
K_AltC    equ       2Eh + SCAN          ; Alt-C
K_AltD    equ       20h + SCAN          ; Alt-D
K_AltE    equ       12h + SCAN          ; Alt-E
K_AltF    equ       21h + SCAN          ; Alt-F
K_AltG    equ       22h + SCAN          ; Alt-G
K_AltH    equ       23h + SCAN          ; Alt-H
K_AltI    equ       17h + SCAN          ; Alt-I
K_AltJ    equ       24h + SCAN          ; Alt-J
K_AltK    equ       25h + SCAN          ; Alt-K
K_AltL    equ       26h + SCAN          ; Alt-L
K_AltM    equ       32h + SCAN          ; Alt-M
K_AltN    equ       31h + SCAN          ; Alt-N
K_AltO    equ       18h + SCAN          ; Alt-O
K_AltP    equ       19h + SCAN          ; Alt-P
K_AltQ    equ       10h + SCAN          ; Alt-Q
K_AltR    equ       13h + SCAN          ; Alt-R
K_AltS    equ       1Fh + SCAN          ; Alt-S
K_AltT    equ       14h + SCAN          ; Alt-T
K_AltU    equ       16h + SCAN          ; Alt-U
K_AltV    equ       2Fh + SCAN          ; Alt-V
K_AltW    equ       11h + SCAN          ; Alt-W
K_AltX    equ       2Dh + SCAN          ; Alt-X
K_AltY    equ       15h + SCAN          ; Alt-Y
K_AltZ    equ       2Ch + SCAN          ; Alt-Z
          ;
K_Alt0    equ       81h + SCAN          ; Alt-0
K_Alt1    equ       78h + SCAN          ; Alt-1
K_Alt2    equ       79h + SCAN          ; Alt-2
K_Alt3    equ       7Ah + SCAN          ; Alt-3
K_Alt4    equ       7Bh + SCAN          ; Alt-4
K_Alt5    equ       7Ch + SCAN          ; Alt-5
K_Alt6    equ       7Dh + SCAN          ; Alt-6
K_Alt7    equ       7Eh + SCAN          ; Alt-7
K_Alt8    equ       7Fh + SCAN          ; Alt-8
K_Alt9    equ       80h + SCAN          ; Alt-9
          ;
          ;         Extended Keys - Misc and Keypad
          ;
K_AltBS   equ       0Eh + SCAN          ; Alt-Bksp
K_ShfTab  equ       0Fh + SCAN          ; Shift-Tab
K_CtlTab  equ       94h + SCAN          ; Ctrl-Tab
K_Left    equ       4Bh + SCAN          ; Left Arrow
K_CtlLft  equ       73h + SCAN          ; Ctrl-Left Arrow
K_Right   equ       4Dh + SCAN          ; Right Arrow
K_CtlRt   equ       74h + SCAN          ; Ctrl-Right Arrow
K_Up      equ       48h + SCAN          ; Up Arrow
K_Down    equ       50h + SCAN          ; Down Arrow
K_Home    equ       47h + SCAN          ; Home
K_CtlHm   equ       77h + SCAN          ; Ctrl-Home
K_End     equ       4Fh + SCAN          ; End
K_CtlEnd  equ       75h + SCAN          ; Ctrl-End
K_PgUp    equ       49h + SCAN          ; PgUp
K_CtlPgU  equ       84h + SCAN          ; Ctrl-PgUp
K_PgDn    equ       51h + SCAN          ; PgDn
K_CtlPgD  equ       76h + SCAN          ; Ctrl-PgDn
K_Ins     equ       52h + SCAN          ; Ins
K_Del     equ       53h + SCAN          ; Del
K_CtlIns  equ       92h + SCAN          ; Ctrl-Ins
K_CtlDel  equ       93h + SCAN          ; Ctrl-Del
          ;
          ;         Extended Keys - Function Keys
          ;
K_F1      equ       3Bh + SCAN          ; F1
K_F2      equ       3Ch + SCAN          ; F2
K_F3      equ       3Dh + SCAN          ; F3
K_F4      equ       3Eh + SCAN          ; F4
K_F5      equ       3Fh + SCAN          ; F5
K_F6      equ       40h + SCAN          ; F6
K_F7      equ       41h + SCAN          ; F7
K_F8      equ       42h + SCAN          ; F8
K_F9      equ       43h + SCAN          ; F9
K_F10     equ       44h + SCAN          ; F10
K_F11     equ       85h + SCAN          ; F11
K_F12     equ       86h + SCAN          ; F12
K_ShfF1   equ       54h + SCAN          ; ShfF1
K_ShfF2   equ       55h + SCAN          ; ShfF2
K_ShfF3   equ       56h + SCAN          ; ShfF3
K_ShfF4   equ       57h + SCAN          ; ShfF4
K_ShfF5   equ       58h + SCAN          ; ShfF5
K_ShfF6   equ       59h + SCAN          ; ShfF6
K_ShfF7   equ       5Ah + SCAN          ; ShfF7
K_ShfF8   equ       5Bh + SCAN          ; ShfF8
K_ShfF9   equ       5Ch + SCAN          ; ShfF9
K_ShfF10  equ       5Dh + SCAN          ; ShfF10
K_ShfF11  equ       87h + SCAN          ; ShfF11
K_ShfF12  equ       88h + SCAN          ; ShfF12
K_CtlF1   equ       5Eh + SCAN          ; CtlF1
K_CtlF2   equ       5Fh + SCAN          ; CtlF2
K_CtlF3   equ       60h + SCAN          ; CtlF3
K_CtlF4   equ       61h + SCAN          ; CtlF4
K_CtlF5   equ       62h + SCAN          ; CtlF5
K_CtlF6   equ       63h + SCAN          ; CtlF6
K_CtlF7   equ       64h + SCAN          ; CtlF7
K_CtlF8   equ       65h + SCAN          ; CtlF8
K_CtlF9   equ       66h + SCAN          ; CtlF9
K_CtlF10  equ       67h + SCAN          ; CtlF10
K_CtlF11  equ       89h + SCAN          ; CtlF11
K_CtlF12  equ       8Ah + SCAN          ; CtlF12
K_AltF1   equ       68h + SCAN          ; AltF1
K_AltF2   equ       69h + SCAN          ; AltF2
K_AltF3   equ       6Ah + SCAN          ; AltF3
K_AltF4   equ       6Bh + SCAN          ; AltF4
K_AltF5   equ       6Ch + SCAN          ; AltF5
K_AltF6   equ       6Dh + SCAN          ; AltF6
K_AltF7   equ       6Eh + SCAN          ; AltF7
K_AltF8   equ       6Fh + SCAN          ; AltF8
K_AltF9   equ       70h + SCAN          ; AltF9
K_AltF10  equ       71h + SCAN          ; AltF10
K_AltF11  equ       8Bh + SCAN          ; AltF11
K_AltF12  equ       8Ch + SCAN          ; AltF12
          ;
.LIST

