
;   FreeDOS DISPLAY:  VideoINT.ASM
;
;   ===================================================================
;
;   Interrupt 10h (video) management routines
;
;   Copyright (C) 9 Aug 2000 Ilya V. Vasilyev aka AtH//UgF@hMoscow
;   e-mail: hscool@netclub.ru
;
;   WWW:    http://www.freedos.org/
;
;   ===================================================================
;
;   ( minimum touches by Aitor )
;               .       .       .       .       .       line that rules
;


;===================================================================
; CONSTANTS

NO              EQU      0      ; ???
MDA             EQU      2      ; Not supported
HGA             EQU      4      ; Currently not supported
CGA             EQU      6      ; Not supported
EGA             EQU      8
MCGA            EQU     10      ; Supported ?
VGA             EQU     12
VESA            EQU     14      ; Currently not detected


;===================================================================
; VARIABLES
;
;               .       .       .       .       .       line that rules

flgOur          DB      0               ; Our INT 10 Fn 00?
bAdapter        DB      0               ; Adapter type detected (one of
                                        ; the constants above)


;===================================================================
; INTERRUPT ROUTINE FOR 10h (video adapter interrup)
;
; ah=00h   (set video mode) 
;          reload the font after changing the video mode
; ax=4f02h (set superVGA mode)
;          reload the font after changing the video mode
; ah=11h   (character generation functions)
;          al= 30h: get info about tables
;          al= 1h, 2h, 4h, 11h, 12h, 14h: load fonts in text mode
;          al= 22h, 23h, 24h: load fonts in graphics mode
;
;               .       .       .       .       .       line that rules

                ;************** check if active

New10:          push    ax
                mov     al,[cs:bActive]  ; are we active?ÿÿÿ
                test    al,0ffh
                pop     ax
                jz      jOld10           ; in that case, switch to old handler


                ;************** check if ah=00h and flgOur is clear

                or      ah,ah
                jnz     i10n0
                cmp     ah,[cs:flgOur]  ; VESA switching can call i10Fn0
                jnz     jnzOld10        ; ah=00h BUT flgOur set, so chain to next int10h

                ;************** AH=00h and flgOur clear: call old, then load fonts
                ; Here we fix Classic Video BIOS Fn 00: Set Screen Mode

                push    ax
                pushf
                call    FAR [cs:dOld10]
                pop     ax              ; Do not assume i10Fn0 saved AL

                push    ax
                and     al,7fH          ; High bit means save video memory
iSFpr:          push    cx              ; preconditions for refreshing
                push    ds              ; hardware codepage
                push    es
                push    di
                push    cs
                pop     es
                call    [cs:pRefreshSWcp]
                pop     di
                pop     es
                pop     ds
                pop     cx

                pop     ax
                iret

                ;************** AH <> 00h
                ;************** check if ah=4f02h (set VESA SuperVGA mode)
i10n0:
                cmp     ax,4f02H        ; VESA SET MODE
                jnz     i10nVESA

                inc     BYTE [cs:flgOur]; prevent our processing the call
                pushf
                call    FAR  [cs:dOld10]
                dec     BYTE [cs:flgOur]
                push    ax
                mov     al,-1           ; Call refresh with Unknown video mode
                jmp     SHORT iSFpr     ; (mode determined dinamically)

jOld10:         DB      0eaH
dOld10:         DD      -16


                ;************** AH <> 00h, 4f02h
                ;************** All other business is with character generator functions (ah=11h)
i10nVESA:
                cmp     ah,11H          ; ah=11h?
jnzOld10:       jnz     jOld10          ; no, then jump to next

                cmp     al,30H          ; Get Font Information?
                jnz     i10f11n30

                ;************** AL = 30h (Get Font Information), BH: font info to be supplied

                ;************** unsupported cases: bh=5, bh>6, bh=6 on NON-VGA

                cmp     bh,5            ; Font 9x14 not supported (now?)
                jz      jOld10
                cmp     bh,6
                ja      jOld10          ; Font 9x16 not supported (now?)
                jb      CheckSubFonts

                cmp     BYTE [cs:bAdapter],VGA
                jb      jOld10          ; We support 8x16 only on VGA

CheckSubFonts:                          ; check that we OWE the subfonts
                cmp     bh,5            ; requested
                ja      csf1
                cmp     bh,2
                jne     i10f1130n6      ; cases 0,1,3,4 always supported
                
csf1:           push    cx
                mov     cx,[cs:wNumSubFonts]
                cmp     cl,2
                ja      pi10f1130n6     ; we have 3 subfonts: always supported
                cmp     bh,6
                je      csfr            ; otherwise, discard bh=6

                cmp     cl,1
                ja      pi10f1130n6     ; we have only 1 subfont

csfr:           pop     cx              ; exit from this block, chain
                jmp     jOld10
                
                ;************** supported cases: bh<5, VGA+bh=6
pi10f1130n6:
                pop     cx              ; we pushed it in CheckSubFonts

i10f1130n6:
                sub     cx,cx           ; CH register must be zero on exit
                mov     es,cx
                mov     dl,[es:484H]    ; Number of lines - 1
                mov     cl,[es:485H]    ; Bytes per char
                cmp     bh,1
                ja      i10f1130a1
                jz      i10f1130s1
                les     bp,[es:7cH]     ; SubFn 00: Return vector 1FH
                iret

i10f1130s1:
                les     bp,[es:43H*4]   ; SubFn 01: Return vector 43H
                iret
i10f1130a1:
                push    cs              ; All other functions return fonts
                pop     es              ;   from DISPLAY.SYS segment
                cmp     bh,3
                ja      i10f1130a3
                jz      i10f1130f3

                mov     bp,bFont8x14    ; SubFn 02: Return ROM font 8x14
                iret
i10f1130f3:
                mov     bp,bFont8x8     ; SubFn 03: Return ROM font 8x8 (00..7f)
                iret
i10f1130a3:
                cmp     bh,6
                jz      i10f1130s6
                mov     bp,bFont8x8+1024; SubFn 04: Return ROM font 8x8 (80..ff)
                iret

i10f1130s6:
                mov     bp,bFont8x16    ; SubFn 06: Return ROM font 8x16
                iret

                ;************** subfunctions AL= 01/11, 02/12 and (VGA+) 04/14

i10f11n30:
                cmp     al,01           ; SubFn 01: Set ROM font 8x14
                jz      i10f1101
                cmp     al,11H          ; SubFn 11: Set ROM font 8x14
                jnz     i10f11n11
i10f1101:
                push    bp
                push    bx
                mov     bp,bFont8x14
                mov     bh,14
i10set:         push    es              ; set a font, by calling ax=1110h, bh=bytes/char
                push    ax
                push    cx
                push    dx
                push    cs
                pop     es
                mov     cx,256          ; load 256 characters
                cwd                     ; DX=0
                and     al,10H
                pushf
                call    FAR [cs:dOld10]
                pop     dx
                pop     cx
                pop     ax
                pop     es
                pop     bx
                pop     bp
                iret
i10f11n11:
                cmp     al,02           ; SubFn 02: Set ROM font 8x8
                jz      i10f1102
                cmp     al,12H          ; SubFn 12: Set ROM font 8x8
                jnz     i10f11n12
i10f1102:
                push    bp
                push    bx
                mov     bp,bFont8x8
                mov     bh,8
                jmp     i10set
i10f11n12:
                cmp     al,04           ; SubFn 04: Set ROM font 8x16
                jz      i10f1104
                cmp     al,14H          ; SubFn 14: Set ROM font 8x16
                jnz     i10f11n14
i10f1104:
                cmp     BYTE [cs:bAdapter],VGA
                jb      jOld10          ; We support 8x16 only on VGA
                push    bp
                push    bx
                mov     bp,bFont8x16
                mov     bh,16
                jmp     i10set

                ;************** subfunctions AL= 22, 23 and (VGA+) 24

i10f11n14:
                cmp     al,22H          ; SubFn 22: Set ROM font 8x14 (INT 43)
                jnz     i10f11n22
                push    bp
                push    cx
                mov     bp,bFont8x14
                mov     cx,14

i10setg:        push    es
                push    ax
                push    cs
                pop     es
                mov     ax,1121H
                pushf
                call    FAR [cs:dOld10]
                pop     ax
                pop     es
                pop     cx
                pop     bp
                iret

i10f11n22:
                cmp     al,23H          ; SubFn 23: Set ROM font 8x8 (INT 43)
                jnz     i10f11n23

                push    bp
                push    cx
                mov     bp,bFont8x8
                mov     cx,8
                jmp     i10setg

i10f11n23:
                cmp     al,24H          ; SubFn 24: Set ROM font 8x16 (INT 43)
                jnz     jOld10
                cmp     BYTE [cs:bAdapter],VGA
                jb      jOld10          ; We support 8x16 only on VGA

                push    bp
                push    cx
                mov     bp,bFont8x16
                mov     cx,16
                jmp     i10setg

