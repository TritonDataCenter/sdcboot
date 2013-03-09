
;   FreeDOS DISPLAY.SYS           v0.13
;   FreeDOS PRINTER.SYS
;
;   ===================================================================
;
;   PREPARE RESIDENT SUBROUTINES
;
;   ===================================================================
;
;   FUNCTION summary:
;
;   EXPORTS:
;
;   MoveDataToBuffer ( IN:      DS:SI-> Data origin
;                               DL      Target buffer
;                               DI      Offset on buffer
;                               CX      Data size
;                      OUT:     CF      clear on success, set on error
;                               SI      updated to byte after data
;                               DL      preserved
;                     )
;                    All other registers preserved
;
;   PrepareCodepage   ( IN:     DS:SI-> CPI file to be prepared
;                               CX:     Size of the CPI file
;                       OUT:    DX:     0 if success, otherwise error code )
;                     (Before calling this, the prepare structure:
;                         wToPrepareSize, wToPrepareBuf
;                      must be filled with the appropriate info)
;
;   INTERNAL FUNCTIONS:
;   - PrepareThisCodepage: given certain codepage, prepare it to the
;     buffers where it was requested
;
;       .       .       .       .       .       .       = RULER =

; Fn:   MoveDataToBuffer
; Does: Moves font data from certain buffer to the SELECT buffer
;       SIZE and OFFSET of the data to be moved should be given
; In:   DL: Number of buffer that is to be used for the transfer (0,1,...)
;       CX: size (in bytes) to be moved
;       DS:SI-> Pointer to memory where source data resides
;       DI: offset on the table to be transferred
; Out:  Carry set on error, clear on success
;       DL must be preserved
;       SI gets updated to the new position

MoveDataToBuffer:
                push    dx
                push    bx              ; get the table entry

                shl     dx,1
                mov     bx,wBufferPtrs
                add     bx,dx
                mov     ax,[cs:bx]

                pop     bx

                push    cx              ; determine wether XMS or TPA
                mov     cx,dx
                shr     cx,1            ; undo the SHL above
                mov     dx,1
                shl     dl,cl
                test    [cs:fBuffersInXMS],dl
                pop     cx
                pop     dx
                jz      MoveDataToBufferTPA

                ;****** Move procedures:
                ; AX: table entry
                ; CX: size (bytes)
                ; DS:SI-> source data
                ; DI: offset

MoveDataToBufferXMS:

                mov     [cs:XMSMoveLen],cx

                mov     word [cs:XMSMoveSrcH],0
                mov     [cs:XMSMoveSrcO],si
                mov     [cs:XMSMoveSrcO+2],ds

                mov     [cs:XMSMoveTrgH],ax
                mov     [cs:XMSMoveTrgO],di
                mov     word [cs:XMSMoveTrgO+2],0

                add     si,cx
                push    si
                push    ds

                mov     ah,0Bh
                push    cs
                pop     ds
                mov     si,XMSMoveLen

                call    far [cs:lpXMSdriver]

                pop     ds
                pop     si

                cmp     ax,1
                je      MoveDataToBufferSuccess
                stc
                ret

MoveDataToBufferTPA:
                push    cs
                pop     es

                add     di,ax

                shr     cx,1            ; WORD granularity!
                cld
           rep  movsw

MoveDataToBufferSuccess:
                clc
                ret


; Fn:   PrepareThisCodepage
; Does: For the codepage pointed by DS:SI, fill in all buffers that apply
; In:   DS:SI->  Where the codepage starts (see description in ReadCodepage)
; Out:  CF=0:    Everything is ok, loop on (CPs on file) must continue
;       CF=1:    There was an error in the CPI file (DX contains errorcode)

; FORMAT OF THE CODEPAGE (offsets respect to SI)
; ----------------------
;  0  DW      FontHeaderSize (usually 28)
;  2  DD      Far Pointer to Next Codepage (FONT)
;  6  DW   *  Driver type signature (1=DISPLAY, 2=PRINTER)
;  8  8DB  *  Hardware type string (e.g. "EGA     ")
; 16  DW   +  CP-ID (e.g. 850)
; 18  6DB     Reserved (empty)
; 24  DD      Pointer to next FontHeader
; --- the  limit of the font header (28)
;
; (*)  to be tested

PrepareThisCodepage:

                xor     dx,dx
LoopPrepareList:
                ; see if it is on our prepare list
                mov     bx,dx
                shl     bx,1
                mov     bx,[cs:bx+wToPrepareBuf]

                cmp     bx,[ds:si+16]
                jne     LoopPrepareListBreak


                ; found! prepare it
                push    bx
                push    dx
                push    si

                add     si,28           ; DS:SI -> begining of the font
                
                call    [cs:pReadFont]
                
                pop     si
                pop     dx
                pop     bx

                jc      LoopPrepareListBreak    ; everything OK?
                
                mov     di, [cs:wNumHardCPs]    ; store CP number
                add     di, dx
                shl     di, 1
                add     di, wCPList
                mov     [cs:di], bx
                
                mov     bx,dx                   ; mark this one as prepared
                shl     bx,1
                mov     word [cs:bx+wToPrepareBuf],0

LoopPrepareListBreak:
                inc     dx

                cmp     dx,[cs:wToPrepareSize]
                jb      LoopPrepareList

PrepareThisCodepageCont:
                clc
PrepareThisCodepageEnd:
                ret

;
; Fn:	CheckSize
; Does:	Checks that this codepage fits in the buffer
; In:	DS:SI->	codepage entry header
;	BX = bytes left in file
; Out:	AX trashed
;	Carry set if codepage doesn't fit
;
CheckSize:
                push	si
		mov	si,[ds:si+24]
		add	si,[cs:SI0]	;SI -> info header
		mov	ax,[ds:si+4]	;AX = info header size
		add	ax,si
		sub	ax,[cs:SI0]	;SI -> info header
		pop	si
		add	ax,6		;AX = address of last byte beyond font
		cmp	bx,ax
		ret

; Fn:   PrepareCodepage
; Does: Prepares the codepage in the appropiate position
; In:   CX:      Size of the whole CPI file
;       DS:SI->  RAW CPI structure
; Out:  DX:      Error code, 0 if not error
;
; FORMAT OF THE CODEPAGE (FONT)
; ----------------------
;  0  DB      0ffh, "FONT   "
;  8  DB 15   (ignore)
; 23  DW      Number of CPs in file
; 25  --- (CP 1 starts here)


SI0             DW      0

PrepareCodepage:
                push    ax
                push    cx
                push    bx
                push    es
                push    si
                push    di


                mov     [cs:SI0], si

                ;*** header of the CPI file

                push    cs              ; ES segment = CS
                pop     es

                mov     bx,cx           ; BX to hold the CPI structure size

                mov     di, sCPIsignature
                mov     cx, 8
           repe cmpsb
                jne     ErrorInSubfonts

                mov     cx, [ds:si+15]  ; 15=23-8; number of CPs in files
                add     si,17           ; si-> first font

loopFindCP:
;                cmp     bx, 9780
                call    CheckSize
                jb      ErrorInSubfonts

                mov     dx,[si+6]      ; Correct driver signature
                cmp     dx,[cs:DriverSignature]
                jne     loopFindCPnext
                                        ; Correct FONT name, according to
                push    cx              ; the hardware-specific code
                push    si              ; DS:SI->String in CPI file
                add     si,8

                push    cs              ; ES:DI->Our font name
                pop     es
                mov     di,[cs:psCPIHardwareType]

                mov     cx,8
          repe  cmpsb
                pop     si
                pop     cx
                jne     loopFindCPnext

                push    bx              ; OK: prepare this one where it
                push    cx              ; is appropriate
                call    PrepareThisCodepage
                pop     cx
                pop     bx

                jc      prepareEnd

loopFindCPnext:
                ;       next
                mov     si,[ds:si+2]
                add     si,[cs:SI0]
;                add     si,9780         ; 9780=distance between fonts
;                sub     bx,9780

                loop    loopFindCP

                ;       check if all the requested have been prepared
                xor     dx,dx
                mov     cx,[cs:wToPrepareSize]

loop3:          mov     bx,cx
                dec     bx
                shl     bx,1
                or      dx,[cs:bx+wToPrepareBuf]
                loop    loop3

                test    dx,0FFFFh
                jz      prepareEnd

                mov     dx, ERR_DevCPNotFound
                jmp     prepareEnd

ErrorInSubfonts:
                mov     dx,ERR_FileDamaged

prepareEnd:
                pop     di
                pop     si
                pop     es
                pop     bx
                pop     cx
                pop     ax
                ret

