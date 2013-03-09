
;   FreeDOS DISPLAY.SYS           v0.13
;   FreeDOS PRINTER.SYS
;
;   ===================================================================
;
;   COMMANDLINE PARSING FOR DISPLAY/PRINTER
;
;   ===================================================================
;

;   Functions assume that [CS:SI] points to the begining of a new param
;   At the end, [CS:SI] points to the first character after the param
;   Registers are freely used (except for SI)
;
;   FUNCTION summary:
;
;  ParseDeviceParameter:    drivername[:]=(hwname,hwlist,swlist)
;
;                   hwname: hardware type name, in HWinit.asm table
;                   hwlist: single CP, or CP list (cp,cp,...) for the
;                           hardcoded hardware codepages
;                   swlist: n        number of required buffers or
;                           (n,u)    n: number of required buffers
;                                    u: user-defined parameter for the driver
;
;  ParseSwitch              /C       Unique: no other DISPLAY can be loaded
;                           /NOHI    Do not use XMS
;

;   ===================================================================
;
;   CONSTANTS
;
;   ===================================================================
;               .       .       .       .       .       line that rules


Param_NOHI      DB      "NOHI"


;   ===================================================================
;
;   PARSING A DEVICE DRIVER PARAMETER
;
;   ===================================================================
;               .       .       .       .       .       line that rules

ParseDeviceParameter:

                ; 0.- Save device name
                ;     Also block illegal chars (ASCII compatibility
                ;     assumed)

                mov     di, DevName
                mov     cx, 8
loop2:          mov     al, [es:si]
                cmp     al, '='
                je      EndDevName
                cmp     al, ':'
                je      EndDevName
                cmp     al, 13
                jne     skp2
                mov     dx, SES_UnexpectedEOL
                jmp     SyntaxError
skp2:           cmp     al, '/'
                je      illegalChar
                cmp     al, '\'         ; character '\
                je      illegalChar
                cmp     al, 34          ; illegal char: double quote
                je      illegalChar
                cmp     al, '*'
                je      illegalChar
                cmp     al, '|'
                je      illegalChar
                cmp     al, '<'         ; illegal chars: '<'..'?'
                jb      cont1
                cmp     al, '?'
                ja      cont1
                cmp     al, '='         ; and =
                je      cont1              
                jmp     illegalChar
cont1:          movsb
                loop    loop2
                mov     dx, SES_NameTooLong
                jmp     SyntaxError

illegalChar:    mov     dx, SES_IllegalChar
                jmp     SyntaxError

EndDevName:
                cmp     al, ':'
                jne     cont2
                inc     si

cont2:          inc     si              ; it was pointing to =
                mov     al, '('
                mov     di, si
                scasb
                je      skp3
                mov     dx, SES_OpenBrExpected
                jmp     SyntaxError
       

                ; 1.- Get hardware name, and save the pointer to the Init
                ;     procedure, and the parameter from the table at HWinit

skp3:
                inc     si
                mov     di, HardwareTables

loop3b:
                push    si              ; si->begining of the name
                mov     cx, 8           ; length
           repe cmpsb
                je      HwTypeFound8c   ; if =, CX=0 => found (check ,)
                mov     al, [ds:si-1]
                mov     dl, [es:di-1]

                add     di,cx           ; di->CP-HardwareName
                
                cmp     al, ','
                jne     skp3b           ; if commandline<>, error

                test    dl, 0ffh
                jz      HwTypeFound     ; if sourcename<>0 error

skp3b:
                add     di,12           ; skip the table
                mov     al, [es:di]
                pop     si              ; recover si->beginig of the name
                test    al, 0ffh        ; 0=end of the table
                jnz     loop3b

                mov     dx, SES_WrongHwName
                jmp     SyntaxError     ; hardware NOT supported

HwTypeFound8c:  mov     al, [ds:si]     ; if name is 8 char long, we still
                cmp     al, ','         ; need to check the comma
                jne     needC
                inc     si
                
HwTypeFound:
                mov     ax,[cs:di+2]
                mov     [cs:wInitParam],ax
                mov     [cs:pInitProc],di

                pop     di              ; discard 1 element from stack
                                        ; (was: begining of HwName in params)
                jmp     GetHWCodepages  ; goto next stage

needC:          ; error if comma not found
                mov     dx, SES_CommaExpected
                jmp     SyntaxError

                ; 2.- Hardware codepage list

GetHWCodepages:
                cmp     byte [cs:si],'('
                jne     HW1codepage
                inc     si
                mov     di,wCPList
                mov     ch,MaxCPNumbers
                call    ReadSW          ; routine to read a list of words
                xor     ch,ch
                mov     [cs:wNumHardCPs],cx
                jmp     CommaBeforeFonts
HW1codepage:
                call    ReadNumber
                mov     [cs:wCPList], ax

CommaBeforeFonts:
                mov     di, si
                mov     al,','
                scasb
                jne     needC           ; , expected
                inc     si

                ; 3.- Software buffers and user parameter

                cmp     byte [cs:si],'('
                jne     NoUserParam     ; (fonts,userparam) specified

                inc     si
                mov     di,wNumSoftCPs
                mov     ch,2
                call    ReadSW
                mov     ax,[cs:wNumSoftCPs]
                jmp     CheckFontNumber
NoUserParam:
                call    ReadNumber      ; number of fonts only

CheckFontNumber:
                cmp     ax, 8           ; maximum: 8 (due to the inXMS flags)
                jna     numberOk
                mov     dx, SES_TooManyPools
                jmp     SyntaxError

numberOk:
                test    al,0ffh         ; minimum: 1
                jnz     jmp4b
                inc     al
jmp4b:
                mov     [cs:wNumSoftCPs], ax

                ; 4.- No more arguments: closing bracket

                mov     di, si
                mov     al,')'
                scasb
                je      jmp5
                mov     dx, SES_CloseBrExpected
                jmp     SyntaxError
jmp5:
                inc     si              ; place SI AFTER the parameter
                clc
                ret

;   ===================================================================
;
;   SWITCH PARSING
;
;   ===================================================================
;               .       .       .       .       .       line that rules


errNoXMS       DB     "Will NOT use XMS", 0dH, 0aH, "$"


ParseSwitch:
                lodsb

                ;       Parameter: /C (DISPLAY only)
%ifdef DISPLAY

                cmp     al, "C"
                jne     next1
                mov     al,[si]
                call    IsBlank
                jnc     invalid
                
                mov     ax,0ad00H
                int     2fH
                cmp     al,0FFH
                jne     ExitParseSwitch

                mov     dx,errAlready
                jmp     OutStrDX
                stc
                ret

next1:
%endif
                ;       Parameter: /V (Verbose)

                cmp     al, "V"
                jne     next2
                mov     al,[si]
                call    IsBlank
                jnc     invalid

                mov     byte [cs:bVerbosity],2

ExitParseSwitch:
                clc
                ret
                
next2:

                ;       Parameter: /NOHI (DISPLAY only)
                dec     si
                mov     di, Param_NOHI
                mov     cx, 4           ; length
           repe cmpsb
                jne     invalid

                mov     BYTE [cs: bMemoryUssage], 0
                clc
                ret


invalid:
                mov     dx, SES_InvalidParameter
                jmp     SyntaxError     ; Invalid parameter




;   ===================================================================
;
;   MAIN COMMANDLINE PARSING FUNCTION
;
;   ===================================================================
;               .       .       .       .       .       line that rules


DeviceParsed    DB      0

; Fn:   ParseCommandLine
; In:   SI-> First char on the commandline, which ends in ASCII 13
; Out:  CF on error (do not stay resident)


ParseCommandLine:

                ;****** Capitalise string
                push    si
loop0:
                lodsb
                cmp     al, 13
                je      endUpper
                cmp     al, 'a'
                jb      cont0
                cmp     al, 'z'
                ja      cont0
                sub     al, 'a'-'A'
                mov     [si-1], al
cont0:
                loop    loop0
endUpper:
                pop     si

                ; 2.- Loop on parameters
    

loop1:
                lodsb
                cmp     al, 13
                je      skp1
                call    IsBlank
                jc      loop1

                cmp     al, '/'
                jne     skp1b
                call    ParseSwitch
                jnc     loop1
                ret

skp1b:
                test    BYTE [cs:DeviceParsed],0FFH
                jz      skp1c

                mov     dx, SES_WrongNumberPars
                jmp     SyntaxError     ; wrong # of params specified

skp1c:
                mov     BYTE [cs:DeviceParsed],1
                call    ParseDeviceParameter
                jnc     loop1
                ret

skp1:
                ; 3.- After loop: check required parameters
                
                cmp     BYTE [cs:DeviceParsed],1
                je      jmp6

                mov     dx, SES_ParamRequired
                jmp     SyntaxError
jmp6:

                ; 4.- Everything OK, exit ok
                clc
                ret

                
