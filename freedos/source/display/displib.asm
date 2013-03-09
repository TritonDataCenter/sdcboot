;
;   FreeDOS DISPLAY.SYS           v0.13
;   FreeDOS PRINTER.SYS
;
;   ===================================================================
;
;   TRANSIENT UTILITIES TO DISPLAY/PRINTER
;
;   ===================================================================
;
;   FUNCTION summary:
;
;   EXPORTS:
;
;   IsBlank        ( IN:        AL=character to be tested
;                    OUT:       CF set if it is blank, clear otherwise )
;                    All other registers preserved
;                  BLANK: 32, 9, 10, 13
;
;   OutStrDX       ( IN:        DX: near pointer to string
;                    OUT:       CF set  )
;                    Displays a string to CON: (uses DOS)
;
;   WriteNumber    ( IN:        AX: number of at most 3 cyphers
;                    OUT:       [CS:CopyVer]  ASCII version of the number )
;                    Writes a number in ASCII to CON:
;                    Must always be called after displaying the copyright.
;
;  SyntaxError     ( IN:        DX: Error string to be displayed
;                    OUT:       NEVER RETURNS )
;                    Writes to CON: the error number and the string, then
;                    aborts the program
;
;  ReadNumber      ( IN:        [ES:SI] ASCII string of the number
;                    OUT:       AX: the number read  )
;                    Reads the number. It aborts at the first non-number or
;                    after 5 cyphers
;
;  ReadSW          ( IN:        [ES:SI] list of ASCII numbers n,m,...
;                               [ES:DI] array of word to store the data
;                               CH:     length of the previous array
;                    OUT:       [ES:DI] filled with the data
;                               CL:     actual length of the array  )
;                    Reads a list of comma separated numbers to an array.
;                    Stop conditions:
;                    - maximum length (ch) is reached
;                    - ')' character is found
;                    - illegal (not number, not ',', not ')') character
;
;  mAlloc          ( IN:        AX: bytes to allocate
;                    OUT:       DX: near ptr (wrt CS) of the allocated
;                                   memory block
;                                   (0 if it couldn't be allocated) )
;                    Reserve memory from the heap
;
;  XMSAlloc        ( IN:        AX: bytes to allocate
;                    OUT-OK:    FC clear
;                               DX: handle to the EMB
;                    OUT-ERROR: FC set   )
;                    Reserve memory in an extended memory block
;
;  AllocateBuffers ( IN:        -
;                    OUT:       NEVER RETURNS on not enough memory
;                               Otherwise, returns with buffers allocated )
;                    Reserve memory for the prepared buffers.
;                    Variable [cs:wBufferSize] MUST be filled before the call
;                    Registers AX,BX,CX,DX,DI,SI are freely used
;



; Fn:   IsBlank
; In:   AL: character
; Out:  CarryFlag set if blank, clear if not

IsBlank:
                cmp     al,32
                jne     IsBlank_a
IsBlankExit1:   stc
                ret
IsBlank_a:
                cmp     al,10
                je      IsBlankExit1
                cmp     al,9
                je      IsBlankExit1
                cmp     al,13
                je      IsBlankExit1
                clc
                ret

; Fn:   OutStrDX
; In:   DX: near pointer to string to be displayed
; Out:  -

OutStrDX:       mov     ah,9
                int     21H
		stc			; Error
                ret			; From DoInstall


; Fn:   WriteNumber
; In:   AX: number of at most 3 cyphers to be written
; Out:  -
; Note: copyVer space is reused (as it is no longer used when
;       this function is called)
;       I know this is not the most optimal code in the world...

NumberSpace     db      "     $"

WriteNumber:    mov     di, NumberSpace
                mov     bx,100
                push    cs
                pop     es
                call    DWriteCypher
div10:          mov     al,ah
                xor     ah,ah
                mov     bx,10
                call    DWriteCypher
                mov     al,ah
                call    WriteCypher
                mov     BYTE [es:di],'$'
                mov     dx, NumberSpace
                jmp     OutStrDX              ; write the str and exit

DWriteCypher:   div     bl

WriteCypher:    add     al,'0'
                mov     [es:di],al
                inc     di
retWC:          ret


; Fn:   SyntaxError
; In:   DX: error string to be displayed
; Out:  -

SyntaxError:    push    dx                 ; save address of string to output
                mov     dx, SyntaxErrorStr
                call    OutStrDX

                mov     ax, si
                sub     ax, CommandLine_Offset
                call    WriteNumber

                pop     dx
SyntaxError2:
                call    OutStrDX

                mov     dx, ReturnString
                call    OutStrDX

                sti
                stc
                jmp     Done

; Fn:   ReadNumber
; In:   ES:SI -> a string number of at most 5 cyphers
; Out:  AX the number read
; Note: it stops whenever it finds the first non-number character or
;       whenever 5 characters are read
;       I know this is not the most optimal code in the world...


ReadNumber:     xor     ax,ax
                xor     bx,bx
                mov     cx,5
loopA:          cmp     BYTE [es:si],'0'
                jb      EndReadNumber
                cmp     BYTE [es:si],'9'
                ja      EndReadNumber
                mov     dx, 10             ; DX annihilated by MUL!
                mul     dx
                mov     bl, [es:si]
                add     ax, bx
                sub     ax, '0'
                inc     si
                loop    loopA
EndReadNumber:  ret



; Fn:   ReadSW
; In:   ES:SI -> list of the form [number,] ... )
;       ES:DI -> array of WORD to store the list
;       CH maximum admissible length
; Out:  CL lenght of the list
; Note: it stops whenever it finds the )
;       on error condition (illegal char, list too long) it aborts
;       automatically


ReadSW:         xor     cl, cl
NextItemListSW: push    cx
                call    ReadNumber
                pop     cx
                inc     cl
                stosw
                lodsb
                cmp     al, ','
                jne     NotCommaSW
                cmp     cl, ch
                jb      NextItemListSW
                mov     dx, SES_ListTooLong
                jmp     SyntaxError
NotCommaSW:     cmp     al, ')'
                je      ReadSWEnd
                mov     dx, SES_IllegalChar
                jmp     SyntaxError
ReadSWEnd:      ret


; Fn:   mAlloc
; In:   AX: number of bytes to allocate
; Out:  DX: near ptr (wrt to CS) where memory was allocated
;           0 if it couldn't be allocated
; Does: Reserves memory from the local DISPLAY heap

mAlloc:         mov     dx,[cs:pHeapBase]
                push    dx
                add     dx,ax
                cmp     dx,[cs:pMemTop]
                jbe     mAllocOk

                pop     dx              ; failure: not enough memory
                xor     dx,dx
                ret

mAllocOk:       mov     [cs:pHeapBase],dx
                pop     dx
                ret

; Fn:   XMSAlloc
; In:   AX: number of bytes to allocate
; Out:  FCarry set on error
;       DX: handle to EMB in XMS
; Does: Tries to allocate AX bytes in an EMB in extended memory

XMSAlloc:
                test    byte [cs:fIsXMS],0FFh
                jnz     XMSAllocOk
                stc
                ret
XMSAllocOk:
                push    ax
                
                push    cx
                add     ax,1023         ; ax: size in KBs
                mov     cx,10
                shr     ax,cl
                pop     cx

                mov     dx,ax
                mov     ah,9
                push    bx
                push    cx
                call    far [cs:lpXMSdriver]
                pop     cx
                pop     bx

                xor     al,1            ; driver returns 1 on success
                rcr     ax,1

                pop     ax
                ret

; Fn:   AllocateBuffers
; In:   -
; Out:  NEVER RETURNS on not enough memory
; Does: Allocate the prepare buffers in XMS (if available) else in the heap

AllocateBuffers:

                mov     ax,[cs:wBufferSize]     ; get size

                call    mAlloc                  ; SELECT buffer
                                                ; ALWAYS assumed at TOP of heap

                mov     si,wBufferPtrs

                xor     cx,cx                   ; CX: global counter
                xor     bx,bx                   ; BH: allocated in XMS
                                                ; BL: allocated in conventional

loop5:
                call    XMSAlloc                ; loop: allocate PREP. buffers
                jc      TryLocalHeap

                inc     bh
                push    ax
                mov     al,1
                shl     al,cl
                or      [cs:fBuffersInXMS],al
                pop     ax
                jmp     loop5next

TryLocalHeap:
                call    mAlloc                  ; second try: TPA memory
                inc     bl

                test    dx,dx                   ; allocated?
                jnz     loop5next

                mov     dx, SES_NoAllocatedBufs
                jmp     SyntaxError

loop5next:
                mov     [cs:si],dx              ; save for the first buffer
                add     si,2

                inc     cx
                cmp     cx,[cs:wNumSoftCPs]
                jb      loop5                   ; end of loop

                mov     dx,sMemAllocatedBuffers
                call    OutStrDX
                xor     ah,ah
                mov     al,bl
                push    bx
                call    WriteNumber
                pop     bx
                mov     dx,sInTPA
                call    OutStrDX
                xor     ah,ah
                mov     al,bh
                call    WriteNumber
                mov     dx,sInXMS
                call    OutStrDX
                mov     dx,ReturnString
                call    OutStrDX

                ret

