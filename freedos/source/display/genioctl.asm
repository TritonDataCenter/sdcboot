
;   FreeDOS DISPLAY.SYS           v0.13
;   FreeDOS PRINTER.SYS
;
;   ===================================================================
;
;   GENERIC IOCTL SUBROUTINES
;
;   ===================================================================
;
;   FUNCTION summary:
;
;   EXPORTS:
;
;   GenericIoctl   ( IN:      CL:      subfunction number
;                             DS:BX->  specific parameter table
;                    OUT:     CF       clear on success, set on error
;                             AX       (on ERROR) error code
;
;       .       .       .       .       .       .       = RULER =

fInPrepare      DB      0               ; Preparation in course?

; Fn:   GenericIOCTL
; Does: Implements the Generic IOCTL call
; In:   CL:      subfunction number
;       DS:BX->  specific parameter table
; Out:  cf  set on error. In that case:
;         AX:      error code
;
; FUNCTIONS 45h/65h: set/get iteration count  (UNSUPPORTED)
; ------------------------------------------
; 00h  DW   Iteration count
;
; FUNCTION 4Ah/6Ah: select/query codepage
; ---------------------------------------
; 00h  DW   Length of data (ignored)
; 02h  DW   codepage ID
;
; FUNCTION 4Ch: start codepage prepare
; ------------------------------------
; 00h  DW   Flags (must be 0 for DISPLAY,
;                  bit0: prepare catridge selection in PRINTER.SYS)
;                 (we shall IGNORE these flags)
; 02h  DW   Length of the rest in BYTES ( 2 + 2N ) (will be ignored)
; 04h  DW   Number N of codepages
; 06h  DW times N  Codepages to prepare
;
; FUNCTION 4Dh: end codepage prepare  (IGNORE PACKET)
; ----------------------------------
;
; FUNCTION 6Bh: query prepared codepages list
; -------------------------------------------
; 00h  DW   total length of data
; 02h  DW   number of hardware CPs (N)
; 04h  DW times N  hardware CPs
; xxh  DW   number of prepared CPs (M)
; xxh  DW times M  prepared CPs
;
; FUNCTIONS 5Fh/7Fh: set/get Display Information Packet
; -----------------------------------------------------
; 00h  DB   Video parameters packet
; 01h  DB   Reserved (=0)
; 02h  DW   Packet length (=14)
; 04h  DW   Flags (0: intense; 1: blink)
; 06h  DB   Mode  (1: text, 2: graphics)
; 07h  DB   Reserved (=0)
; 08h  DW   Colours  (0 if mono)
; 0Ah  DW   Width (APA width in pixels)
; 0Ch  DW   Length (APA length in pixels)
; 0Eh  DW   Width in characters (columns)
; 10h  DW   Lehgth in characters (rows)
;

GenericIoctl:

                ;****** Select codepage
                cmp     cl, 04Ah
                jne     GI_no04Ah

                push    bx
                mov     bx, [ds:bx+2]
                call    SelectCodepage
                pop     bx
                test    ax, 0ffffh
                jnz     GI_success
                mov     ax, dx
                jmp     GI_error

GI_no04Ah:
                ;****** Query codepage
                cmp     cl, 06Ah
                jne     GI_no06Ah

                mov     ax,[cs:wCPselected]
                mov     [ds:bx+2],ax
                cmp     ax,-1
                jne     GI_success

                mov     ax, ERR_CPNotSelected
                jmp     GI_error

GI_no06Ah:
                ;****** Start codepage prepare
                cmp     cl, 04Ch
                jne     GI_no04Ch

                mov     cx, [ds:bx+2]
                mov     si, bx
                add     si, 4

                push    cs
                pop     es
                mov     di, wToPrepareSize

                cld
           rep  movsb

                mov     byte [cs:fInPrepare], 1
                jmp     GI_success

GI_no04Ch:
                ;****** End codepage prepare
                cmp     cl, 04Dh
                jne     GI_no04Dh

                test    byte [cs:fInPrepare],0FFh
                jnz     endprepOk

                mov     ax,ERR_NoPrepareStart
                jmp     GI_error
endprepOk:
                mov     byte [cs:fInPrepare], 0

GI_success:
                xor     ax, ax
                clc
                ret

GI_no04Dh:
                ;****** Query codepage list
                cmp     cl, 06Bh
                jne     GI_no06Bh

                mov     ax, [cs:wNumHardCPs]    ; length of data
                add     ax, [cs:wNumSoftCPs]
                shl     ax, 1                   ; was in words
                add     ax, 4
                mov     [ds:bx], ax

                mov     cx, [cs:wNumHardCPs]    ; number of HW CPs
                mov     [ds:bx+2], cx

                mov     di, bx                  ; hardware CPs
                add     di, 4
                push    ds
                pop     es
                push    cs
                pop     ds
                mov     si, wCPList
                cld
           rep  movsw

                mov     ax, [cs:wNumSoftCPs]    ; number of SW CPs
                mov     [es:di], ax
                add     di, 2

                mov     si, wCPList             ; software CPs
                add     si, [cs:wNumHardCPs]
                add     si, [cs:wNumHardCPs]
                mov     cx, [cs:wNumSoftCPs]
           rep  movsw

                cmp     word [cs:wCPselected],-1
                jne     GI_success

                mov     ax, ERR_CPNotSelected
                jmp     GI_error

GI_no06Bh:
                ;****** Unknown function

                test    cl, 80h
                jnz     GI_success
                
                mov     ax, ERR_UnknownFunction

GI_error:
                stc
                ret

