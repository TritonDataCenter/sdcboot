;
;   FreeDOS DISPLAY.SYS           v0.13
;   FreeDOS PRINTER.SYS
;
;   ===================================================================
;
;   SELECT RESIDENT SUBROUTINES
;
;   ===================================================================
;
;   FUNCTION summary:
;
;   EXPORTS:
;   SelectCodepage ( IN:        BX=codepage
;                    OUT-ok:    CF clear
;                               AX=1
;                               DX preserved
;                    OUT-error: CF set
;                               AX=0
;                               DX=errorcode )
;                    All other registers preserved
;
;   INTERNAL FUNCTIONS:
;   - FindCodepage: finds the position of given codepage in the
;     codepage array
;   - GetMemoryLocation: recovers info about handle/seg/ofs of
;     certain buffer to be used
;   - MoveBufferToSelect: moves info from a PREPARE buffer to the
;     select buffer
;
;       .       .       .       .       .       .       = RULER =


; Fn:   FindCodepage
; Does: Finds a codepage in the prepared array
; In:   BX=new codepage
; Out:  CL=position in table (starts on 0=first hardware codepage, ...)
;          (0ffh if not found)
;       CH=00h


FindCodepage:   xor     cx,cx           ; CL: total table size
                mov     cl,[cs:wNumSoftCPs]
                add     cl,[cs:wNumHardCPs]
                push    bx
                mov     ax,bx
                xor     bh,bh
                mov     bl,cl           ; copy of table size on BL

                dec     bl
                shl     bx,1
                
                mov     di,wCPList      ; DS:SI: begining of table
                add     di,bx  ; NUEVA
                push    es
                push    cs
                pop     es
                std

        repne   scasw

                test    cl,0ffh
                jnz     CodepageFound
                mov     cl,0ffh
                jmp     FindCodepageEnd


CodepageFound:
FindCodepageEnd:
                pop     es
                pop     bx
                ret

; Fn:   GetMemoryLocation
; Does: From the number of buffer to be used, obtains the
;       Handle, Segment and Offset to be used in the memory transfer
; In:   CX: number of buffer to be used (untouched)
; Out:  AX: Handle
;       DX: Segment // upperWord of offset
;       SI: Offset

GetMemoryLocation:
                push    cx
                shl     cx,1
                mov     si,wBufferPtrs
                add     si,cx
                mov     dx,[cs:si]              ; DX: stored value
                pop     cx

                mov     al,1
                shl     al,cl
                test    al,[cs:fBuffersInXMS]
                jz      RealMemoryData

XMSMemoryData:
                mov     ax,dx
                xor     dx,dx
                xor     si,si
                ret

RealMemoryData:
                xor      ax,ax
                mov      si,dx
                push     cs
                pop      dx
                ret



; Fn:   MoveBufferToSelect
; Does: Moves info from a prepare buffer to the select buffer (SelectBuffer)
; In:   CL: Number of buffer that is to be used for the transfer
; Out:  CFlag set on error

MoveBufferToSelect:

                ; Stablish the source
                call    GetMemoryLocation

                test    ax, ax
                jnz     FromXMS

                ; Real move
                push    es
                push    ds
                mov     cx,[cs:wBufferSize]
                mov     di,SelectBuffer
                mov     ax,cs
                mov     es,ax
                mov     ds,dx
                cld
        rep     movsb
                pop     ds
                pop     es
                clc
                ret
                                                                                                                                                                                                                
FromXMS:
                mov     [cs:XMSMoveSrcH],ax
                mov     [cs:XMSMoveSrcO+2],dx
                mov     word [cs:XMSMoveSrcO],si

                ; Stablish the target

                mov     word [cs:XMSMoveTrgH],0
                mov     [cs:XMSMoveTrgO+2],cs
                mov     word [cs:XMSMoveTrgO],SelectBuffer

                ; Stablish the length

                mov     cx,[cs:wBufferSize]

                ; Do the move

                mov     word [cs:XMSMoveLen],cx

                push    cs
                pop     ds
                mov     si,XMSMoveLen

                ; Stablish the function number and call XMS
                
                mov     ah,0Bh

                call    far [cs:lpXMSdriver]
                cmp     ax,1
                je      ReturnSuccess
                stc
                ret
ReturnSuccess:
                clc
                ret


; Fn:   SelectCodepage
; Does: Selects a codepage
; In:   BX=new codepage
; Error:   CF set
;          AX=0 if not found, and in this case
;          DX= error code
; Success: AX=1
;          CF clear
;          DX untouched

whtCPselected   DW      0               ; position in the table of cp numbers

SelectCodepage:
               ;************ PUSH globally DI, SI, DX

                push    di
                push    si
                push    dx

                ;************ check if KEYB/PRINT are ready for the change

                push    bx              ; save cp number, just in case
;jmp KeybOK ; Fast and dirty way to bypass Keyb support
%ifdef          DISPLAY
                mov     ax,0ad80h       ; first see if it is installed
                int     02fh
                cmp     al,0ffh
                jne     KeybOK          ; if not found, continue
                mov     ax,0ad81h
                pop     bx
                clc
                push    bx
                int     02fh
                jnc     KeybOK
                jmp     KeybFailed
%endif

%ifdef          PRINTER
                push    ds
                mov     ax,0104h        ; Freeze status to read config
                int     02fh
                jc      PrintFailed
                mov     dl,[ds:si]      ; get first byte of the first
                pop     ds              ; file in the print queue
                mov     ax,0105h        ; Restore status (hope to succeed!)
                int     02fh
                test    dl,0ffh         ; now test if there are files to
                jz      KeybOK          ; print, if there is any -> fail
                jmp     KeybFailed      ; (DS already pop-ed)

PrintFailed:    pop     ds
%endif

KeybFailed:
                pop     bx              ; KEYB or PRINT are not ready
                mov     dx, ERR_KEYBFailed
                jmp     SelectCodepageError

                ;************ If it is the ACTIVE, then REFRESH

KeybOK:
                pop     bx              ; recover codepage number

                cmp     bx, [cs:wCPselected]
                jne     CheckBuffer

                push    cx
                test    BYTE [cs:bActive], 0ffh
                jz      RefreshHW

PreRefreshSW:
                push    es
                push    ds
                
                jmp     RefreshSW

                ;************ see if it is the one in the SELECT buffer

CheckBuffer:
                cmp     bx,[cs:wCPbuffer]
                jne     NotRefresh

                push    cx
                jmp     PreRefreshSW

                ;************ see if it is in the table
                
NotRefresh:
                push    cx
                call    FindCodepage    ; see if we find it

                cmp     cl, 0ffh
                jne     CPFound
                mov     dx, ERR_CPNotPrepared
                pop     cx
                jmp     SelectCodepageError

CPFound:
                ;************ READY TO SELECT

                cmp     cx,[cs:wNumHardCPs]
                jae     SetSWCodepage
                mov     [cs:whtCPselected], cx

                ;************ set the hardware codepage

RefreshHW:
                mov     cx,[cs:whtCPselected]
                call    [cs:pRefreshHWcp]
                pop     cx

                jc      SelectCodepageError

                mov     BYTE [cs:bActive],0

                jmp     SelectCodepageSuccess

                ;************ set the software codepage

SetSWCodepage:
                push    es
                push    ds

                sub     cx,[cs:wNumHardCPs]  ; CX=0,1,2...

                call    MoveBufferToSelect
                mov     dx,ERR_SelPrepDeviceError
                jc      SelectCodepageError

RefreshSW:
                call    [cs:pRefreshSWcp]

                pop     ds
                pop     es
                pop     cx

                mov     dx,ERR_SelPrepDeviceError
                jc      SelectCodepageError

                mov     [cs:wCPbuffer],bx
                                        ; the buffer we copied

                mov     dx, ERR_SelPrepDeviceError
                                        ; in case of success, DX will be
                                        ; POP-ed

                jc      SelectCodepageError
                
                mov     BYTE [cs:bActive],1

                ;************ exit routines: we have BX=CP

SelectCodepageSuccess:

                mov     [cs:wCPselected],bx
                mov     ax, 0001h
                clc

                jmp     SelectCodepageEnd2

SelectCodepageError:
                mov     si,sp
                mov     word [ss:si],dx
                xor     ax,ax
                stc
                jmp     SelectCodepageEnd2

SelectCodepageEnd:
                pop     cx
SelectCodepageEnd2:
                pop     dx
                pop     si
                pop     di
                ret
                
