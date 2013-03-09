
;   FreeDOS DISPLAY:  MuxINT.ASM
;
;   ===================================================================
;
;   Multiplexer (2Fh) DISPLAY API management routines
;
;   Copyright (C) 27 Aug 2002-03 Aitor Santamar¡a_Merino
;   email:  aitor.sm@wanadoo.es
;
;   WWW:    http://www.freedos.org/
;
;   ===================================================================
;
;   ( minimum touches by Ilya )
;               .       .       .       .       .       line that rules
;

;===================================================================
; CONSTANTS
;

MuxCode         EQU     0adh            ; for DISPLAY and KEYB

;===================================================================
; INTERRUPT ROUTINE FOR 2Fh (DOS Multiplexer)
;
; 00: Installation check (return al non-zero)
; 01: Set Active Codepage (IN: bx=requested codepage)
; 02: Get Active Codepage (OUT: bx=active codepage)
; 03: Get codepage information table
; 0E: (DISPLAY 0.12-0.99) IOCTL calls. Subfunctions in BX:
;     BX=0003h:  Write to IOCTL
;     BX=000Ch:  Generic IOCTL
;
;
; Note: DISPLAY may not save information about 8x16 font on EGA,
;       so don't expect to get 8x16 font from EGA via Fn 0F
;
;        .      .       .       .       .       .       line that rules

                ;************** check first MUX code for DISPLAY: ADh

New2f:          pushf                   ; we must preserve flags!

                cmp     ax,0B000h       ; if GRAFTABL wants to load
                jne     True2f
                mov     al,01           ; say: it is NOT ok to load
                jmp     retClear

True2f:
                cmp     ah,MuxCode
                jnz     jOld2f

                ;************** function 00h: Installation check

                test    al,0ffH
                jnz     mDno0

                mov     al,0ffH         ; Installation check
                mov     bx, Version     ; return version in BX (major.minor)
                                        ; bh=0 will stand for the beta versions of
                                        ; FreeDOS DISPLAY
                popf
                clc                     ; clear carry
                retf    2

                ;************** function 01h: Set active codepage
                ; Exit on error:   CF set, AX=0
                ; Exit on success: CF clear, AX=1

mDno0:          cmp     al,01
                jnz     mDno1

                push    dx
                call    SelectCodepage
                pop     dx               ; possible error code is lost

                jc      retSet

retClear:       popf                    ; restore flags,
                clc                     ; clear carry (success)
                retf    2               ; and return, discarding flags
retSet:
                popf
                stc                     ; set carry: error
                retf    2


                ;************** function 02h: Get active codepage

mDno1:          cmp     al,02
                jnz     mDno2           ; we return the active codepage

                mov     bx, [cs:wCPselected]

                cmp     bx, -1          ; if it was never set,
                jne     retClear        ; then we have to
                mov     ax, 0001h       ; set ax to 1 (see RBIL)
                jmp     retSet

                ;************** jump to next multiplex handler

jOld2f:         popf
                DB      0eaH
dOld2f:         DD      -16

                ;************** function 03h: Get codepage information

mDno2:          cmp     al,03
                jnz     mDno3

                push    ax
                mov     ax,[cs:wNumSoftCPs]
                add     ax,[cs:wNumHardCPs]
                add     ax,3            ; cps + header (3)
                shl     ax,1            ; they are in words!
                cmp     ax,cx
                mov     cx,ax           ; if failure: CX: size of table
                pop     ax
                ja      retSet          ; buffer too small!

                push    ds              ; set DS:SI
                push    si

                push    cs
                pop     ds

                mov     si,wNumSoftCPs
                shr     cx,1            ; transfer size = our size (in words)
                cld			; for crazy callers
           rep  movsw

                pop     si
                pop     ds

                jmp     retClear

mDno3:

                ;************** function 0Eh: IOCTL call for DISPLAY 0.12-0.99
                ; TO DISAPPEAR FOR DEVICE DRIVER.SYS
                ; BX: subfunction
                ; 0003h: Write to IOCTL
                ;   CX:      amount of data
                ;   DS:DX->  data
                ; 000Ch: Generic IOCTL
                ;   CH=3         (if not, error will be reported)
                ;   CL=Generic subfunction
                ;   DS:DX->  specific packet
                ;
                ; Return:
                ;   success: CF clear (in write, AX=number of bytes written)
                ;   failure: CF set, AX=error code

                cmp     al,0eh
                jne     jOld2f

                push    bx
                push    cx
                push    dx
                push    si
                push    di
                push    ds
                push    es

                cmp     bx,0003h
                jne     mDno53

                mov     si,dx
                call    PrepareCodepage
                mov     ax,dx

                pop     es
                pop     ds
                pop     di
                pop     si
                pop     dx
                pop     cx
                pop     bx
                
                test    ax,ax
                jnz     retSet

                mov     ax,cx
                clc
                jmp     retClear

mDno53:         mov     ax, ERR_UnknownFunction
                cmp     ch,3
                jne     PopNRet

                mov     bx,dx
                call    GenericIoctl

PopNRet:
                pop     es
                pop     ds
                pop     di
                pop     si
                pop     dx
                pop     cx
                pop     bx

                test    ax,ax
                jnz     retSet

                mov     ah,MuxCode
                mov     al,0eh
                jmp     retClear



